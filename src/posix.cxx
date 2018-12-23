#include "impl.hpp"
#include <cstdlib>

#if defined(__ELF__) and __ELF__
  #define SESSION_IMPL_SECTION ".init_array"
  #define SESSION_IMPL_ELF 1
#elif defined(__MACH__) and __MACH__
  #define SESSION_IMPL_SECTION "__DATA,__mod_init_func"
  #define SESSION_IMPL_ELF 0
#endif

namespace {

char const** argv__ { };
int argc__ { };

[[gnu::section(SESSION_IMPL_SECTION)]]
auto init = +[] (int argc, char const** argv, char const**) {
  argv__ = argv;
  argc__ = argc;
  if constexpr(SESSION_IMPL_ELF) { return 0; }
};

} /* nameless namespace */

extern "C" char** environ;

namespace impl {

char const* argv (std::size_t idx) noexcept { return argv__[idx]; }
char const** argv() noexcept { return argv__; }
int argc () noexcept { return argc__; }

char const** envp () noexcept { return (char const**)environ; }
size_t env_size() noexcept {
    size_t environ_size = 0;
    while (environ[environ_size]) environ_size++;
    return environ_size;
}

}

char const* get_env_var(char const* key) noexcept
{
  return getenv(key);
}
void set_env_var(const char* key, const char* value) noexcept
{
  setenv(key, value, 42);
}
void rm_env_var(const char* key) noexcept
{
  unsetenv(key);
}

} /* namespace impl */
