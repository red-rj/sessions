#ifndef SESSION_SRC_IMPL_HPP
#define SESSION_SRC_IMPL_HPP

#include <vector>
#include <string_view>
#include <mutex>

#include <cstddef>

namespace impl {

char const* argv (std::size_t) noexcept;
char const** argv() noexcept;
int argc () noexcept;

extern const char path_sep;


struct environ_cache
{
    using value_type = const char*;
    // struct iterator;
    using iterator = struct env_iterator;

    environ_cache();
    environ_cache(environ_cache&& other);
    ~environ_cache() noexcept;

    environ_cache(const environ_cache&) = delete;

    iterator begin() noexcept;
    iterator end() noexcept;

    size_t size() const noexcept;

    iterator find(std::string_view);
    bool contains(std::string_view);

    value_type getvar(std::string_view);
    void setvar(std::string_view, std::string_view);
    void rmvar(std::string_view);

private:

    iterator getenvstr(std::string_view key) noexcept;
    iterator getenvstr_sync(std::string_view key);

    std::mutex m_mtx;
    // std::vector<value_type> m_env;
};

int osenv_find_pos(const char*);

template<class T>
inline size_t osenv_size(T** envptr) {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}

// env_iterator definition
#if defined(WIN32)
struct env_iterator : public std::vector<const char*>::iterator
{
    using base_t = std::vector<const char*>::iterator;
    using base_t::base_t;
};
#elif defined(_POSIX_C_SOURCE)
#include <string>

struct env_iterator : public std::vector<std::string>::iterator
{
    using base_t = std::vector<std::string>::iterator;
    using base_t::base_t;
};
#endif // WIN32 or _POSIX_C_SOURCE


} /* namespace impl */

#endif /* SESSION_SRC_IMPL_HPP */
