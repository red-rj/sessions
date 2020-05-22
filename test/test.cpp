#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#if defined(WIN32)
#include "win32.hpp"
#endif // WIN32

#include <iostream>
#include <array>
#include <vector>
#include <utility>

#include "session.hpp"
#include "sys_layer.hpp"
#include "range/v3/view.hpp"
#include <range/v3/view/split.hpp>
#include <range/v3/range/conversion.hpp>

using namespace red::session;
using namespace std::literals;
using namespace ranges;


TEST_CASE("Environment tests", "[environment]")
{
    sys::setenv("Phasellus", "LoremIpsumDolor");
    sys::setenv("thug2song", "354125go");
    sys::rmenv("nonesuch");

    environment env;

    REQUIRE(env["Phasellus"] == "LoremIpsumDolor"sv);
    REQUIRE(env["thug2song"] == "354125go"sv);

    std::array cases = {
        std::pair{"DRUAGA1"sv, "WEED"sv},
        std::pair{"PROTOCOL"sv, "DEFAULT"sv},
        std::pair{"SERVER"sv, "127.0.0.1"sv},
        // std::pair{"ACENTOS"sv, u8"ÁáÉéÍíÓóÚúÇç"sv}
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
            REQUIRE(var == value);
        }

        REQUIRE(env["nonesuch"] == ""sv);
    }
    SECTION("finding variables using find()")
    {
        for(auto c : cases)
        {
            auto it = env.find(c.first);
            REQUIRE(it != env.end());
        }

        REQUIRE(env.find("nonesuch") == env.end());
    }
    SECTION("erasing variables")
    {
        env.erase("PROTOCOL");
        CHECK(env.size() == env_start_l - 1);
        auto e = sys::getenv("PROTOCOL");
        REQUIRE(e.empty());
        REQUIRE(env.find("PROTOCOL") == env.end());
    }
    SECTION("contains")
    {
        for(auto c : cases)
        {
            REQUIRE(env.contains(c.first));
        }
        
        REQUIRE_FALSE(env.contains("nonesuch"));
    }
    SECTION("validate ranges")
    {
        int count = 0;
        for (std::string k : env.keys() | views::take(10))
        {
            CAPTURE(k, ++count);
            REQUIRE(k.find('=') == std::string::npos);
            REQUIRE(env.contains(k));
        }
        for (std::string v : env.values() | views::take(10))
        {
            CAPTURE(v);
            REQUIRE(v.find('=') == std::string::npos);
        }
        
    }
}

TEST_CASE("Path Split", "[pathsplit]")
{
    using std::cout; using std::quoted;
    using path_iterator = environment::variable::path_iterator;

    environment environment;
    
    auto pathsplit = environment["PATH"].split();
    
    int count=0;
    CAPTURE(sys::path_sep, count);
    for (auto it = pathsplit.begin(); it != pathsplit.end(); it++, count++)
    {
        auto current = ranges::to<std::string>(*it);
        CAPTURE(current);
        REQUIRE(current.find(sys::path_sep) == std::string::npos);
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