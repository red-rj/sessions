#include "ixm/session.hpp"
#include <iostream>


int main()
{
    ixm::session::arguments args;
    ixm::session::environment env;

    int i = 0;
    std::cout << "Args:\n";

    for (auto& a : args)
    {
        std::cout << i++ << ": " << a << '\n';
    }

    i = 0;
    std::cout << "\n" "Env (size: " << env.size() << "):\n";

    for (auto e : env)
    {
        std::cout << i++ << ": " << e << '\n';
    }

    auto usrname = env["USERNAME"];
    std::cout << '\n' << "username before: " << (std::string_view)usrname << '\n';

    usrname = "Fulano";

    std::cout << '\n' << "username after: " << (std::string_view)env["USERNAME"] << '\n';

    auto memeIt = env.find("HAVE_PHONES");

    auto[path_begin, path_end] = env["PATH"].split();

    puts("\n" "PATH split:");
    for (auto it = path_begin; it != path_end; it++)
    {
        std::cout << *it << '\n';
    }

    env["DRUAGA1"] = "WEED";
    

    return 0;
}
