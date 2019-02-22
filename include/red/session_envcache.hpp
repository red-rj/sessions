#if !defined(RED_SESSION_ENVCACHE_HPP)
#define RED_SESSION_ENVCACHE_HPP

#include <vector>
#include <string>
#include <string_view>
#include <mutex>


namespace red::session
{
    class environment;

namespace detail
{
    class environ_cache
    {
        friend environment;

#if defined(WIN32)
        using vector_t = std::vector<const char*>;
#elif defined(_POSIX_C_SOURCE)
        using vector_t = std::vector<std::string>;
#endif // WIN32

        using iterator = vector_t::iterator;
        using const_iterator = vector_t::const_iterator;

        environ_cache();
        ~environ_cache() noexcept;

        environ_cache(const environ_cache&) = delete;

        // locking methods
        
        // cache and os
        std::string_view getvar(std::string_view);
        void setvar(std::string_view, std::string_view);
        void rmvar(std::string_view);

        // cache only
        iterator find(std::string_view) noexcept;
        bool contains(std::string_view) const;


        // non locking methods

        // cache only
        iterator getenvstr(std::string_view) noexcept;

        // cache and os
        iterator getenvstr_sync(std::string_view);

        // members
        std::mutex m_mtx;
        vector_t myenv;
    };

    inline auto environ_cache::find(std::string_view key) noexcept -> iterator
    {
        std::lock_guard _{ m_mtx };
        return getenvstr(key);
    }
} // detail

} // red::session

#endif // RED_SESSION_ENVCACHE
