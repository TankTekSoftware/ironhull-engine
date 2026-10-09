#include <IronHull/entity/EntityRegistry.hpp>

#include <algorithm>

#include <raylib.h>

#include <IronHull/io/FileSystem.hpp>
#include <IronHull/script/LuaVM.hpp>

namespace IronHull
{
    namespace
    {
        bool has_lua_extension(const std::string& uri)
        {
            return uri.size() > 4 && uri.compare(uri.size() - 4, 4, ".lua") == 0;
        }
    }

    EntityRegistry& EntityRegistry::get_singleton()
    {
        static EntityRegistry singleton;
        return singleton;
    }

    int EntityRegistry::load_directory(const std::string& uri_directory)
    {
        std::vector<std::string> files = FileSystem::list_files(uri_directory);

        // The filesystem lists in whatever order the backend gives, which differs between a
        // loose directory and a packed archive. Sorting keeps load order - and so the order
        // of any warning a script produces - the same either way.
        std::sort(files.begin(), files.end());

        int loaded = 0;

        for (const std::string& uri : files) {
            if (!has_lua_extension(uri)) {
                continue;
            }

            try {
                const EntityDefinition& definition = EntityRegistry::load_file(uri);
                TraceLog(LOG_INFO, "ENTITY: Loaded '%s' from %s", definition.classname.c_str(), uri.c_str());
                ++loaded;
            } catch (const std::exception& error) {
                TraceLog(LOG_WARNING, "ENTITY: %s", error.what());
            }
        }

        if (files.empty()) {
            TraceLog(LOG_WARNING, "ENTITY: No entity scripts found in '%s'", uri_directory.c_str());
        }

        return loaded;
    }

    const EntityDefinition& EntityRegistry::load_file(const std::string& uri)
    {
        EntityRegistry& self = EntityRegistry::get_singleton();

        sol::table table = LuaVM::run_table_file(uri);
        EntityDefinition definition = EntityDefinition::from_lua(table, uri);

        std::string classname = definition.classname;

        auto entry = self.definitions.find(classname);
        if (entry != self.definitions.end()) {
            TraceLog(LOG_WARNING, "ENTITY: '%s' from %s replaces the definition from %s",
                classname.c_str(), uri.c_str(), entry->second->source_uri.c_str());

            // Replaced in place, so that anything already holding a pointer to this
            // definition - a live entity, mid-reload - keeps pointing at something valid.
            *entry->second = std::move(definition);
            return *entry->second;
        }

        auto inserted = self.definitions.emplace(classname,
            std::make_unique<EntityDefinition>(std::move(definition)));

        return *inserted.first->second;
    }

    const EntityDefinition* EntityRegistry::find(const std::string& classname)
    {
        EntityRegistry& self = EntityRegistry::get_singleton();

        auto entry = self.definitions.find(classname);
        return entry != self.definitions.end() ? entry->second.get() : nullptr;
    }

    bool EntityRegistry::has(const std::string& classname)
    {
        return EntityRegistry::find(classname) != nullptr;
    }

    std::vector<const EntityDefinition*> EntityRegistry::all()
    {
        EntityRegistry& self = EntityRegistry::get_singleton();

        std::vector<const EntityDefinition*> found;
        found.reserve(self.definitions.size());

        for (const auto& [classname, definition] : self.definitions) {
            found.push_back(definition.get());
        }

        std::sort(found.begin(), found.end(), [](const EntityDefinition* a, const EntityDefinition* b) {
            return a->classname < b->classname;
        });

        return found;
    }

    std::vector<const EntityDefinition*> EntityRegistry::in_category(const std::string& category)
    {
        std::vector<const EntityDefinition*> found;

        for (const EntityDefinition* definition : EntityRegistry::all()) {
            if (definition->category == category) {
                found.push_back(definition);
            }
        }

        return found;
    }

    void EntityRegistry::clear()
    {
        EntityRegistry& self = EntityRegistry::get_singleton();
        self.definitions.clear();
    }
}
