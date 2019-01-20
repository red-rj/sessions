#include "ixm/session.hpp"
#include <iostream>

using namespace ixm::session;

namespace
{
    void print(environment const& env)
    {
        int i = 0;
        std::cout << "Environment (size: " << env.size() << "):\n";
        for (auto e : env)
        {
            std::cout << ++i << ": " << e << '\n';
        }
    }

    void print(arguments const& args)
    {
        int i = 0;
        std::cout << "Arguments (size: " << args.size() << "):" "\n";
        for (auto a : args)
        {
            std::cout << ++i << ": " << a << '\n';
        }
    }

    std::string get_env(const char* key)
    {
#ifdef WIN32
        char* buffer;
        _dupenv_s(&buffer, nullptr, key);
        std::string val = buffer;
        ::free(buffer);
        return val;
#else
        return getenv(key);
#endif // WIN32
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

}

int main()
{
    arguments args;
    environment env;

    print(args);
    std::cout << '\n';

    auto[path_begin, path_end] = env["PATH"].split();

    puts("\n" "PATH split:");
    for (auto it = path_begin; it != path_end; it++)
    {
        std::cout << *it << '\n';
    }

    // setting some new variables
    env["DRUAGA1"] = "WEED";
    env["PROTOCOL"] = "DEFAULT";
    env["ERASE"] = "ME1234";
    
    // setting new variables outside the class
    set_env("thug2song", "354125go");
    set_env("findme", "asddsa");


    std::string_view song = env["thug2song"];
    std::cout << "\n" "song var: " << song << "\n\n";

    // finding
    auto it1 = env.find("findme"), it2 = env.find("thug2song"), it3 = env.find("nonesuch");

    // erasing
    env.erase("ERASE");
    env.erase("nonesuch");

    set_env("thug2song", "123456789");
    it2 = env.find("thug2song");

    rm_env("findme");
    std::cout << "findme rm? " << (std::string_view)env["findme"] << '\n';

    return 0;
}

