#pragma once

#include <string>
#include <vector>

#include <raylib.h>
#include <sol/sol.hpp>

#include <IronHull/entity/PropertyValue.hpp>

namespace IronHull
{
    // How the editor is allowed to place an entity.
    enum class EntityClass
    {
        // Placed as a point in the world, drawn using the editor bounds or model.
        POINT,

        // Built out of map brushes; its geometry comes from the solid it is made of, and the
        // compiler gives it a model of its own so it can move.
        BRUSH,
    };

    // One option of a `choices` property.
    struct EntityChoice
    {
        std::string value;
        std::string display;
    };

    // A property an entity exposes: editable per-instance in the editor, written into the
    // map, and resolved onto self.props by the time the script's OnSpawn runs.
    struct EntityProperty
    {
        std::string name;
        PropertyType type = PropertyType::STRING;
        PropertyValue default_value;

        std::string display;
        std::string description;

        bool has_min = false;
        double min = 0.0;
        bool has_max = false;
        double max = 0.0;

        std::vector<EntityChoice> choices;

        // Properties with an explicit order sort before those without, which keeps an
        // inspector laid out the way the script author intended while still showing
        // properties they did not bother to number.
        bool has_order = false;
        int order = 0;

        // Came from a shared set named in `base` rather than from the script itself.
        bool inherited = false;
    };

    // One value an input carries. Declared by the script so the editor knows what to ask
    // for when a connection is wired up.
    struct EntityIoParameter
    {
        PropertyType type = PropertyType::STRING;
        std::string display;
    };

    struct EntityInput
    {
        std::string name;
        std::string description;
        std::vector<EntityIoParameter> parameters;
    };

    struct EntityOutput
    {
        std::string name;
        std::string description;
    };

    // Presentation for the editor only; the runtime ignores all of it.
    struct EntityEditorInfo
    {
        // Selection and render bounds in authored map space, as the script writes them:
        // { min_x, min_y, min_z, max_x, max_y, max_z }.
        bool has_size = false;
        BoundingBox size = { { -8.0f, -8.0f, -8.0f }, { 8.0f, 8.0f, 8.0f } };

        Color color = { 255, 0, 255, 255 };
        std::string model;
        std::string icon;
    };

    // Everything an entity script declares about itself: what the editor needs to offer it,
    // and what the runtime needs to spawn it.
    //
    // This is the replacement for the .fgd file a Quake-lineage editor would have loaded
    // separately. Keeping the declaration in the same file as the behaviour means the two
    // cannot drift apart - an input the editor offers is a method that exists, because the
    // definition is read from the same table the method was written on.
    class EntityDefinition
    {
        public:
            EntityClass entity_class = EntityClass::POINT;

            std::string classname;
            std::string display;
            std::string category;
            std::string description;

            // Names of shared property sets mixed in ahead of this script's own properties.
            std::vector<std::string> base;

            EntityEditorInfo editor;

            std::vector<EntityProperty> properties;
            std::vector<EntityInput> inputs;
            std::vector<EntityOutput> outputs;

            // The table the script returned. Instances share it as their method table, so
            // the behaviour lives here exactly once however many entities are spawned.
            sol::table script;

            std::string source_uri;

        public:
            const EntityProperty* find_property(const std::string& name) const;
            const EntityInput* find_input(const std::string& name) const;
            const EntityOutput* find_output(const std::string& name) const;

        public:
            // Reads a definition out of the table a script returned, mixing in the shared
            // property sets it asked for.
            //
            // Throws std::invalid_argument for a declaration that cannot be honoured: no
            // classname, an unknown property type, or an input with no matching method. That
            // last check is what stops a connection the editor happily offers from failing
            // silently at runtime.
            static EntityDefinition from_lua(const sol::table& table, const std::string& source_uri);
    };
}
