#include <IronHull/entity/base/BaseProperties.hpp>

namespace IronHull
{
    namespace
    {
        EntityProperty make(const std::string& name, PropertyType type, const std::string& display,
            const std::string& description, int order)
        {
            EntityProperty property;
            property.name = name;
            property.type = type;
            property.display = display;
            property.description = description;
            property.has_order = true;
            property.order = order;
            property.inherited = true;
            property.default_value.type = type;

            return property;
        }

        // Inherited properties are numbered well below zero so they sort above a script's
        // own, which is where an inspector wants them: a reader looks for what an entity is
        // called before what it does.
        std::vector<EntityProperty> targetname_set()
        {
            return {
                make("targetname", PropertyType::TARGET, "Name",
                    "Name other entities use to aim their outputs at this one.", -100),
            };
        }

        std::vector<EntityProperty> angles_set()
        {
            return {
                make("angles", PropertyType::ANGLE, "Angles",
                    "Pitch, yaw and roll in degrees. Yaw turns about the world's up axis,"
                    " and positive pitch tips downwards.", -90),
            };
        }

        std::vector<EntityProperty> parent_set()
        {
            return {
                make("parentname", PropertyType::TARGET, "Parent",
                    "Name of an entity this one moves with.", -80),
            };
        }
    }

    std::vector<EntityProperty> BaseProperties::get(const std::string& name)
    {
        if (name == "Targetname") {
            return targetname_set();
        }

        if (name == "Angles") {
            return angles_set();
        }

        if (name == "Parent") {
            return parent_set();
        }

        return {};
    }

    bool BaseProperties::exists(const std::string& name)
    {
        return !BaseProperties::get(name).empty();
    }

    std::vector<std::string> BaseProperties::names()
    {
        return { "Targetname", "Angles", "Parent" };
    }
}
