#include "ixm/session.hpp"
#include <iostream>

using namespace ixm::session;

void print(environment const&);
void print(arguments const&);

int main()
{
    arguments args;
    environment env;

    print(args);
    std::cout << '\n';
    print(env);

    auto memeIt = env.find("HAVE_PHONES");
    if (memeIt != env.end())
        std::cout << "memeIt: " << *memeIt << '\n';

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
    
    // TODO: what should happend if an external source mutates the environment?
#ifdef WIN32
    _wputenv_s(L"thug2song", L"354125go");
    // won't be found on windows
    std::string_view song = env["thug2song"];

    std::cout << "\n" "song var (getenv): " << getenv("thug2song") << '\n';
    std::cout << "\n" "song var (environment before): " << song;

    // but it will be if env is recreated;
    env = {};

    song = env["thug2song"];

    std::cout << "\n" "song var (environment after): " << song << "\n\n";

#else
    setenv("thug2song", "354125go", true);
#endif // WIN32

    // erasing
    env.erase("ERASE");

    print(env);

    return 0;
}

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