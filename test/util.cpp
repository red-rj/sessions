#if defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif // WIN32

#include "util.hpp"


std::string get_env(const char* key)
{
    auto* val = getenv(key);
    return val ? val : "";
}

void set_env(const char* key, const char* value)
{
#ifdef WIN32
    _putenv_s(key, value);
#else
    setenv(key, value, true);
#endif // WIN32
}

void rm_env(const char* key)
{
#ifdef WIN32
    _putenv_s(key, "");
#else
    unsetenv(key);
#endif // WIN32
}

