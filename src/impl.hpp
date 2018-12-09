#ifndef SESSION_SRC_IMPL_HPP
#define SESSION_SRC_IMPL_HPP

#include <cstddef>

namespace impl {

char const* argv (std::size_t) noexcept;
char const** argv() noexcept;
int argc () noexcept;

char const** envp () noexcept;
size_t env_size();
char const* get_env_var(char const*) noexcept;
void set_env_var(const char*, const char*) noexcept;
void rm_env_var(const char*) noexcept;
extern const char env_path_sep;

} /* namespace impl */

#endif /* SESSION_SRC_IMPL_HPP */
