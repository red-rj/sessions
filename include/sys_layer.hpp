#pragma once
#include <type_traits>
#include <string>
#include <string_view>
#include <cstdlib>


#ifndef WIN32

extern "C" char** environ;

#endif // !WIN32

// system layer
namespace red::session::sys {

// env

inline auto envp() noexcept
{
#ifdef WIN32
    return _wenviron;
#else
    return environ;
#endif
}

template<class T>
inline size_t envsize(T** envptr) noexcept {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}

using env_t = decltype(envp());
using envchar = std::remove_pointer_t<std::remove_pointer_t<env_t>>;

std::string narrow(envchar const* s);

std::string getenv(std::string_view key);
void setenv(std::string_view key, std::string_view value);
void rmenv(std::string_view key);

inline constexpr char path_sep =
#ifdef WIN32
    ';';
#else
    ':';
#endif

// args

char const** argv() noexcept;
int argc() noexcept;

} // namespace red::session::sys
