#ifndef SESSION_SRC_IMPL_HPP
#define SESSION_SRC_IMPL_HPP

#include <cstddef>

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

} /* namespace impl */

#endif /* SESSION_SRC_IMPL_HPP */
