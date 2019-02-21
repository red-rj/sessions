#ifndef SESSION_SRC_IMPL_HPP
#define SESSION_SRC_IMPL_HPP

#include <cstddef>


namespace ixm::session::detail {
    template<typename T>
    struct ci_char_traits;
}

namespace impl {

char const* argv (std::size_t) noexcept;
char const** argv() noexcept;
int argc () noexcept;

extern const char path_sep;


int osenv_find_pos(const char*);
    
template<class T>
inline size_t osenv_size(T** envptr) {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}



template<typename CharTraits>
struct envstr_finder_base
{
    using char_type = typename CharTraits::char_type;
    using StrView = std::basic_string_view<char_type, CharTraits>;

    StrView key;

    explicit envstr_finder_base(StrView k) : key(k) {}
    explicit envstr_finder_base(const char_type* ptr) : key(ptr) {}
    envstr_finder_base(const char_type* ptr, size_t len) : key(ptr, len) {}

    bool operator() (StrView entry) noexcept
    {
        return
            entry.length() > key.length() &&
            entry[key.length()] == '=' &&
            entry.compare(0, key.size(), key) == 0;
    }
};

template<typename T>
using envstr_finder = envstr_finder_base<std::char_traits<T>>;

template<typename T>
using ci_envstr_finder = envstr_finder_base<ixm::session::detail::ci_char_traits<T>>;

} /* namespace impl */

#endif /* SESSION_SRC_IMPL_HPP */
