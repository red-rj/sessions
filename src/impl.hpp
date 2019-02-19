#ifndef SESSION_SRC_IMPL_HPP
#define SESSION_SRC_IMPL_HPP

#include <algorithm>
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
    using iterator = std::vector<const char*>::iterator;
    using const_iterator = std::vector<const char*>::const_iterator;

    environ_cache();
    environ_cache(environ_cache&& other)
        : m_env(std::exchange(other.m_env, {}))
    {}
    ~environ_cache() noexcept;

    environ_cache(const environ_cache&) = delete;

    iterator begin() noexcept { return m_env.begin(); }
    iterator end() noexcept { return m_env.end()-1; }

    size_t size() const noexcept { return m_env.size()-1; }

    iterator find(std::string_view);

    const char* getvar(std::string_view);
    void setvar(std::string_view, std::string_view);
    void rmvar(std::string_view);

private:

    iterator getenvstr(std::string_view key) noexcept;
    iterator getenvstr_sync(std::string_view key);

    std::mutex m_mtx;
    std::vector<const char*> m_env;
};

int osenv_find_pos(const char*);

template<class T>
inline size_t osenv_size(T** envptr) {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}

[[nodiscard]]
inline char* new_envstr(std::string_view key, std::string_view value)
{
    const auto buffer_l = key.size() + value.size() + 2;
    auto buffer = new char[buffer_l];
    key.copy(buffer, key.size());
    buffer[key.size()] = '=';
    value.copy(buffer + key.size() + 1, value.size());
    buffer[buffer_l - 1] = 0;

    return buffer;
}


} /* namespace impl */

#endif /* SESSION_SRC_IMPL_HPP */
