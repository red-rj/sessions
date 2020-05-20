// implementation helpers
#pragma once
#include <string>
#include <string_view>

namespace sys
{
// system layer
using std::string;
using std::string_view;

char const** argv() noexcept;
int argc() noexcept;
extern const char path_sep;

// env
string getenv(string_view key);
void setenv(string_view key, string_view value);
void rmenv(string_view key);

template<class T>
inline size_t envsize(T** envptr) {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}

} // namespace sys
