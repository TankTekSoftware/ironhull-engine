#pragma once

#include <string>
#include <vector>

#include <IronHull/entity/EntityDefinition.hpp>

namespace IronHull
{
    // The shared property sets an entity script can mix in by name:
    //
    //   base = { "Targetname", "Angles" },
    //
    // These are the properties so many entities need that repeating them in every script
    // would be noise - and worse, would let them drift apart, so that two entities ended up
    // with subtly different ideas of what "angles" means.
    //
    // They are built in rather than loaded from content because the engine itself relies on
    // them: map I/O finds its targets by `targetname`, and Entity reads `angles` to build its
    // facing. A script is free to declare a property of the same name itself, which wins over
    // the inherited one.
    class BaseProperties
    {
        public:
            // The properties of a named set, or an empty list for a name that is not one.
            // The caller reports the unknown name - this does not throw, so that one bad
            // entry in a `base` list does not stop the entity loading.
            static std::vector<EntityProperty> get(const std::string& name);

            static bool exists(const std::string& name);

            // Every set's name, for the editor to show and for diagnostics that need to say
            // what was available.
            static std::vector<std::string> names();
    };
}
