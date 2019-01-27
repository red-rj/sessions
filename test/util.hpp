#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>


std::string get_env(const char* key);
void set_env(const char* key, const char* value);
void rm_env(const char* key);
