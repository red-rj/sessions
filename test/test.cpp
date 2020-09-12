#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#if defined(WIN32)
#include "../src/win32.hpp"
#endif // WIN32

#include <iostream>
#include <array>
#include <vector>
#include <utility>

#include <range/v3/view.hpp>
#include <range/v3/action.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/algorithm.hpp>

#include "red/sessions/session.hpp"

using namespace red::session;
using namespace std::literals;
namespace sys = red::session::sys;

using std::string;
using std::string_view;
using keyval_pair = std::pair<string_view, string_view>;
template <size_t N> using sv_array = std::array<string_view, N>;

auto constexpr TEST_VARS = std::array<keyval_pair, 5>{{
    {"DRUAGA1", "WEED"},
    {"PROTOCOL", "DEFAULT"},
    {"SERVER", "127.0.0.1"},
    {"Phasellus", "LoremIpsumDolor"},
    {"thug2song", "354125go"}
}};

void push_test_vars() {
    for(auto[key, value] : TEST_VARS) {
        sys::setenv(key, value);
    }
    sys::rmenv("nonesuch");
}
void pop_test_vars() {
    for(auto[key, _] : TEST_VARS) {
        sys::rmenv(key);
    }
}

class test_vars_guard
{
public:
    test_vars_guard() {
        push_test_vars();
    }
    ~test_vars_guard() {
        pop_test_vars();
    }
};


//---

TEST_CASE("get environment variables", "[environment]")
{
    test_vars_guard _;
    environment env;

    SECTION("operator[]")
    {
        for(auto[key, value] : TEST_VARS)
        {
            REQUIRE(string(env[key]) == value);
        }

        REQUIRE(std::string(env["nonesuch"]).empty());
    }
    SECTION("find()")
    {
        for(auto c : TEST_VARS)
        {
            auto it = env.find(c.first);
            REQUIRE(it != env.end());
        }

        REQUIRE(env.find("nonesuch") == env.end());
    }
    SECTION("contains()")
    {
        for(auto c : TEST_VARS)
        {
            REQUIRE(env.contains(c.first));
        }
        
        REQUIRE_FALSE(env.contains("nonesuch"));
    }
    SECTION("from external changes")
    {
        sys::setenv("Horizon", "Chase");
        sys::rmenv("DRUAGA1");

        REQUIRE(env.contains("Horizon"));
        REQUIRE_FALSE(env.contains("DRUAGA1"));
    }
}

TEST_CASE("set environment variables", "[environment]")
{
    environment env;

    for(auto[key, value] : TEST_VARS)
    {
        env[key] = value;
        CHECK(env.contains(key));
        REQUIRE_FALSE(sys::getenv(key).empty());
    }

    REQUIRE(std::string(env["nonesuch"]).empty());
}

TEST_CASE("remove environment variables", "[environment]")
{
    test_vars_guard _;
    environment env;
    auto const env_size = env.size();

    env.erase("PROTOCOL");
    CHECK(env.size() == env_size - 1);
    REQUIRE_FALSE(env.contains("PROTOCOL"));
    REQUIRE(env.find("PROTOCOL") == env.end());
}

TEST_CASE("environment iteration", "[environment]")
{
    using namespace ranges;
    environment env;

    SECTION("Key/Value ranges")
    {
        for (auto k : env.keys() | views::take(10))
        {
            CAPTURE(k);
            REQUIRE(k.find('=') == std::string::npos);
            REQUIRE(env.contains(k));
        }
        for (auto v : env.values() | views::take(10))
        {
            CAPTURE(v);
            REQUIRE(v.find('=') == std::string::npos);
        }
    }
    SECTION("iterator")
    {
        auto begin = env.begin();
        auto it1 = begin; auto it2 = begin;

        it1++; it2++;
        REQUIRE(it1 == it2);
        CHECK(*it1 == *it2);
        REQUIRE(string(*it1) == string(*it2));

        it1++; ranges::advance(it2, 5);
        REQUIRE(it1 != it2);
        CHECK(*it1 != *it2);
        REQUIRE(string(*it1) != string(*it2));
    }

}

TEST_CASE("environment::variable", "[environment]")
{
    environment environment;

    SECTION("Path Split")
    {
        auto pathsplit = environment["PATH"].split();
        
        int count=0;
        CAPTURE(environment::path_separator, count);
        for (auto it = pathsplit.begin(); it != pathsplit.end(); it++, count++)
        {
            auto current = ranges::to<std::string>(*it);
            CAPTURE(current);
            REQUIRE(current.find(environment::path_separator) == std::string::npos);
        }
    }

}

TEST_CASE("use environment like a range","[environment][range]")
{
    test_vars_guard _g_;

    environment environment;
    auto dist = ranges::distance(environment);
    REQUIRE(dist == environment.size());

    auto const envline = string(TEST_VARS[0].first) + "="s + string(TEST_VARS[0].second);
    auto range_it = ranges::find(environment, envline);
    auto env_it = environment.find(TEST_VARS[0].first);

    REQUIRE(range_it == env_it);
    REQUIRE(ranges::distance(environment.begin(), range_it) == ranges::distance(environment.begin(), env_it));
}

TEST_CASE("join_paths")
{
    auto elems = sv_array<5> {
        "path", "dir", "folder", "location"
    };
    auto constexpr joinend_elems = "path;dir;folder;location"sv;
    string_view expected = joinend_elems;

    string result = join_paths(elems, ';');
    REQUIRE(result == expected);

    result = join_paths(elems.begin(), elems.end(), ';');
    REQUIRE(result == expected);

    {
        expected = "path;dir;folder;location;some;thing;else";
        auto elems = {joinend_elems, "some;thing;else"sv};
        result = join_paths(elems,';');
        REQUIRE(result == expected);
    }
}


std::vector<std::string> cmdargs;

TEST_CASE("Arguments")
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
#define T(txt) L ## txt
int wmain(int argc, const wchar_t* argv[]) {
    using char_t = wchar_t;
#else
#define T(txt) txt
int main(int argc, const char* argv[]) {
    using char_t = char;
#endif // WIN32
    using namespace Catch::clara;
    
    setlocale(LC_ALL, "");

    for (int i = 0; i < argc; i++)
    {
        std::string arg = sys::narrow(argv[i]);
        cmdargs.push_back(arg);
    }

    Catch::Session session;

    // fake support for '--'
    string dummy;
    auto cli = session.cli()
    | Opt(dummy, "arg1 arg2 ...")["--"]("pass more values to session::arguments tests");

    session.cli(cli);

    // remove args past '--'
    using namespace ranges;
    std::vector<char_t const*> argvec{argv, argv+argc};
    argvec |= actions::take_while([](char_t const* arg) { return arg != T("--"sv); });

    int rc = session.applyCommandLine((int)argvec.size(), argvec.data());
    if (rc != 0)
        return rc;

    return session.run();
}