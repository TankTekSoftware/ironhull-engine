#pragma once

#include <string>
#include <vector>

#include <raylib.h>
#include <sol/sol.hpp>

#include <IronHull/entity/EntityDefinition.hpp>
#include <IronHull/map/MapFile.hpp>

namespace IronHull
{
    class EntityWorld;

    // A live entity: the properties a map authored for it, the Lua table its script runs
    // against, and the I/O wiring it was given.
    //
    // Instances share their script's table as a method table rather than copying it, so
    // behaviour is loaded once however many entities are spawned. Anything the script
    // assigns to `self` lands on the instance; anything it reads falls through to the shared
    // script table. This is why an entity script keeps per-instance state on self rather
    // than in a file-level local, which every instance would share.
    class Entity
    {
        private:
            EntityWorld* world = nullptr;
            const EntityDefinition* definition = nullptr;
            int id = 0;

            std::string targetname;

            // Engine space, already converted by the compiler.
            Vector3 origin = { 0.0f, 0.0f, 0.0f };

            // Authored (pitch, yaw, roll) in degrees, deliberately left in that convention.
            // forward() is where it becomes a direction.
            Vector3 angles = { 0.0f, 0.0f, 0.0f };

            // Compiled model holding this entity's brushes, or -1 for a point entity.
            int model = -1;

            bool spawned = false;
            bool alive = true;

            std::vector<MapConnection> connections;

            // How many times each connection has fired, so a connection with a limit can be
            // retired once it is used up. Parallel to `connections`.
            std::vector<int> fire_counts;

            // Whoever most recently sent this entity an input, for a connection aimed at
            // "!activator" - the usual way a door tells the player who opened it.
            int activator = 0;

            sol::table self;

        public:
            Entity(EntityWorld& owner, const EntityDefinition& type, int entity_id);
            ~Entity();

            Entity(const Entity&) = delete;
            Entity& operator=(const Entity&) = delete;

        public:
            // Resolves the script's declared defaults against the map's keyvalues and builds
            // the Lua instance table. Runs before spawn().
            void configure(const std::vector<MapKeyValue>& keyvalues,
                const std::vector<MapConnection>& wiring);

            // Calls the script's OnSpawn. Separate from configure() so that every entity in
            // a map exists before any of them spawns - otherwise an OnSpawn looking for
            // another entity by name would find only the ones loaded before it.
            void spawn();

            void update(float delta);

        public:
            // Sends an output along every connection wired to it. `parameter` is passed on
            // unless the connection overrides it with one of its own.
            void fire(const std::string& output, const std::string& parameter = "");

            // Calls the named input. Returns false when the entity has no such input, which
            // means a connection is aimed at something this entity cannot do.
            bool accept_input(const std::string& input, const std::string& parameter, int activator_id);

        public:
            int get_id() const;
            const EntityDefinition& get_definition() const;
            const std::string& get_classname() const;
            const std::string& get_targetname() const;

            Vector3 get_origin() const;
            void set_origin(Vector3 value);

            Vector3 get_angles() const;
            void set_angles(Vector3 value);

            // The direction this entity faces, in engine space.
            Vector3 forward() const;

            int get_model() const;
            bool is_brush_entity() const;

            bool is_alive() const;
            void destroy();

            const std::vector<MapConnection>& get_connections() const;
            const std::vector<int>& get_fire_counts() const;
            int get_activator() const;

            // Records that one of this entity's connections has fired, so a connection with
            // a firing limit can be retired once it is spent. Called by EntityWorld while
            // dispatching, which is the only thing that knows a connection was followed.
            void note_connection_fired(int connection_index);

            sol::table lua_table() const;

        private:
            // Builds the instance table and attaches the methods a script calls on itself.
            void bind_lua(const std::vector<MapKeyValue>& keyvalues);

            // Strips the engine methods back off the table when the entity goes away. A
            // script that stashed `self` somewhere then gets a clean Lua error if it calls
            // one, rather than reaching a C++ object that no longer exists.
            void detach_lua();

            // Calls a script method if it has one, reporting a Lua error against the
            // entity's classname. Returns false when there is no such method.
            bool call_method(const std::string& name, const std::vector<sol::object>& arguments = {});
    };
}
