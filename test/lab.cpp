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
    std::cout << "\nEnv (size: " << env.size() << "):\n";

    for (auto e : env)
    {
        std::cout << i++ << ": " << e << '\n';
    }

    auto usrname = env["USERNAME"];
    std::cout << '\n' << "username before: " << (std::string_view)usrname << '\n';

    usrname = "Fulano";

    std::cout << '\n' << "username after: " << (std::string_view)env["USERNAME"] << '\n';

    auto memeIt = env.find("HAVE_PHONES");

    env.erase("HAVE_PHONES");

    std::string_view path = env["PATH"];

    ixm::session::detail::pathsep_iterator<char> pi_begin{';', path};
    pi_begin++;
    pi_begin++;
    pi_begin++;


    ;

    return 0;
}
