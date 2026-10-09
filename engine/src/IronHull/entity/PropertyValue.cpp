#include <IronHull/entity/PropertyValue.hpp>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace IronHull
{
    namespace
    {
        // Pulls three numbers out of a Lua value written either as a Vector3 or, as is far
        // more natural in a script, a plain array: { 0, 0, 72 }.
        bool read_triple(const sol::object& value, float& x, float& y, float& z)
        {
            if (value.is<Vector3>()) {
                Vector3 vector = value.as<Vector3>();
                x = vector.x;
                y = vector.y;
                z = vector.z;
                return true;
            }

            if (value.is<sol::table>()) {
                sol::table table = value.as<sol::table>();

                x = table.get_or(1, 0.0f);
                y = table.get_or(2, 0.0f);
                z = table.get_or(3, 0.0f);
                return true;
            }

            if (value.is<std::string>()) {
                std::string text = value.as<std::string>();
                return std::sscanf(text.c_str(), "%f %f %f", &x, &y, &z) >= 1;
            }

            return false;
        }

        unsigned char to_channel(double value)
        {
            if (value < 0.0) {
                return 0;
            }

            if (value > 255.0) {
                return 255;
            }

            return static_cast<unsigned char>(value);
        }
    }

    PropertyType PropertyValue::type_from_name(const std::string& name)
    {
        if (name == "int") return PropertyType::INT;
        if (name == "float") return PropertyType::FLOAT;
        if (name == "bool") return PropertyType::BOOL;
        if (name == "string") return PropertyType::STRING;
        if (name == "vector") return PropertyType::VECTOR;
        if (name == "angle") return PropertyType::ANGLE;
        if (name == "color") return PropertyType::COLOR;
        if (name == "choices") return PropertyType::CHOICES;
        if (name == "flags") return PropertyType::FLAGS;
        if (name == "target") return PropertyType::TARGET;
        if (name == "model") return PropertyType::MODEL;
        if (name == "material") return PropertyType::MATERIAL;
        if (name == "sound") return PropertyType::SOUND;

        throw std::invalid_argument("PropertyValue: '" + name + "' is not a property type"
            " (expected one of: int, float, bool, string, vector, angle, color, choices,"
            " flags, target, model, material, sound)");
    }

    const char* PropertyValue::type_name(PropertyType type)
    {
        switch (type) {
            case PropertyType::INT: return "int";
            case PropertyType::FLOAT: return "float";
            case PropertyType::BOOL: return "bool";
            case PropertyType::STRING: return "string";
            case PropertyType::VECTOR: return "vector";
            case PropertyType::ANGLE: return "angle";
            case PropertyType::COLOR: return "color";
            case PropertyType::CHOICES: return "choices";
            case PropertyType::FLAGS: return "flags";
            case PropertyType::TARGET: return "target";
            case PropertyType::MODEL: return "model";
            case PropertyType::MATERIAL: return "material";
            case PropertyType::SOUND: return "sound";
        }

        return "string";
    }

    PropertyValue PropertyValue::parse(PropertyType type, const std::string& text)
    {
        PropertyValue value;
        value.type = type;

        switch (type) {
            case PropertyType::INT:
            case PropertyType::FLAGS:
                value.integer = std::strtoll(text.c_str(), nullptr, 10);
                break;

            case PropertyType::FLOAT:
                value.number = std::strtod(text.c_str(), nullptr);
                break;

            case PropertyType::BOOL:
                // Maps write 0 and 1, but a hand-edited map may well say "true", and there
                // is no reason to be strict about it.
                value.boolean = text == "1" || text == "true" || text == "yes";
                break;

            case PropertyType::VECTOR:
            case PropertyType::ANGLE:
                std::sscanf(text.c_str(), "%f %f %f", &value.vector.x, &value.vector.y, &value.vector.z);
                break;

            case PropertyType::COLOR: {
                float channels[3] = { 255.0f, 255.0f, 255.0f };
                std::sscanf(text.c_str(), "%f %f %f", &channels[0], &channels[1], &channels[2]);

                value.color = Color{
                    to_channel(channels[0]),
                    to_channel(channels[1]),
                    to_channel(channels[2]),
                    255,
                };
                break;
            }

            default:
                value.text = text;
                break;
        }

        return value;
    }

    PropertyValue PropertyValue::from_lua(PropertyType type, const sol::object& value)
    {
        PropertyValue result;
        result.type = type;

        if (!value.valid() || value == sol::lua_nil) {
            return result;
        }

        switch (type) {
            case PropertyType::INT:
            case PropertyType::FLAGS:
                result.integer = value.is<long long>() ? value.as<long long>()
                    : static_cast<long long>(value.as<double>());
                break;

            case PropertyType::FLOAT:
                result.number = value.as<double>();
                break;

            case PropertyType::BOOL:
                result.boolean = value.as<bool>();
                break;

            case PropertyType::VECTOR:
            case PropertyType::ANGLE:
                read_triple(value, result.vector.x, result.vector.y, result.vector.z);
                break;

            case PropertyType::COLOR: {
                float channels[3] = { 255.0f, 255.0f, 255.0f };
                read_triple(value, channels[0], channels[1], channels[2]);

                result.color = Color{
                    to_channel(channels[0]),
                    to_channel(channels[1]),
                    to_channel(channels[2]),
                    255,
                };
                break;
            }

            default:
                result.text = value.is<std::string>() ? value.as<std::string>() : std::string();
                break;
        }

        return result;
    }

    std::string PropertyValue::to_string() const
    {
        char buffer[128];

        switch (this->type) {
            case PropertyType::INT:
            case PropertyType::FLAGS:
                std::snprintf(buffer, sizeof(buffer), "%lld", this->integer);
                return std::string(buffer);

            case PropertyType::FLOAT:
                std::snprintf(buffer, sizeof(buffer), "%.6g", this->number);
                return std::string(buffer);

            case PropertyType::BOOL:
                return this->boolean ? "1" : "0";

            case PropertyType::VECTOR:
            case PropertyType::ANGLE:
                std::snprintf(buffer, sizeof(buffer), "%.6g %.6g %.6g",
                    static_cast<double>(this->vector.x),
                    static_cast<double>(this->vector.y),
                    static_cast<double>(this->vector.z));
                return std::string(buffer);

            case PropertyType::COLOR:
                std::snprintf(buffer, sizeof(buffer), "%u %u %u",
                    static_cast<unsigned int>(this->color.r),
                    static_cast<unsigned int>(this->color.g),
                    static_cast<unsigned int>(this->color.b));
                return std::string(buffer);

            default:
                return this->text;
        }
    }

    sol::object PropertyValue::to_lua(sol::state_view lua) const
    {
        switch (this->type) {
            case PropertyType::INT:
            case PropertyType::FLAGS:
                return sol::make_object(lua, this->integer);

            case PropertyType::FLOAT:
                return sol::make_object(lua, this->number);

            case PropertyType::BOOL:
                return sol::make_object(lua, this->boolean);

            case PropertyType::VECTOR:
            case PropertyType::ANGLE:
                return sol::make_object(lua, this->vector);

            case PropertyType::COLOR:
                return sol::make_object(lua, this->color);

            default:
                return sol::make_object(lua, this->text);
        }
    }
}
