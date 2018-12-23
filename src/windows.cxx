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

            wchar_t** wide_environ = _wenviron;

            for (size_t i = 0; wide_environ[i]; i++)
            {
                // we own the converted strings
                m_env.push_back(to_utf8(wide_environ[i]).release());
            }
            
            m_env.emplace_back(); // terminating null
        }

        ~environ_table()
        {
            // delete converted items
            for (auto& elem : m_env) {
                // assumes default deleter
                delete[] elem;
            }
        }


        char const** data() {
            return m_env.data();
        }

        size_t size() const noexcept { return m_env.size() - 1; }

        auto begin() noexcept { return m_env.begin(); }
        auto end() noexcept { return m_env.end() - 1; }

        
        const char* getvar(ci_string_view key) noexcept
        {
            auto it = getvarline(key);
            return it != end() ? *it + key.length() + 1 : nullptr;
        }

        void setvar(const char* key, const char* value)
        {
            auto it = getvarline(key);
            
            if (it == end()) {
                // insert b4 the terminating null
                m_env.insert(end(), new_varline(key, value));
            }
            else {
                const char* old = *it;
                *it = new_varline(key, value);
                delete[] old;
            }
        }

        void rmvar(const char* key) 
        {
            auto it = getvarline(key);
            if (it == m_env.end()) 
                return;

            auto* old = *it;
            m_env.erase(it);
            delete[] old;
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

        [[nodiscard]]
        char* new_varline(std::string_view key, std::string_view value)
        {
            const auto buffer_sz = key.size()+value.size()+2;
            auto buffer = new char[buffer_sz];
            key.copy(buffer, key.size());
            buffer[key.size()] = '=';
            value.copy(buffer + key.size() + 1, value.size());
            buffer[buffer_sz-1] = 0;

            return buffer;
        }

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

    size_t env_size() noexcept {
        return environ_.size();
    }

    char const* get_env_var(char const* key) noexcept
    {
        return environ_.getvar(key);
    }

    void set_env_var(const char* key, const char* value) noexcept
    {
        environ_.setvar(key, value);
        
        auto wkey = to_utf16(key), wvalue = to_utf16(value);
        _wputenv_s(wkey.get(), wvalue.get());
    }

    void rm_env_var(const char* key) noexcept
    {
        environ_.rmvar(key);

        auto wkey = to_utf16(key);
        _wputenv_s(wkey.get(), L"");
    }

} /* namespace impl */
