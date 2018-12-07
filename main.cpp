
#include <sol.hpp>

#include <rttr/type>
#include <rttr/registration>
#include <rttr/visitor.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "RttrSolBinder.h"

#include <iostream>
#include <math.h>

static auto console = spdlog::stdout_color_mt("console");

namespace test {

class Vec {
public:
     static void declare(sol::state& state) {
         console->info("declare test::Vec");
         state.new_usertype<Vec>("Vec",
             sol::constructors<Vec(), Vec(float, float)>(),
             "x", &Vec::x,
             "y", &Vec::y,
             "add", &Vec::add,
             "length", &Vec::length
         );
     }
    Vec() {}
    Vec(float _x, float _y): x(_x), y(_y) {}
    float x {0.0f};
    float y {0.0f};
    const Vec add(const Vec& v) const { return Vec(x + v.x, y + v.y); }
    float length() const { return sqrtf(x*x + y*y); }

    RTTR_ENABLE()
};

class Rigidbody {
public:
     static void declare(sol::state& state) {
         console->info("declare test::Rigidbody");
         state.new_usertype<Rigidbody>("Rigidbody",
             "pos", &Rigidbody::pos,
             "rot", &Rigidbody::rot
         );
     }
    Vec pos {Vec(0.0f, 0.0f)};
    Vec rot {Vec(0.0f, 0.0f)};

    RTTR_ENABLE()
};

}

using namespace test;
RTTR_REGISTRATION
{
    rttr::registration::class_<Vec>("Vec")
        .property("x", &Vec::x)
        .property("y", &Vec::y)
        .method("add", &Vec::add)
        .method("length", &Vec::length)
    ;

    rttr::registration::class_<Rigidbody>("Rigidbody")
        .property("pos", &Rigidbody::pos)
        .property("rot", &Rigidbody::rot)
        ;
}


int main() {
    auto solTypeToString = [](sol::type solType) {
        switch (solType) {
            default:
            case sol::type::none: return "none";
            case sol::type::nil: return "nil";
            case sol::type::number: return "number";
            case sol::type::boolean: return "bool";
            case sol::type::function: return "function";
            case sol::type::string: return "string";
            case sol::type::userdata: return "userdata";
            case sol::type::lightuserdata: return "lightuserdata";
            case sol::type::table: return "table";
            case sol::type::poly: return "poly";
        }
    };

    console->info("start rttr+sol+lua test...");
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    const char* showGlobal = R"(
local function showGlobalTable()
    for k, v in pairs(_G) do
        print(k)
    end
end
showGlobalTable()
    )";

    BindRttrToSol(lua);
    console->info("----------------");
    lua.script(showGlobal);

//    Vec::declare(lua);
    console->info("----------------");
    lua.script(showGlobal);


//    Rigidbody::declare(lua);

//    lua.new_usertype<Vec>("Vec");

    auto startsWith = [](const std::string &key, const std::string &prefix) -> bool {
        return (key.size() >= prefix.size()) && (key.compare(0, prefix.size(), prefix) == 0);
    };
    auto solMembers = [&](std::string&& name) {
        sol::table metaTable = lua[name][sol::metatable_key];

        std::vector<std::string> keys = {};

        for (const auto &pair : metaTable) {
            const std::string key = pair.first.as<std::string>();

            if (!startsWith(key, "__")) {
                keys.push_back(key);
                std::cout << key << ", ";
            }
            else
            {
                // The Meta Methods and stuff here
            }
        }
        return keys;
    };

    Vec vec(1.0f, 2.0f);
    lua["v"] = vec;

    lua.script("v.x = 3.0");

    solMembers("v");
    lua.script(R"( print("Hello from lua!") )");
    lua.script(R"( print("type of Vec is " .. type(Vec)) )");
    lua.script(R"( print("vec: [" .. v.x .. ", " .. v.y .. "]") )");
    lua.script(R"( print("type of Vec's instance is " .. type(v)) )");
    console->info("mod vec: [{}, {}]", vec.x, vec.y);

    // Vec luaVec = lua["v"];
    // console->info("lua vec: [{}, {}]", luaVec.x, luaVec.y);

    // Rigidbody rb;
    // rb.pos = Vec(1.0, 2.0);
    // rb.rot = Vec(30.0, 60.0);

    // // lua["rb"] = rb;
    // // sol::table 

    // console->info("mod Rigidbody pos: [{}, {}]", rb.pos.x, rb.pos.y);
    
    return 0;
}