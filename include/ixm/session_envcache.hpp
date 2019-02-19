#if !defined(IXM_SESSION_ENVCACHE_HPP)
#define IXM_SESSION_ENVCACHE_HPP

#include <vector>
#include <string>
#include <string_view>
#include <mutex>

namespace ixm::session::detail
{
    struct environ_cache
    {
#if defined(WIN32)
        using vector_t = std::vector<const char*>;
#elif defined(_POSIX_C_SOURCE)
        using vector_t = std::vector<std::string>;
#endif // WIN32

        using iterator = vector_t::iterator;

        environ_cache();
        environ_cache(environ_cache&& other);
        ~environ_cache() noexcept;

        environ_cache(const environ_cache&) = delete;

        iterator begin() noexcept;
        iterator end() noexcept;

        size_t size() const noexcept;

        iterator find(std::string_view);
        bool contains(std::string_view);

        const char* getvar(std::string_view);
        void setvar(std::string_view, std::string_view);
        void rmvar(std::string_view);

    private:

        iterator getenvstr(std::string_view key) noexcept;
        iterator getenvstr_sync(std::string_view key);

        std::mutex m_mtx;
        vector_t myenv;
    };

} // ixm::session::detail


#endif // IXM_SESSION_ENVCACHE
