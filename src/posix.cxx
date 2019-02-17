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

    std::mutex envmtx__;

    struct find_key_pred
    {
        std::string_view key;

        explicit find_key_pred(std::string_view k) : key(k) {}

        bool operator() (std::string_view entry) const noexcept
        {
            return
                entry.length() > key.length() &&
                entry[key.length()] == '=' &&
                entry.compare(0, key.size(), key) == 0;
        }
    };

    auto& env_cache()
    {
        static auto envcache = [] {
            std::vector<std::string> vec;

            size_t envsize = 0;
            for (; environ[envsize]; envsize++);

            vec.insert(vec.end(), environ, environ + envsize);

            return vec;
        }();
        return envcache;
    }

    int env_find_offset(std::string_view key)
    {
        auto pred = find_key_pred{ key };

        for (int i = 0; environ[i]; i++)
        {
            if (pred(environ[i]))
                return i;
        }

        return -1;
    }

    void env_sync_key(std::string_view key)
    {
        auto it_cache = std::find_if(env_cache().begin(), env_cache().end(), find_key_pred{ key });

        const int offs = env_find_offset(key);
        char** it_os = offs != -1 ? environ + offs : nullptr;

        if (it_cache == env_cache().end() && it_os)
        {
            env_cache().push_back(*it_os);
        }
        else if (it_cache != env_cache().end())
        {
            if (it_os)
            {
                std::string_view var_cache = *it_cache, var_os = *it_os;

                auto const offset = key.size() + 1;
                var_cache.remove_prefix(offset);
                var_os.remove_prefix(offset);

                if (var_cache != var_os) {
                    *it_cache = *it_os;
                }
            }
            else
            {
                env_cache().erase(it_cache);
            }
        }
    }

} /* nameless namespace */


namespace impl {

    char const* argv(std::size_t idx) noexcept { return argv__[idx]; }
    char const** argv() noexcept { return argv__; }
    int argc() noexcept { return argc__; }

    char const** envp() noexcept { return (char const**)environ; }

    int env_find(char const* key) {
        std::lock_guard _{ envmtx__ };
        std::string_view keysv = key;

        env_sync_key(keysv);
        return env_find_offset(keysv);
    }

    size_t env_size() noexcept {
        return env_cache().size();
    }

    char const* get_env_var(char const* key)
    {
        std::lock_guard _{ envmtx__ };
        env_sync_key(key);

        return getenv(key);
    }
    void set_env_var(const char* key, const char* value)
    {
        std::lock_guard _{ envmtx__ };

        auto it_cache = std::find_if(env_cache().begin(), env_cache().end(), find_key_pred{ key });

        if (it_cache == env_cache().end())
        {
            env_cache().push_back(key + "="s + value);
        }
        else
        {
            auto& envstr = *it_cache;
            envstr.replace(strlen(key) + 1, envstr.size(), value);
        }

        setenv(key, value, true);
    }
    void rm_env_var(const char* key)
    {
        std::lock_guard _{ envmtx__ };

        auto it_cache = std::find_if(env_cache().begin(), env_cache().end(), find_key_pred{ key });
        if (it_cache != env_cache().end())
        {
            env_cache().erase(it_cache);
        }

        unsetenv(key);
    }

    const char path_sep = ':';

} /* namespace impl */
