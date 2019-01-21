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
        char* buffer; size_t buffer_l;
        _dupenv_s(&buffer, &buffer_l, key);
        std::string val{buffer, buffer_l};
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

auto& operator << (std::ostream& os, environment::variable const& var)
{
    return os << var.operator std::string_view();
}


int main()
{
    setlocale(LC_ALL, ".UTF-8");

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
    env["ACENTOS"] = "ÁáÉéÍíÓóÚúÇç";
    
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
    std::cout << "findme rm? " << env["findme"] << "\n\n";

    print(env);

    return 0;
}

