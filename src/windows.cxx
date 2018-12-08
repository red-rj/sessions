#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <shellapi.h>

#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
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
        int argc; // skip invoke command
        auto wargv = CommandLineToArgvW(cl, &argc) + 1;

        auto vec = std::vector<char const*>(argc, nullptr);

        for (int i = 0; i < argc - 1; i++)
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
            init_env();
        }

        ~environ_table()
        {
            free_env();
        }


        char const** get() {
            if (!m_valid) {
                free_env();
                init_env();
            }

            return m_env.data();
        }


        void invalidate() noexcept { m_valid = false; }

    private:
        void init_env()
        {
            // make sure _wenviron is initialized
            // https://docs.microsoft.com/en-us/cpp/c-runtime-library/environ-wenviron?view=vs-2017#remarks
            if (!_wenviron) {
                _wgetenv(L"initpls");
            }

            wchar_t** wide_environ = _wenviron;

            free_env();
            
            for (size_t i = 0; wide_environ[i]; i++)
            {
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
    
    } g_env;


} /* nameless namespace */

namespace impl {

    char const* argv(std::size_t idx) noexcept {
        return ::args_vector()[idx];
    }

    char const** argv() noexcept {
        return ::args_vector().data();
    }

    int argc() noexcept { return static_cast<int>(args_vector().size()); }

    char const** envp() noexcept {
        return g_env.get();
    }

    void set_env_var(const char* key, const char* value) noexcept
    {
        auto ec = _putenv_s(key, value);
        _ASSERTE(ec == 0);
        g_env.invalidate();
    }

    const char env_path_sep = ';';

} /* namespace impl */
