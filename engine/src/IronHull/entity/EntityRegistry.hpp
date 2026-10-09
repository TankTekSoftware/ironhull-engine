#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <IronHull/entity/EntityDefinition.hpp>

namespace IronHull
{
    // Every entity type the game knows about, loaded from `content://entities/*.lua`.
    //
    // Definitions are discovered rather than registered: dropping a script into the entities
    // directory is all it takes to add an entity type, with no C++ to recompile and no list
    // to keep in step. Both halves of the engine read from here - the runtime to spawn a
    // classname a map asked for, the editor to populate its entity browser.
    class EntityRegistry
    {
        private:
            // Definitions hold a sol::table and are handed out by pointer, so they are kept
            // behind unique_ptr: rehashing the map must not move a definition an entity is
            // already pointing at.
            std::unordered_map<std::string, std::unique_ptr<EntityDefinition>> definitions;

        private:
            static EntityRegistry& get_singleton();

        private:
            EntityRegistry() = default;
            ~EntityRegistry() = default;
            EntityRegistry(const EntityRegistry&) = delete;
            EntityRegistry& operator=(const EntityRegistry&) = delete;

        public:
            // Loads every `.lua` in a directory, by default `content://entities/`. Returns
            // how many definitions were loaded.
            //
            // A script that fails to load is reported and skipped rather than aborting the
            // scan: one broken entity should not cost the game every other one.
            static int load_directory(const std::string& uri_directory = "content://entities");

            // Loads a single script, replacing any definition already registered under the
            // same classname. Throws for a script that does not load or does not describe a
            // valid entity.
            static const EntityDefinition& load_file(const std::string& uri);

        public:
            static const EntityDefinition* find(const std::string& classname);
            static bool has(const std::string& classname);
            static std::vector<const EntityDefinition*> all();

            // Definitions in a category, for an editor browser that groups them.
            static std::vector<const EntityDefinition*> in_category(const std::string& category);

        public:
            // Releases every definition. Must run before LuaVM::shutdown(), since the
            // definitions hold tables belonging to that state.
            static void clear();
    };
}
