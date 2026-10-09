#pragma once

#include <string>

#include <sol/sol.hpp>

namespace IronHull
{
    // The single Lua state the engine runs scripts in, and the engine API they can reach.
    //
    // Scripts are loaded through FileSystem rather than Lua's own file handling, so an
    // entity script lives in `content://` like any other asset and keeps working once the
    // content directory has been packed into an archive.
    //
    // The state is deliberately sandboxed: the base, string, math and table libraries are
    // open, but io, os and package are not. A script cannot reach the disk, run a program,
    // or `require` another file - all three would be ways for content to escape the content
    // directory, and nothing an entity script legitimately needs to do calls for them.
    class LuaVM
    {
        private:
            sol::state state;

        private:
            static LuaVM& get_singleton();

        private:
            LuaVM() = default;
            ~LuaVM() = default;
            LuaVM(const LuaVM&) = delete;
            LuaVM& operator=(const LuaVM&) = delete;

        public:
            // Opens the sandboxed libraries and registers the engine API. Safe to call more
            // than once; subsequent calls do nothing.
            static void init();

            // Releases every script, definition and instance table held by the state. The
            // entity registry has to be cleared before this, or its definitions will outlive
            // the state they came from.
            static void shutdown();

            static bool is_initialized();
            static sol::state& get();

        public:
            // Runs a script and hands back whatever it returned. Throws std::runtime_error
            // with the Lua error message - including the script name and line - for a script
            // that fails to load or raises while running.
            static sol::object run_file(const std::string& uri);

            // The same, for a script expected to return a table describing something (an
            // entity definition). Throws if it returned anything else, which is the mistake
            // of leaving the `return` off the bottom of the file.
            static sol::table run_table_file(const std::string& uri);

        private:
            // Exposes Vector3 and Color as Lua values, plus the `ironhull` table holding
            // log() and warn(). Entity instance methods are bound per-entity instead, in
            // Entity::bind_lua().
            static void register_api();
    };
}
