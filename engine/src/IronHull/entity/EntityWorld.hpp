#pragma once

#include <memory>
#include <string>
#include <vector>

#include <IronHull/entity/Entity.hpp>
#include <IronHull/map/CompiledMap.hpp>

namespace IronHull
{
    // The live entities of one loaded map, and the machinery that carries outputs to inputs.
    //
    // Map I/O is how a level is wired together without writing code: a mapper connects a
    // trigger's OnTouch to a door's Open, and this is what makes that connection happen. An
    // entity fires an output; every connection the mapper drew from that output is looked up,
    // its target found by name, and the input delivered - after a delay, if the connection
    // asked for one.
    //
    // Inputs are always queued rather than called straight through, even with no delay. Two
    // entities wired into each other would otherwise recurse until the stack ran out, and a
    // queue turns that into a loop this class can put a ceiling on.
    class EntityWorld
    {
        private:
            struct PendingInput
            {
                int target = 0;
                int activator = 0;
                std::string input;
                std::string parameter;
                float remaining = 0.0f;
            };

        private:
            // Entities are held by pointer because their Lua tables and the I/O queue refer
            // to them, and the vector must be free to grow without those references moving.
            std::vector<std::unique_ptr<Entity>> entities;
            std::vector<PendingInput> pending;

            // Ids start at 1 so that 0 can mean "nothing", which is what an output fired by
            // the engine itself rather than by another entity carries as its activator.
            int next_id = 1;

        public:
            EntityWorld() = default;
            ~EntityWorld();

            EntityWorld(const EntityWorld&) = delete;
            EntityWorld& operator=(const EntityWorld&) = delete;

        public:
            // Spawns every entity a compiled map asked for, and returns how many were
            // created. A classname with no script in `content://entities/` is reported once
            // and skipped - a map referring to an entity the game does not have should not
            // stop the map loading.
            int load(const CompiledMap& map);

            Entity* spawn(const std::string& classname,
                const std::vector<MapKeyValue>& keyvalues,
                const std::vector<MapConnection>& connections = {});

            // Ticks every entity, then delivers whatever I/O has come due.
            void update(float delta);

            void clear();

        public:
            Entity* find_by_id(int id);
            Entity* find_by_targetname(const std::string& name);
            std::vector<Entity*> find_all_by_targetname(const std::string& name);
            std::vector<Entity*> find_by_classname(const std::string& classname);
            std::vector<Entity*> all();

            int count() const;
            int pending_input_count() const;

        public:
            // Follows every connection drawn from `output` on `source` and queues the inputs
            // they lead to. Called by Entity::fire().
            void dispatch_output(Entity& source, const std::string& output, const std::string& parameter);

            // Queues one input directly, which is how engine code (rather than a map
            // connection) pokes an entity.
            void queue_input(int target, const std::string& input, const std::string& parameter,
                int activator, float delay);
    };
}
