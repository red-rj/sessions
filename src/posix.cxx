#include <string>
#include <string_view>
#include <mutex>
#include <algorithm>
#include <vector>

#include <cstdlib>
#include <cstring>

#include "impl.hpp"

#if defined(__ELF__) and __ELF__
    #define SESSION_IMPL_SECTION ".init_array"
    #define SESSION_IMPL_ELF 1
#elif defined(__MACH__) and __MACH__
    #define SESSION_IMPL_SECTION "__DATA,__mod_init_func"
    #define SESSION_IMPL_ELF 0
#endif

using namespace std::literals;

extern "C" char** environ;

namespace {

    char const** argv__{ };
    int argc__{ };

    [[gnu::section(SESSION_IMPL_SECTION)]]
    auto init = +[](int argc, char const** argv, char const**) {
        argv__ = argv;
        argc__ = argc;
        if constexpr (SESSION_IMPL_ELF) { return 0; }
    };

    struct find_envstr
    {
        std::string_view key;

        explicit find_envstr(std::string_view k) : key(k) {}

        bool operator() (std::string_view entry) const noexcept
        {
            return
                entry.length() > key.length() &&
                entry[key.length()] == '=' &&
                entry.compare(0, key.size(), key) == 0;
        }
    };

} /* nameless namespace */


namespace impl {

    char const* argv(std::size_t idx) noexcept { return argv__[idx]; }
    char const** argv() noexcept { return argv__; }
    int argc() noexcept { return argc__; }

    const char path_sep = ':';

    environ_cache::environ_cache()
    {
        try
        {
            size_t envsize = osenv_size(environ);
            m_env.insert(m_env.end(), environ, environ + envsize + 1);
        }
        catch (const std::exception&)
        {
            //this->~environ_cache();
            std::throw_with_nested(std::runtime_error("Failed to create environment"));
        }
    }

    environ_cache::~environ_cache() noexcept
    {
        //for (auto* p : m_env) delete[] p;
    }

    auto environ_cache::find(std::string_view key) -> iterator
    {
        std::lock_guard _{ m_mtx };

        return getenvstr_sync(key);
    }

    const char* environ_cache::getvar(std::string_view key)
    {
        std::lock_guard _{ m_mtx };

        auto it = getenvstr_sync(key);
        return it != end() ? *it + key.length() + 1 : nullptr;
    }

    void environ_cache::setvar(std::string_view key, std::string_view value)
    {
        std::lock_guard _{ m_mtx };

        setenv(key.data(), value.data(), true);

        auto it = getenvstr(key);
        int offset = osenv_find_pos(key.data());

        if (it != end()) {
            *it = environ[offset];
        }
        else {
            it = m_env.insert(end(), environ[offset]);
        }
    }

    void environ_cache::rmvar(std::string_view key)
    {
        std::lock_guard _{ m_mtx };

        auto it = getenvstr(key);
        if (it != end()) {
            m_env.erase(it);
        }

        unsetenv(key.data());
    }


    auto environ_cache::getenvstr(std::string_view key) noexcept -> iterator
    {
        return std::find_if(begin(), end(), find_envstr(key));
    }

    auto environ_cache::getenvstr_sync(std::string_view key) -> iterator
    {
        auto it_cache = getenvstr(key);
        const char* envstr_os = nullptr;

        int offset = osenv_find_pos(key.data());
        if (offset != -1) {
            envstr_os = environ[offset];
        }

        if (it_cache == end() && envstr_os)
        {
            // not found in cache, found in OS env
            it_cache = m_env.insert(end(), envstr_os);
        }
        else if (it_cache != end())
        {
            if (envstr_os)
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
                m_env.erase(it_cache);
                it_cache = end();
            }
        }

        return it_cache;
    }


    int osenv_find_pos(const char* k)
    {
        auto predicate = find_envstr(k);

        for (int i = 0; environ[i]; i++)
        {
            if (predicate(environ[i])) return i;
        }

        return -1;
    }

} /* namespace impl */
