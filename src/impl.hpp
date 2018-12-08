#ifndef SESSION_SRC_IMPL_HPP
#define SESSION_SRC_IMPL_HPP

#include <type_traits>
#include <cstddef>

namespace impl {

char const* argv (std::size_t) noexcept;
char const** argv() noexcept;
int argc () noexcept;

char const** envp () noexcept;
void set_env_var(const char*, const char*) noexcept;
extern const char env_path_sep;

} /* namespace impl */

#endif /* SESSION_SRC_IMPL_HPP */
