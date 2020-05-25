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
using std::string_view;
using keyval_pair = std::pair<string_view, string_view>;


TEST_CASE("Environment manip.", "[environment]")
{
    sys::rmenv("nonesuch");

    environment env;

    auto cases = std::array<keyval_pair, 5>{{
        {"DRUAGA1"sv, "WEED"sv},
        {"PROTOCOL"sv, "DEFAULT"sv},
        {"SERVER"sv, "127.0.0.1"sv},
        {"Phasellus", "LoremIpsumDolor"},
        {"thug2song", "354125go"}
    }};

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
    SECTION("external changes")
    {
        sys::setenv("Horizon", "Chase");
        sys::rmenv("DRUAGA1");

        REQUIRE(env.contains("Horizon"));
        REQUIRE_FALSE(env.contains("DRUAGA1"));
    }

    // remove
    for(auto[key, val] : cases)
    {
        sys::rmenv(key);
    }
}

TEST_CASE("Environment ranges", "[environment][keyval]")
{
    environment env;

    int count = 0;
    for (auto k : env.keys() | views::take(10))
    {
        CAPTURE(k, ++count);
        REQUIRE(k.find('=') == std::string::npos);
        CHECK(env.contains(k));
    }
    count=0;
    for (auto v : env.values() | views::take(10))
    {
        CAPTURE(v, ++count);
        REQUIRE(v.find('=') == std::string::npos);
    }

}

TEST_CASE("Path Split", "[environment][pathsplit]")
{
    using std::cout; using std::quoted;

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

TEST_CASE("Environment iterator", "[environment][iterator]")
{
    environment environment;
    auto it = environment.begin();
    auto it2 = it;

    it++;
    it2++; it2++;

    REQUIRE_FALSE(it == it2);
    REQUIRE_FALSE(*it == *it2);
}

std::vector<std::string> cmdargs;

TEST_CASE("Arguments tests", "[arguments]")
{
    arguments args;
    
    REQUIRE(args.size() == cmdargs.size());

    for (size_t i = 0; i < args.size(); i++)
    {
        CAPTURE(args[i], cmdargs[i]);
        REQUIRE(args[i] == cmdargs[i]);
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
        std::string arg = sys::narrow(argv[i]);
        cmdargs.push_back(arg);
    }

    setlocale(LC_ALL, ".UTF-8");

    Catch::Session session;

    // we already captured the arguments, but we need this so catch doesn't
    // error out due to unknown arguments
    std::string dummy;
    auto cli = session.cli()
    | Opt(dummy, "arg1 arg2...")["--test-args"]("additional test arguments for session::arguments");

    session.cli(cli);

    int rc = session.applyCommandLine(argc, argv);
    if (rc != 0)
        return rc;

    return session.run();
}