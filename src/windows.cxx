#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <string>
#include <system_error>

#include <cstdlib>

#include "impl.hpp"


namespace {

    [[noreturn]]
    void throw_win_error(DWORD error = GetLastError())
    {
        throw std::system_error(std::error_code{ static_cast<int>(error), std::system_category() });
    }

    auto narrow(wchar_t const* wstr, char* ptr = nullptr, int length = 0) {
        return WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            wstr,
            -1,
            ptr,
            length,
            nullptr,
            nullptr);
    }

    auto wide(const char* nstr, wchar_t* ptr = nullptr, int lenght = 0) {
        return MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            nstr,
            -1,
            ptr,
            lenght
        );
    }

    auto to_utf8(wchar_t const* wstr) {
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
        auto cl = GetCommandLineW();
        int argc;
        auto wargv = CommandLineToArgvW(cl, &argc);

        auto vec = std::vector<char const*>(argc+1, nullptr);

        for (int i = 0; i < argc; i++)
        {
            vec[i] = to_utf8(wargv[i]).release();
        }

        LocalFree(wargv);

        return vec;
    }

    auto& args_vector() {
        static auto value = initialize_args();
        return value;
    }


    class environ_table
    {
    public:
        environ_table()
        {
            // make sure _wenviron is initialized
            // https://docs.microsoft.com/en-us/cpp/c-runtime-library/environ-wenviron?view=vs-2017#remarks
            if (!_wenviron) {
                _wgetenv(L"initpls");
            }

            init_env();
        }

        ~environ_table()
        {
            free_env();
        }


        char const** data() {
            if (!m_valid) {
                init_env();
            }
            return m_env.data();
        }

        size_t size(bool check = true) {
            if (check && !m_valid) {
                init_env();
            }
            return m_env.size() - 1;
        }

        auto begin() {
            if (!m_valid) {
                init_env();
            }
            return m_env.begin(); 
        }
        auto end() { return m_env.end() - 1; }


        void invalidate() noexcept { m_valid = false; }

    private:
        void init_env()
        {
            wchar_t** wide_environ = _wenviron;

            free_env();
            
            for (size_t i = 0; wide_environ[i]; i++)
            {
                // we own the converted strings
                m_env.push_back(to_utf8(wide_environ[i]).release());
            }

            m_env.emplace_back(); // terminating null
            
            m_valid = true;
        }

        void free_env() noexcept
        {
            // delete converted items
            for (auto& elem : m_env) {
                // TODO: assumes default deleter
                delete[] elem;
            }

            m_env.clear();
        }

        bool m_valid;
        std::vector<char const*> m_env;
    
    } environ_;


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
        return environ_.data();
    }

    size_t env_size() {
        return environ_.size();
    }

    char const* get_env_var(char const* key) noexcept
    {
        const auto key_len = strlen(key);
        
        for(auto& entry : environ_)
        {
            if (strlen(entry) <= key_len) continue;
            if (entry[key_len] != '=') continue;
            // case insensitive string compare w/ current locale
            if (_strnicoll(entry, key, key_len) != 0) continue;

            return entry + key_len + 1;
        }
        
        return nullptr;
    }

    void set_env_var(const char* key, const char* value) noexcept
    {
        _putenv_s(key, value);
        environ_.invalidate();
    }

    void rm_env_var(const char* key) noexcept
    {
        _putenv_s(key, "");
        environ_.invalidate();
    }

    const char env_path_sep = ';';

} /* namespace impl */
