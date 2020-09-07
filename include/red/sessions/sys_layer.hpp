#pragma once
#include <string>
#include <string_view>


// system layer
namespace red::session::sys {

// envionment

#ifdef WIN32
using envchar = wchar_t;
#else
using envchar = char;
#endif

using env_t = envchar**;

env_t envp() noexcept;

template<class T>
inline size_t envsize(T** envptr) noexcept {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}

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
