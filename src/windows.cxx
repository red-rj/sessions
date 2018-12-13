#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <system_error>

#include <cstdlib>

#include "impl.hpp"
#include "ixm/session_impl.hpp"


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

    using namespace ixm::session::detail;

    class environ_table
    {
        using env_iterator = std::vector<const char*>::iterator;

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
            return m_env.data();
        }

        size_t size() const noexcept { return m_env.size() - 1; }

        auto begin() noexcept { return m_env.begin(); }
        auto end() noexcept { return m_env.end() - 1; }

        
        auto getvar(ci_string_view key) noexcept
        {
            auto it = getvarline(key);
            return it != m_env.end() ? *it + key.length() + 1 : nullptr;
        }

        void setvar(std::string_view key, std::string_view value)
        {
            auto varline = make_varline(key, value);
            auto it = getvarline({key.data(), key.length()});
            
            if (it == end()) {
                // insert b4 the terminating null
                m_env.insert(end(), varline.release());
            }
            else {
                const char* old = *it;
                *it = varline.release();
                delete[] old;
            }

            _putenv_s(key.data(), value.data());
        }

        void rmvar(const char* key) 
        {
            auto it = getvarline(key);
            if (it == m_env.end()) 
                return;

            m_env.erase(it);
            _putenv_s(key, "");
        }

    private:
        auto getvarline(ci_string_view key) noexcept -> env_iterator
        {
            return std::find_if(begin(), end(), 
            [&](ci_string_view entry){
                return 
                    entry.length() > key.length() &&
                    entry[key.length()] == '=' &&
                    entry.compare(0, key.size(), key) == 0;
            });
        }

        auto make_varline(std::string_view key, std::string_view value) -> std::unique_ptr<char[]>
        {
            const auto buffer_sz = key.size()+value.size()+2;
            auto buffer = std::make_unique<char[]>(buffer_sz);
            key.copy(buffer.get(), key.size());
            buffer[key.size()] = '=';
            value.copy(buffer.get() + key.size() + 1, value.size());

            return buffer;
        }

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
        return environ_.getvar(key);
    }

    void set_env_var(const char* key, const char* value) noexcept
    {
        environ_.setvar(key, value);
    }

    void rm_env_var(const char* key) noexcept
    {
        environ_.rmvar(key);
    }

} /* namespace impl */
