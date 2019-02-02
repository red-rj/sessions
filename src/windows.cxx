#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <system_error>
#include <mutex>

#include <cstdlib>
#include <cassert>

#include "impl.hpp"
#include "ixm/session_impl.hpp"

namespace {
    using namespace ixm::session::detail;


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


    class environ_t
    {
        environ_t()
        {
            // make sure _wenviron is initialized
            // https://docs.microsoft.com/en-us/cpp/c-runtime-library/environ-wenviron?view=vs-2017#remarks
            if (!_wenviron) {
                _wgetenv(L"initpls");
            }

            try
            {
                for (wchar_t** wenv = _wenviron; *wenv; wenv++) 
                {
                    // we own the converted strings
                    m_env.push_back(to_utf8(*wenv).release());
                }

                m_env.emplace_back(); // terminating null
            }
            catch(const std::exception&)
            {
                free_env();
                std::throw_with_nested(std::runtime_error("Failed to create environment"));
            }
            
        }

    public:

        using iterator = std::vector<const char*>::iterator;

        environ_t(const environ_t&) = delete;
        environ_t(environ_t&&) = delete;

        ~environ_t()
        {
            free_env();
        }

        static auto& instance() 
        {
            static auto e = environ_t();
            return e;
        }


        auto data() noexcept { return m_env.data(); }
        auto size() const noexcept { return m_env.size() - 1; }
        auto begin() noexcept { return m_env.begin(); }
        auto end() noexcept { return m_env.end() - 1; }


        const char* getvar(ci_string_view key)
        {
            std::lock_guard lock{m_mtx};
            
            auto it = getvarline_sync(key);
            return it != end() ? *it + key.length() + 1 : nullptr;
        }

        void setvar(ci_string_view key, std::string_view value)
        {
            std::lock_guard lock{m_mtx};

            auto it = getvarline(key);
            const char* vl = new_varline(key, value);
            
            if (it == end()) {
                it = m_env.insert(end(), vl);
            }
            else {
                auto* old = std::exchange(*it, vl);
                delete[] old;
            }

            auto wvarline = to_utf16(vl);
            _wputenv(wvarline.get());
        }

        void rmvar(const char* key) 
        {
            std::lock_guard lock{m_mtx};

            auto it = getvarline(key);
            if (it != end()) {
                auto* old = *it;
                delete[] old;
                m_env.erase(it);
            }

            auto wkey = to_utf16(key);
            _wputenv_s(wkey.get(), L"");
        }

        int find_pos(const char* key)
        {
            std::lock_guard lock{m_mtx};

            auto it = getvarline_sync(key);
            return it != end() ? it - begin() : -1;
        }

    private:

        iterator getvarline(ci_string_view key) noexcept
        {
            return std::find_if(begin(), end(), 
            [&](ci_string_view entry){
                return 
                    entry.length() > key.length() &&
                    entry[key.length()] == '=' &&
                    entry.compare(0, key.size(), key) == 0;
            });
        }

        iterator getvarline_sync(ci_string_view key)
        {
            auto it_cache = getvarline(key);
            auto varline_os = std::unique_ptr<const char[]>{ varline_from_os(key) };

            if (it_cache == end() && varline_os)
            {
                // not found in cache, found in OS env
                it_cache = m_env.insert(end(), varline_os.release());
            }
            else if (it_cache != end())
            {
                if (varline_os)
                {
                    // found in both
                    std::string_view var_cache = *it_cache, var_os = varline_os.get();

                    // skip key=
                    auto const offset = key.size() + 1;
                    var_cache.remove_prefix(offset);
                    var_os.remove_prefix(offset);

                    // sync if values differ
                    if (var_cache != var_os) {
                        auto* old = std::exchange(*it_cache, varline_os.release());
                        varline_os.reset(old);
                    }
                }
                else
                {
                    // found in cache, not in OS. Remove
                    varline_os.reset(*it_cache);
                    m_env.erase(it_cache);
                    it_cache = end();
                }
            }

            assert(m_env.back() == nullptr);
            return it_cache;
        }

        [[nodiscard]]
        char* varline_from_os(ci_string_view key)
        {
            auto wkey = to_utf16(key.data());
            wchar_t* wvalue = _wgetenv(wkey.get());
            
            if (wvalue) {
                auto value = to_utf8(wvalue);
                return new_varline(key, value.get());
            }

            return nullptr;
        }

        [[nodiscard]]
        char* new_varline(ci_string_view key, std::string_view value)
        {
            const auto buffer_sz = key.size()+value.size()+2;
            auto buffer = new char[buffer_sz];
            key.copy(buffer, key.size());
            buffer[key.size()] = '=';
            value.copy(buffer + key.size() + 1, value.size());
            buffer[buffer_sz-1] = 0;

            return buffer;
        }

        void free_env() noexcept
        {
            // delete converted items
            for (auto elem : m_env) {
                delete[] elem;
            }
        }

        std::mutex m_mtx;
        std::vector<char const*> m_env;
    
    };

} /* nameless namespace */

namespace impl {

    char const* argv(std::size_t idx) noexcept {
        return ::args_vector()[idx];
    }

    char const** argv() noexcept {
        return ::args_vector().data();
    }

    int argc() noexcept { return static_cast<int>(args_vector().size()) - 1; }

    char const** envp() noexcept {
        return environ_t::instance().data();
    }

    int env_find(char const* key) {
        return environ_t::instance().find_pos(key);
    }

    size_t env_size() noexcept {
        return environ_t::instance().size();
    }

    char const* get_env_var(char const* key)
    {
        return environ_t::instance().getvar(key);
    }

    void set_env_var(const char* key, const char* value)
    {
        environ_t::instance().setvar(key, value);
    }

    void rm_env_var(const char* key)
    {
        environ_t::instance().rmvar(key);
    }

    const char path_sep = ';';

} /* namespace impl */
