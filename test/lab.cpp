#include "ixm/session.hpp"
#include <iostream>


int main(int argc, char const *argv[])
{
    ixm::session::arguments args;
    bool argc_check = args.argc() == argc;
    auto a1 = args[0];
    
    ixm::session::environment env;

    int i = 0;
    std::cout << "Args:\n";


    for (auto& a : args)
    {
        std::cout << i++ << ": " << a << '\n';
    }

    i = 0;
    std::cout << "\nEnv:\n";

    for (auto e : env)
    {
        std::cout << i++ << ": " << e << '\n';
    }

    auto usrname = env["USERNAME"];
    std::cout << '\n' << "username before: " << (std::string_view)usrname << '\n';

    usrname = "Fulano";

    std::cout << '\n' << "username after: " << (std::string_view)env["USERNAME"] << '\n';

    i = 0;

    std::cout << "env size: " << env.size() << '\n';
    
    auto invalidateIts = env.begin();

    return 0;
}
    