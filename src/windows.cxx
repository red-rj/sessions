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
#include "red/session_envcache.hpp"


using namespace red::session::detail;

namespace {

    [[noreturn]]
    void throw_win_error(DWORD error = GetLastError())
    {
        throw std::system_error(static_cast<int>(error), std::system_category());
    }

    bool is_valid_utf8(const char* str) {
        return MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, 
            str, -1, 
            nullptr, 0) != 0;
    }

    auto narrow(wchar_t const* wstr, char* ptr = nullptr, int length = 0) {
        return WideCharToMultiByte(
            CP_UTF8, 0, 
            wstr, -1,
            ptr, length, 
            nullptr, nullptr);
    }

    auto wide(const char* nstr, wchar_t* ptr = nullptr, int length = 0) {
        const int CP = is_valid_utf8(nstr) ? CP_UTF8 : CP_ACP;
        return MultiByteToWideChar(
            CP, 0,
            nstr, -1,
            ptr, length);
    }

    auto to_utf8(const wchar_t* wstr) {
        auto length = narrow(wstr);
        auto ptr = std::make_unique<char[]>(length);
        auto result = narrow(wstr, ptr.get(), length);

        if (result == 0)
            throw_win_error();

        return ptr;
    }

    auto to_utf16(const char* nstr) {
        auto length = wide(nstr);
        auto ptr = std::make_unique<wchar_t[]>(length);
        auto result = wide(nstr, ptr.get(), length);
        
        if (result == 0)
            throw_win_error();

        return ptr;
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
            vec.resize(argc + 1);

            for (int i = 0; i < argc; i++)
            {
                vec[i] = to_utf8(wargv[i]).release();
            }
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


    [[nodiscard]]
    char* new_envstr(std::string_view key, std::string_view value)
    {
        const auto buffer_l = key.size() + value.size() + 2;
        auto buffer = new char[buffer_l];
        key.copy(buffer, key.size());
        buffer[key.size()] = '=';
        value.copy(buffer + key.size() + 1, value.size());
        buffer[buffer_l - 1] = 0;

        return buffer;
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

        auto predicate = ci_envstr_finder<wchar_t>(wkey.get());

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

            myenv.resize(wenv_l);

            std::transform(wenv, wenv + wenv_l, myenv.begin(), [](wchar_t* envstr) {
                // we own the converted strings
                return to_utf8(envstr).release();
            });
        }
        catch (const std::exception&)
        {
            this->~environ_cache();
            std::throw_with_nested(std::runtime_error("Failed to create environment"));
        }
    }

    environ_cache::~environ_cache() noexcept
    {
        for (auto* p : myenv) delete[] p;
    }


    std::string_view environ_cache::getvar(std::string_view key)
    {
        std::lock_guard _{ m_mtx };

        auto it = getenvstr_sync(key);
        if (it != end()) {
            std::string_view envstr = *it;
            return envstr.substr(key.size() + 1);
        }

        return {};
    }

    void environ_cache::setvar(std::string_view key, std::string_view value)
    {
        std::lock_guard _{ m_mtx };
        
        auto it = getenvstr(key);
        const char* vl = new_envstr(key, value);

        if (it == end()) {
            myenv.push_back(vl);
        }
        else {
            auto* old = std::exchange(*it, vl);
            delete[] old;
        }

        auto wvarline = to_utf16(vl);
        wvarline[key.size()] = L'\0';
        
        auto wkey = wvarline.get(), wvalue = wvarline.get() + key.size() + 1;
        _wputenv_s(wkey, wvalue);
    }

    void environ_cache::rmvar(std::string_view key)
    {
        std::lock_guard _{ m_mtx };
        
        auto it = getenvstr(key);
        if (it != end()) {
            auto* old = *it;
            delete[] old;
            myenv.erase(it);
        }

        auto wkey = to_utf16(key.data());
        _wputenv_s(wkey.get(), L"");
    }


    auto environ_cache::getenvstr(std::string_view key) noexcept -> iterator
    {
        return std::find_if(begin(), end(), ci_envstr_finder<char>(key.data(), key.size()));
    }

    auto environ_cache::getenvstr_sync(std::string_view key) -> iterator
    {
        auto it_cache = getenvstr(key);
        std::unique_ptr<const char[]> envstr_os;

        int pos = osenv_find_pos(key.data());
        if (pos != -1) {
            envstr_os = to_utf8(_wenviron[pos]);
        }

        if (it_cache == end() && envstr_os)
        {
            // not found in cache, found in OS env
            it_cache = myenv.insert(end(), envstr_os.release());
        }
        else if (it_cache != end())
        {
            if (envstr_os)
            {
                // found in both
                std::string_view var_cache = *it_cache, var_os = envstr_os.get();

                // skip key=
                auto const offset = key.size() + 1;
                var_cache.remove_prefix(offset);
                var_os.remove_prefix(offset);

                // sync if values differ
                if (var_cache != var_os) {
                    auto* old = std::exchange(*it_cache, envstr_os.release());
                    envstr_os.reset(old);
                }
            }
            else
            {
                // found in cache, not in OS. Remove
                envstr_os.reset(*it_cache);
                myenv.erase(it_cache);
                it_cache = end();
            }
        }

        return it_cache;
    }



}