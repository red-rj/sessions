#define _CRT_SECURE_NO_WARNINGS

#include "win32.hpp"
#include <shellapi.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <system_error>
#include <mutex>

#include <cstdlib>
#include <cassert>

#include "impl.hpp"
#include "red/session_impl.hpp"


using namespace red::session::detail;

namespace {

    [[noreturn]]
    void throw_win_error(DWORD error = GetLastError())
    {
        throw std::system_error(static_cast<int>(error), std::system_category());
    }

    bool is_valid_utf8(const char* str, int str_l = -1) {
        return MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, 
            str, str_l, 
            nullptr, 0) != 0;
    }

    auto narrow(wchar_t const* wstr, int wstr_l = -1, char* ptr = nullptr, int length = 0) {
        return WideCharToMultiByte(
            CP_UTF8, 0, 
            wstr, wstr_l,
            ptr, length, 
            nullptr, nullptr);
    }

    auto wide(const char* nstr, int nstr_l = -1, wchar_t* ptr = nullptr, int length = 0) {
        const int CP = is_valid_utf8(nstr, nstr_l) ? CP_UTF8 : CP_ACP;
        return MultiByteToWideChar(
            CP, 0,
            nstr, nstr_l,
            ptr, length);
    }
	

	std::string to_utf8(std::wstring_view wstr) {
		auto length = narrow(wstr.data(), wstr.size());
		auto str8 = std::string(length, '*');
		auto result = narrow(wstr.data(), wstr.size(), str8.data(), length);

		if (result == 0)
			throw_win_error();

		return str8;
	}

	std::wstring to_utf16(std::string_view nstr) {
		auto length = wide(nstr.data(), nstr.size());
		auto str16 = std::wstring(length, L'*');
		auto result = wide(nstr.data(), nstr.size(), str16.data(), length);

		if (result == 0)
			throw_win_error();

		return str16;
	}

    auto initialize_args() {
        int argc;
        auto wargv = std::unique_ptr<LPWSTR[], decltype(LocalFree)*>{
            CommandLineToArgvW(GetCommandLineW(), &argc),
            LocalFree
        };

        if (!wargv)
            throw_win_error();

        std::vector<char const*> vec;

        try
        {
            vec.reserve(argc + 1);

            for (int i = 0; i < argc; i++)
            {
				auto length = narrow(wargv[i]);
				char* ptr = new char[length];
				vec.push_back(ptr);
			
				auto result = narrow(wargv[i], -1, ptr, length);

				if (result == 0) {
					throw_win_error();
				}
            }

			vec.emplace_back();
        }
        catch (const std::exception&)
        {
            for (auto* p : vec) delete[] p;
            std::throw_with_nested(std::runtime_error("Failed to create arguments"));
        }

        return vec;
    }

    auto& args_vector() {
        static auto value = initialize_args();
        return value;
    }

} /* nameless namespace */

namespace impl {

    char const* argv(std::size_t idx) noexcept {
        return ::args_vector()[idx];
    }

    char const** argv() noexcept {
        return ::args_vector().data();
    }

    int argc() noexcept { return static_cast<int>(args_vector().size()) - 1; }


    const char path_sep = ';';



    int osenv_find_pos(const char* k)
    {
        auto** wenvp = _wenviron;
        auto wkey = to_utf16(k);

        auto predicate = ci_envstr_finder<wchar_t>(wkey);

        for (int i = 0; wenvp[i]; i++)
        {
            if (predicate(wenvp[i])) return i;
        }

        return -1;
    }

} /* namespace impl */

namespace red::session::detail
{
    using namespace impl;
    
    environ_cache::environ_cache()
    {
        // make sure _wenviron is initialized
        // https://docs.microsoft.com/en-us/cpp/c-runtime-library/environ-wenviron?view=vs-2017#remarks
        if (!_wenviron) {
            _wgetenv(L"initpls");
        }

        try
        {
            wchar_t** wenv = _wenviron;
            size_t wenv_l = osenv_size(wenv);

            myenv.reserve(wenv_l);

            std::transform(wenv, wenv + wenv_l, std::back_inserter(myenv), 
			[](wchar_t* envstr) {
                return to_utf8(envstr);
            });
        }
        catch (const std::exception&)
        {
            //this->~environ_cache();
            std::throw_with_nested(std::runtime_error("Failed to create environment"));
        }
    }

    environ_cache::~environ_cache() noexcept
    {
        //for (auto* p : myenv) delete[] p;
    }

    bool environ_cache::contains(std::string_view key) const
    {
        return osenv_find_pos(key.data()) != -1;
    }

    std::string_view environ_cache::getvar(std::string_view key)
    {
        std::lock_guard _{ m_mtx };

        auto it = getenvstr_sync(key);
        if (it != myenv.end()) {
            std::string_view envstr = *it;
            return envstr.substr(key.size() + 1);
        }

        return {};
    }

    void environ_cache::setvar(std::string_view key, std::string_view value)
    {
        std::lock_guard _{ m_mtx };
        
        auto it = getenvstr(key);
        auto vl = make_envstr(key, value);

        if (it == myenv.end()) {
            myenv.push_back(vl);
        }
        else {
			*it = vl;
        }

        auto wvarline = to_utf16(vl);
        wvarline[key.size()] = L'\0';
        
        auto wkey = wvarline.c_str(), wvalue = wvarline.c_str() + key.size() + 1;
        _wputenv_s(wkey, wvalue);
    }

    void environ_cache::rmvar(std::string_view key)
    {
        std::lock_guard _{ m_mtx };
        
        auto it = getenvstr(key);
        if (it != myenv.end()) {
            myenv.erase(it);
        }

        auto wkey = to_utf16(key);
        _wputenv_s(wkey.c_str(), L"");
    }


    auto environ_cache::getenvstr(std::string_view key) noexcept -> iterator
    {
        return std::find_if(myenv.begin(), myenv.end(), ci_envstr_finder<char>(key));
    }

    auto environ_cache::getenvstr_sync(std::string_view key) -> iterator
    {
        auto it_cache = getenvstr(key);
        std::string envstr_os;

        int pos = osenv_find_pos(key.data());
        if (pos != -1) {
            envstr_os = to_utf8(_wenviron[pos]);
        }

        if (it_cache == myenv.end() && !envstr_os.empty())
        {
            // not found in cache, found in OS env
            it_cache = myenv.insert(myenv.end(), envstr_os);
        }
        else if (it_cache != myenv.end())
        {
            if (!envstr_os.empty())
            {
                // found in both
                std::string_view var_cache = *it_cache, var_os = envstr_os;

                // skip key=
                auto const offset = key.size() + 1;
                var_cache.remove_prefix(offset);
                var_os.remove_prefix(offset);

                // sync if values differ
                if (var_cache != var_os) {
					*it_cache = envstr_os;
                }
            }
            else
            {
                // found in cache, not in OS. Remove
                myenv.erase(it_cache);
                it_cache = myenv.end();
            }
        }

        return it_cache;
    }

}

