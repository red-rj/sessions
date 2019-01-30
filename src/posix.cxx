#include "impl.hpp"
#include <string_view>
#include <mutex>

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

std::mutex envmtx__;

} /* nameless namespace */

extern "C" char** environ;

namespace impl {

char const* argv (std::size_t idx) noexcept { return argv__[idx]; }
char const** argv() noexcept { return argv__; }
int argc () noexcept { return argc__; }

char const** envp () noexcept { return (char const**)environ; }

int env_find(char const* key) {
  std::lock_guard _{envmtx__};
  
  std::string_view keysv = key;

  for (int i = 0; environ[i]; i++)
  {
    std::string_view elem = environ[i];
    
    if (elem.length() <= keysv.length()) continue;
    if (elem[keysv.length()] != '=') continue;
    if (elem.compare(0, keysv.length(), keysv) != 0) continue;

    return i;
  }

  return -1;
}

size_t env_size() noexcept {
  std::lock_guard _{envmtx__};

  size_t size;
  for (size = 0; environ[size]; size++);
  return size;
}

char const* get_env_var(char const* key)
{
  std::lock_guard _{envmtx__};

  return getenv(key);
}
void set_env_var(const char* key, const char* value)
{
  std::lock_guard _{envmtx__};

  setenv(key, value, true);
}
void rm_env_var(const char* key)
{
  std::lock_guard _{envmtx__};

  unsetenv(key);
}

} /* namespace impl */
