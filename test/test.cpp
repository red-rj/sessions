#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#if defined(WIN32)
#include "slimwindows.h"
#endif // WIN32


#include "red/session.hpp"
#include "util.hpp"
#include <iostream>
#include <array>
#include <vector>
#include <utility>


using namespace red::session;
using namespace std::literals;


auto& operator << (std::ostream& os, environment::variable const& var)
{
    return os << (std::string_view)var;
}


TEST_CASE("Environment tests", "[environment]")
{
    set_env("Phasellus", "LoremIpsumDolor");
    set_env("thug2song", "354125go");
    rm_env("nonesuch");

    environment env;

    REQUIRE(env["Phasellus"] == "LoremIpsumDolor"sv);
    REQUIRE(env["thug2song"] == "354125go"sv);

    std::array cases = {
        std::pair{"DRUAGA1"sv, "WEED"sv},
        std::pair{"PROTOCOL"sv, "DEFAULT"sv},
        std::pair{"SERVER"sv, "127.0.0.1"sv},
        std::pair{"ACENTOS"sv, u8"ÁáÉéÍíÓóÚúÇç"sv}
    };

    for(auto[key, value] : cases)
    {
        env[key] = value;
    }

    const auto env_start_l = env.size();
    CAPTURE(env_start_l);

    SECTION("finding variables using operator[]")
    {
        for(auto[key, value] : cases)
        {
            std::string var { env[key] };
            CHECK(var == value);
        }

        CHECK(env["nonesuch"] == ""sv);
    }
    SECTION("finding variables using find()")
    {
        for(auto c : cases)
        {
            auto it = env.find(c.first);
            INFO(*it);
            CHECK(it != env.end());
        }

        CHECK(env.find("nonesuch") == env.end());
    }
    SECTION("erasing variables")
    {
        env.erase("PROTOCOL");
        CHECK(env.find("PROTOCOL") == env.end());
        CHECK(env.size() == env_start_l - 1);
        auto e = get_env("PROTOCOL");
        CHECK(e.empty());
    }
    SECTION("stay in sync w/ external changes")
    {
        rm_env("thug2song");
        set_env("Phasellus", "DolorLorem");

        // find() does not sync
        CHECK_FALSE(env.find("thug2song") == env.end());

        CHECK(env["Phasellus"] == "DolorLorem"sv);
        CHECK(env["thug2song"] == ""sv);
        CHECK(env.size() == env_start_l - 1);
    }
    SECTION("contains")
    {
        for(auto c : cases)
        {
            CHECK(env.contains(c.first));
        }
        
        CHECK_FALSE(env.contains("nonesuch"));
    }
    SECTION("Path split")
    {
        auto[path_begin, path_end] = env["PATH"].split();

        for (auto it = path_begin; it != path_end; it++)
        {
            INFO(*it);
        }
    }
}

std::vector<std::string> cmdargs;

TEST_CASE("Arguments tests", "[arguments]")
{
    arguments args;
    
    REQUIRE(args.size() == cmdargs.size());

    for (size_t i = 0; i < args.size(); i++)
    {
        CAPTURE(args[i], cmdargs[i]);
        CHECK(args[i] == cmdargs[i]);
    }
}


// entry point
#if defined(WIN32)
int wmain(int argc, const wchar_t* argv[]) {
#else
int main(int argc, const char* argv[]) {
#endif // WIN32
    using namespace Catch::clara;

    for (int i = 0; i < argc; i++)
    {
#if defined(WIN32)
        std::wstring_view warg = argv[i];
        std::string arg;
        size_t arg_l = WideCharToMultiByte(CP_UTF8, 0, warg.data(), warg.size(), nullptr, 0, nullptr, nullptr);
        arg.resize(arg_l);
        WideCharToMultiByte(CP_UTF8, 0, warg.data(), warg.size(), arg.data(), arg.size(), nullptr, nullptr);

        cmdargs.push_back(arg);
#else
        cmdargs.push_back(argv[i]);
#endif // WIN32
    }

    setlocale(LC_ALL, ".UTF-8");

    Catch::Session session;

    // we already captured the arguments, but we need this so catch doesn't
    // error out due to unknown arguments
    std::string dummy;
    auto cli = session.cli()
    | Opt(dummy, "test arguments")["--args"];

    session.cli(cli);

    int rc = session.applyCommandLine(argc, argv);
    if (rc != 0)
        return rc;

    return session.run();
}