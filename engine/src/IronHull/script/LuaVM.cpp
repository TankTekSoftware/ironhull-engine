#include <IronHull/script/LuaVM.hpp>

#include <stdexcept>
#include <vector>

#include <raylib.h>
#include <raymath.h>

#include <IronHull/io/FileSystem.hpp>
#include <IronHull/map/MapSpace.hpp>

namespace IronHull
{
    namespace
    {
        bool g_initialized = false;
    }

    LuaVM& LuaVM::get_singleton()
    {
        static LuaVM singleton;
        return singleton;
    }

    void LuaVM::init()
    {
        if (g_initialized) {
            return;
        }

        LuaVM& self = LuaVM::get_singleton();

        // No io, os or package: a script has no business reaching outside the content
        // directory, and FileSystem is the only way in or out.
        self.state.open_libraries(
            sol::lib::base,
            sol::lib::string,
            sol::lib::math,
            sol::lib::table);

        g_initialized = true;

        LuaVM::register_api();
    }

    void LuaVM::shutdown()
    {
        if (!g_initialized) {
            return;
        }

        LuaVM& self = LuaVM::get_singleton();

        // Replacing the state rather than clearing it is the only way to be sure nothing is
        // left referencing a C++ object that is about to go away.
        self.state = sol::state();
        g_initialized = false;
    }

    bool LuaVM::is_initialized()
    {
        return g_initialized;
    }

    sol::state& LuaVM::get()
    {
        if (!g_initialized) {
            LuaVM::init();
        }

        return LuaVM::get_singleton().state;
    }

    void LuaVM::register_api()
    {
        sol::state& lua = LuaVM::get();

        // Vector3 is the one engine type scripts genuinely need to handle - positions,
        // directions, the value of a "vector" property - so it is a real Lua value with
        // arithmetic rather than a table of three numbers.
        lua.new_usertype<Vector3>("Vector3",
            sol::constructors<Vector3(), Vector3(float, float, float)>(),
            "x", &Vector3::x,
            "y", &Vector3::y,
            "z", &Vector3::z,
            sol::meta_function::addition, [](const Vector3& a, const Vector3& b) { return Vector3Add(a, b); },
            sol::meta_function::subtraction, [](const Vector3& a, const Vector3& b) { return Vector3Subtract(a, b); },
            sol::meta_function::multiplication, [](const Vector3& a, float scalar) { return Vector3Scale(a, scalar); },
            sol::meta_function::unary_minus, [](const Vector3& a) { return Vector3Negate(a); },
            sol::meta_function::to_string, [](const Vector3& a) {
                return TextFormat("(%.3f, %.3f, %.3f)", a.x, a.y, a.z);
            },
            "length", [](const Vector3& a) { return Vector3Length(a); },
            "normalized", [](const Vector3& a) { return Vector3Normalize(a); },
            "dot", [](const Vector3& a, const Vector3& b) { return Vector3DotProduct(a, b); },
            "cross", [](const Vector3& a, const Vector3& b) { return Vector3CrossProduct(a, b); });

        lua.new_usertype<Color>("Color",
            sol::constructors<Color(), Color(unsigned char, unsigned char, unsigned char, unsigned char)>(),
            "r", &Color::r,
            "g", &Color::g,
            "b", &Color::b,
            "a", &Color::a);

        sol::table engine = lua.create_named_table("ironhull");

        // Script output goes through raylib's log rather than print(), so it lands in the
        // same place as everything else the engine reports and the editor console can pick
        // it up alongside engine messages.
        engine.set_function("log", [](const std::string& message) {
            TraceLog(LOG_INFO, "SCRIPT: %s", message.c_str());
        });

        engine.set_function("warn", [](const std::string& message) {
            TraceLog(LOG_WARNING, "SCRIPT: %s", message.c_str());
        });

        // Vector properties already arrive converted, and so do entity origins, so a script
        // rarely needs these. They are here for the cases nothing else can resolve: a
        // position worked out from numbers the mapper typed into separate float properties,
        // or a direction a script wants to report back in the terms the editor shows.
        engine.set_function("to_engine_space", [](Vector3 point, sol::optional<float> scale) {
            return MapSpace::to_engine(point, scale.value_or(1.0f));
        });

        engine.set_function("to_map_space", [](Vector3 point, sol::optional<float> scale) {
            return MapSpace::to_map(point, scale.value_or(1.0f));
        });

        // The facing an entity's angles describe, for a script working with an angles value
        // other than its own.
        engine.set_function("angles_to_forward", [](Vector3 angles) {
            return MapSpace::angles_to_forward(angles);
        });

        // print() would otherwise go to stdout, which is invisible in a windowed build.
        lua.set_function("print", [](sol::variadic_args arguments) {
            std::string line;

            for (sol::stack_proxy argument : arguments) {
                if (!line.empty()) {
                    line += "\t";
                }

                line += argument.as<std::string>();
            }

            TraceLog(LOG_INFO, "SCRIPT: %s", line.c_str());
        });
    }

    sol::object LuaVM::run_file(const std::string& uri)
    {
        sol::state& lua = LuaVM::get();

        std::vector<unsigned char> bytes = FileSystem::read_bytes(uri);
        std::string source(bytes.begin(), bytes.end());

        // The chunk name is what Lua puts in front of an error message, so handing it the
        // uri makes a runtime error in a script point at the file it came from.
        sol::load_result chunk = lua.load(source, uri);

        if (!chunk.valid()) {
            sol::error error = chunk;
            throw std::runtime_error("LuaVM: failed to load '" + uri + "': " + error.what());
        }

        sol::protected_function_result result = chunk.get<sol::protected_function>()();

        if (!result.valid()) {
            sol::error error = result;
            throw std::runtime_error("LuaVM: error running '" + uri + "': " + error.what());
        }

        return result;
    }

    sol::table LuaVM::run_table_file(const std::string& uri)
    {
        sol::object result = LuaVM::run_file(uri);

        if (!result.is<sol::table>()) {
            throw std::runtime_error("LuaVM: '" + uri + "' did not return a table"
                " - check that the script ends with a `return` of its definition table");
        }

        return result.as<sol::table>();
    }
}
