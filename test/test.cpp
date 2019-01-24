#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "ixm/session.hpp"
#include "util.hpp"
#include <iostream>
#include <array>
#include <utility>


using namespace ixm::session;
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
        std::pair{"SERVER"sv, "127.0.0.1"sv}
    };

    for(auto[key, value] : cases)
    {
        env[key] = value;
    }

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
        auto e = get_env("PROTOCOL");
        CHECK(e.empty());
    }
    SECTION("stay in sync w/ external changes")
    {
        rm_env("thug2song");
        set_env("Phasellus", "DolorLorem");

        CHECK(env.find("thug2song") == env.end());
        CHECK(env["Phasellus"] == "DolorLorem"sv);
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