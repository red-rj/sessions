#if !defined(RED_SESSION_ENVCACHE_HPP)
#define RED_SESSION_ENVCACHE_HPP

#include <vector>
#include <string>
#include <string_view>
#include <mutex>


namespace red::session {
    class environment;
}

namespace red::session::detail
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

        iterator begin() noexcept;
        iterator end() noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        size_t size() const noexcept;

        iterator find(std::string_view) noexcept;
        bool contains(std::string_view);

        std::string_view getvar(std::string_view);
        void setvar(std::string_view, std::string_view);
        void rmvar(std::string_view);


        iterator getenvstr(std::string_view key) noexcept;
        iterator getenvstr_sync(std::string_view key);

        std::mutex m_mtx;
        vector_t myenv;
    };

} // red::session::detail


#endif // RED_SESSION_ENVCACHE
