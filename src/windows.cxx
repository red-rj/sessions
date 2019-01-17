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
#include <cassert>

#include "impl.hpp"
#include "ixm/session_impl.hpp"

namespace {
    using namespace ixm::session::detail;

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

        using iterator = std::vector<const char*>::iterator;
    
        environ_table()
        {
            // make sure _wenviron is initialized
            // https://docs.microsoft.com/en-us/cpp/c-runtime-library/environ-wenviron?view=vs-2017#remarks
            if (!_wenviron) {
                _wgetenv(L"initpls");
            }
        }

        environ_table(const environ_table&) = delete;
        environ_table(environ_table&& other) = delete;

        ~environ_table()
        {
            free_env();
        }


        void sync()
        {
            free_env();
            init_env();
        }

        char const** data() noexcept { return m_env.data(); }

        size_t size() const noexcept { return m_env.size() - 1; }

        auto begin() noexcept { return m_env.begin(); }
        auto end() noexcept { return m_env.end() - 1; }

        const char* getvar(ci_string_view key)
        {
            auto it = getvarline(key);

            if (it == end()) {
                // not found, try the OS environment
                auto vl = get_from_os(key.data());
                if (vl) {               
                    // found! add to our env
                    // insert b4 the terminating null
                    it = m_env.insert(end(), vl);
                }
            }
            
            return it != end() ? *it + key.length() + 1 : nullptr;
        }

        void setvar(const char* key, const char* value)
        {
            setvarline(key, value);

            auto wkey = to_utf16(key), wvalue = to_utf16(value);
            _wputenv_s(wkey.get(), wvalue.get());
        }

        void rmvar(const char* key) 
        {
            auto it = getvarline(key);
            if (it != end()) {
                auto* old = *it;
                m_env.erase(it);
                delete[] old;
            }

            auto wkey = to_utf16(key);
            _wputenv_s(wkey.get(), L"");
        }

        int find_pos(const char* key)
        {
            auto it = getvarline(key);
            return it - begin();
        }

    private:

        auto getvarline(ci_string_view key) -> iterator
        {
            return std::find_if(begin(), end(), 
            [&](ci_string_view entry){
                return 
                    entry.length() > key.length() &&
                    entry[key.length()] == '=' &&
                    entry.compare(0, key.size(), key) == 0;
            });
        }

        auto setvarline(ci_string_view key, ci_string_view value) -> iterator
        {
            auto it = getvarline(key);
            const char* vl = new_varline(key, value);
            
            if (it == end()) {
                // insert b4 the terminating null
                it = m_env.insert(end(), vl);
            }
            else {
                const char* old = *it;
                *it = vl;
                delete[] old;
            }

            return it;
        }

        [[nodiscard]]
        char* get_from_os(const char* key)
        {
            char* retval{};
            auto wkey = to_utf16(key);
            wchar_t* wvalue = _wgetenv(wkey.get());

            if (wvalue) {
                retval = new_varline(key, to_utf8(wvalue).get());
            }
            
            return retval;
        }

        [[nodiscard]]
        char* new_varline(ci_string_view key, ci_string_view value)
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
            for (auto& elem : m_env) {
                // assumes default deleter
                delete[] elem;
            }

            m_env.clear();
        }

        void init_env()
        {
            wchar_t** wide_environ = _wenviron;

            for (int i = 0; wide_environ[i]; i++)
            {
                // we own the converted strings
                m_env.push_back(to_utf8(wide_environ[i]).release());
            }

            m_env.emplace_back(); // terminating null
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

    void env_sync() {
        environ_.sync();
    }

    int env_find(char const* key) noexcept {
        return environ_.find_pos(key);
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
    }

    void rm_env_var(const char* key) noexcept
    {
        environ_.rmvar(key);
    }

} /* namespace impl */
