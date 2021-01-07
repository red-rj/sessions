#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <iostream>
#include <array>
#include <vector>
#include <utility>
#include <typeinfo>

#include <range/v3/view.hpp>
#include <range/v3/action.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/algorithm.hpp>

#include "red/sessions/session.hpp"

using namespace std::literals;

namespace sys
{
    std::string getenv(std::string_view key);
    void setenv(std::string_view key, std::string_view value);
    void rmenv(std::string_view key);
}

using std::string;
using std::string_view;
using keyval_pair = std::pair<string_view, string_view>;

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

TEST_CASE("get environment variables", "[env]")
{
    test_vars_guard _;

    REQUIRE_FALSE(environment.empty());

    SECTION("operator[]")
    {
        for(auto[key, value] : TEST_VARS)
        {
            REQUIRE(environment[key].value() == value);
        }

        REQUIRE(environment["nonesuch"].value().empty());
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

TEST_CASE("set environment variables", "[env]")
{
    for(auto[key, value] : TEST_VARS)
    {
        environment[key] = value;
        CHECK(environment.contains(key));
        REQUIRE_FALSE(sys::getenv(key).empty());
    }

    REQUIRE(environment["nonesuch"].value().empty());
}

TEST_CASE("remove environment variables", "[env]")
{
    test_vars_guard _;
    auto const env_size = environment.size();

    environment.erase("PROTOCOL");
    CHECK(environment.size() == env_size - 1);
    REQUIRE_FALSE(environment.contains("PROTOCOL"));
    REQUIRE(environment.find("PROTOCOL") == environment.end());
}

TEST_CASE("environment iteration", "[env]")
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

TEST_CASE("environment::variable", "[var]")
{
    using ranges::to;
    auto valid_char = [](unsigned char ch) { return isprint(ch); };
    
    SECTION("Path Split")
    {
        auto path = environment["PATH"];
        auto pathsplit = path.split();

        for (auto p : pathsplit)
        {
            CAPTURE(to<string>(p));
            REQUIRE(ranges::all_of(p, valid_char));
            REQUIRE(ranges::find(p, environment.path_separator) == p.end());
        }
    }
    SECTION("Custom split sep")
    {
        auto var = environment["mysplitvar"] = "values.separated.by.dots";
        auto split = var.split('.');
        for (auto p : split)
        {
            CAPTURE(to<string>(p));
            REQUIRE(ranges::all_of(p, valid_char));
            REQUIRE(ranges::find(p, environment.path_separator) == p.end());
        }
    }
}

TEST_CASE("use environment like a range", "[env][range]")
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

    auto elems = std::array {
        "path"sv, "dir"sv, "folder"sv, "location"sv
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
TEST_CASE("wtftype", "[.]")
{
    std::cout << "environment::values() -> " << typeid(environment.values()).name() << '\n';
    std::cout << "environment::keys() -> " << typeid(environment.keys()).name() << '\n';

    auto pathsplit = environment["PATH"].split();
    std::cout << "splitpath value_type -> " << typeid(*pathsplit.begin()).name() << '\n';
}

std::vector<std::string> cmdargs;

TEST_CASE("Arguments", "[args]")
{
    REQUIRE(arguments.size() == cmdargs.size());

    for (size_t i = 0; i < arguments.size(); i++)
    {
        REQUIRE(arguments[i] == cmdargs[i]);
    }
}


using red::session::detail::envchar;

// on windows, use wmain for unicode arguments
#if defined(WIN32)
#define main wmain
#endif


int main(int argc, const envchar* argv[]) {
    using namespace ranges;
    using views::transform; using views::take_while;
    using std::vector;
    setlocale(LC_ALL, "");

    // copy arguments as narrow strings, for testing session::arguments
    cmdargs = subrange(argv, argv+argc) | transform(red::session::detail::narrow_copy) | to_vector;

    Catch::Session session;

    // support passing additional args
    const auto eoa = "--"s;
    string dummy;
    auto cli = session.cli() | Catch::clara::Opt(dummy, "arg1 arg2 ...")[eoa]("additional values, for testing session::arguments");
    session.cli(cli);

    vector<const char*> arg_ptrs = cmdargs 
        | take_while([&](const string& arg) { return arg != eoa; }) 
        | transform([](const string& arg){ return arg.data(); })
        | to_vector;

    int rc = session.applyCommandLine((int)arg_ptrs.size(), arg_ptrs.data());
    if (rc == 0)
        return session.run();
    else
        return rc;
}