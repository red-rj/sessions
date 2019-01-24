#include <cstdint>
#include <string>
#include <string_view>
#include <vector>


std::string get_env(const char* key);
void set_env(const char* key, const char* value);
void rm_env(const char* key);

std::vector<std::string> get_args();