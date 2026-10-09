#pragma once

#include <string>

#include <raylib.h>
#include <sol/sol.hpp>

namespace IronHull
{
    // The property types an entity script may declare, matching the `type` field in the
    // `properties` table of a `content://entities/*.lua`.
    //
    // Several of these are the same underlying string and differ only in what the editor
    // should offer for them - TARGET pops up the list of named entities, MODEL and MATERIAL
    // and SOUND browse content - which is exactly why the type is kept rather than collapsed
    // down to the handful of storage kinds.
    enum class PropertyType
    {
        INT,
        FLOAT,
        BOOL,
        STRING,
        VECTOR,
        ANGLE,
        COLOR,
        CHOICES,
        FLAGS,
        TARGET,
        MODEL,
        MATERIAL,
        SOUND,
    };

    // One resolved property: a script's declared default, overridden by whatever the map
    // authored for that entity.
    //
    // A property has to survive three different worlds - a text keyvalue in a .map, a native
    // value a Lua script does arithmetic on, and a widget in the editor - so this holds the
    // typed value and knows how to convert in and out of all three. Only the member matching
    // `type` carries meaning.
    class PropertyValue
    {
        public:
            PropertyType type = PropertyType::STRING;

            long long integer = 0;
            double number = 0.0;
            bool boolean = false;
            std::string text;
            Vector3 vector = { 0.0f, 0.0f, 0.0f };
            Color color = { 255, 255, 255, 255 };

        public:
            // Throws std::invalid_argument for a type name no script should be using, which
            // is how a typo in a property declaration gets caught at load rather than
            // turning into a silently wrong default.
            static PropertyType type_from_name(const std::string& name);
            static const char* type_name(PropertyType type);

        public:
            // Reads a map keyvalue. Anything unparseable falls back to a zero value rather
            // than throwing: a map is content, and one bad keyvalue should not stop a level
            // from loading.
            static PropertyValue parse(PropertyType type, const std::string& text);

            // Reads a default out of a script's property declaration. Vectors and colours
            // accept either the engine type or a plain Lua array, since that is how they are
            // naturally written in a script.
            static PropertyValue from_lua(PropertyType type, const sol::object& value);

        public:
            // The form written back into a .map keyvalue.
            std::string to_string() const;

            // The form a script sees on self.props.
            sol::object to_lua(sol::state_view lua) const;
    };
}
