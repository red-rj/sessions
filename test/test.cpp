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

class test_vars_guard
{
public:
    test_vars_guard() {
        for(auto[key, value] : TEST_VARS) {
            sys::setenv(key, value);
        }
        sys::rmenv("nonesuch");
    }
    ~test_vars_guard() {
        for(auto[key, _] : TEST_VARS) {
            sys::rmenv(key);
        }
    }
};


static red::session::environment environment;
static red::session::arguments arguments;

//---

TEST_CASE("get environment variables", "[environment]")
{
    test_vars_guard _;

    REQUIRE_FALSE(environment.empty());

    SECTION("operator[]")
    {
        for(auto[key, value] : TEST_VARS)
        {
            REQUIRE(string(environment[key]) == value);
        }

        REQUIRE(string(environment["nonesuch"]).empty());
    }
    SECTION("find()")
    {
        for(auto c : TEST_VARS)
        {
            auto it = environment.find(c.first);
            REQUIRE(it != environment.end());
        }

        REQUIRE(environment.find("nonesuch") == environment.end());
    }
    SECTION("contains()")
    {
        for(auto c : TEST_VARS)
        {
            REQUIRE(environment.contains(c.first));
        }
        
        REQUIRE_FALSE(environment.contains("nonesuch"));
    }
    SECTION("from external changes")
    {
        sys::setenv("Horizon", "Chase");
        sys::rmenv("DRUAGA1");

        REQUIRE(environment.contains("Horizon"));
        REQUIRE_FALSE(environment.contains("DRUAGA1"));
    }
}

TEST_CASE("set environment variables", "[environment]")
{
    for(auto[key, value] : TEST_VARS)
    {
        environment[key] = value;
        CHECK(environment.contains(key));
        REQUIRE_FALSE(sys::getenv(key).empty());
    }

    REQUIRE(std::string(environment["nonesuch"]).empty());
}

TEST_CASE("remove environment variables", "[environment]")
{
    test_vars_guard _;
    auto const env_size = environment.size();

    environment.erase("PROTOCOL");
    CHECK(environment.size() == env_size - 1);
    REQUIRE_FALSE(environment.contains("PROTOCOL"));
    REQUIRE(environment.find("PROTOCOL") == environment.end());
}

TEST_CASE("environment iteration", "[environment]")
{
    using namespace ranges;

    SECTION("Key/Value ranges")
    {
        for (auto k : environment.keys() | views::take(10))
        {
            CAPTURE(k);
            REQUIRE(k.find('=') == std::string::npos);
            REQUIRE(environment.contains(k));
        }
        for (auto v : environment.values() | views::take(10))
        {
            CAPTURE(v);
            REQUIRE(v.find('=') == std::string::npos);
        }
    }
    SECTION("iterator")
    {
        auto begin = environment.begin();
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
    SECTION("Path Split")
    {
        auto pathsplit = environment["PATH"].split();
        
        int count=0;
        CAPTURE(environment.path_separator, count);
        for (auto it = pathsplit.begin(); it != pathsplit.end(); it++, count++)
        {
            auto current = ranges::to<std::string>(*it);
            CAPTURE(current);
            REQUIRE(current.find(environment.path_separator) == std::string::npos);
        }
    }

}

TEST_CASE("use environment like a range","[environment][range]")
{
    test_vars_guard _g_;

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
    using red::session::join_paths;

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

// don't judge me, working w/ ranges is hard D:
#include <typeinfo>

TEST_CASE("wtftype", "[.]")
{
    std::cout << "environment::values() -> " << typeid(environment.values()).name() << '\n';
    std::cout << "environment::keys() -> " << typeid(environment.keys()).name() << '\n';
}

std::vector<std::string> cmdargs;

TEST_CASE("Arguments")
{
    REQUIRE(arguments.size() == cmdargs.size());

    for (size_t i = 0; i < arguments.size(); i++)
    {
        REQUIRE(arguments[i] == cmdargs[i]);
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
    auto real_args = views::take_while(subrange(argv, argv+argc), 
        [](char_t const* arg) { return arg != T("--"sv); }) | to<std::vector<char_t const*>>();

    int rc = session.applyCommandLine((int)real_args.size(), real_args.data());
    if (rc != 0)
        return rc;

    return session.run();
}