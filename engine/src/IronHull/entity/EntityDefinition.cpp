#include <IronHull/entity/EntityDefinition.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <raylib.h>

#include <IronHull/entity/base/BaseProperties.hpp>

namespace IronHull
{
    namespace
    {
        std::string read_string(const sol::table& table, const char* key, const std::string& fallback = "")
        {
            sol::object value = table[key];
            return value.is<std::string>() ? value.as<std::string>() : fallback;
        }

        void read_editor(const sol::table& table, EntityEditorInfo& editor, const std::string& classname)
        {
            sol::object value = table["editor"];
            if (!value.is<sol::table>()) {
                return;
            }

            sol::table block = value.as<sol::table>();

            sol::object size = block["size"];
            if (size.is<sol::table>()) {
                sol::table extents = size.as<sol::table>();

                // Written the way a mapper thinks about bounds: six numbers, mins then maxs.
                if (extents.size() >= 6) {
                    editor.has_size = true;
                    editor.size.min = Vector3{ extents.get_or(1, 0.0f), extents.get_or(2, 0.0f), extents.get_or(3, 0.0f) };
                    editor.size.max = Vector3{ extents.get_or(4, 0.0f), extents.get_or(5, 0.0f), extents.get_or(6, 0.0f) };
                } else {
                    TraceLog(LOG_WARNING, "ENTITY: '%s' editor.size needs 6 numbers (min x y z, max x y z)",
                        classname.c_str());
                }
            }

            sol::object color = block["color"];
            if (color.is<sol::table>()) {
                sol::table channels = color.as<sol::table>();
                editor.color = Color{
                    static_cast<unsigned char>(channels.get_or(1, 255)),
                    static_cast<unsigned char>(channels.get_or(2, 0)),
                    static_cast<unsigned char>(channels.get_or(3, 255)),
                    255,
                };
            }

            editor.model = read_string(block, "model");
            editor.icon = read_string(block, "icon");
        }

        EntityProperty read_property(const std::string& name, const sol::table& block, const std::string& classname)
        {
            sol::object type_name = block["type"];
            if (!type_name.is<std::string>()) {
                throw std::invalid_argument("EntityDefinition: '" + classname + "' property '" + name
                    + "' has no type");
            }

            EntityProperty property;
            property.name = name;
            property.type = PropertyValue::type_from_name(type_name.as<std::string>());
            property.display = read_string(block, "display", name);
            property.description = read_string(block, "description");
            property.default_value = PropertyValue::from_lua(property.type, block["default"]);

            sol::object min = block["min"];
            if (min.is<double>()) {
                property.has_min = true;
                property.min = min.as<double>();
            }

            sol::object max = block["max"];
            if (max.is<double>()) {
                property.has_max = true;
                property.max = max.as<double>();
            }

            sol::object order = block["order"];
            if (order.is<double>()) {
                property.has_order = true;
                property.order = static_cast<int>(order.as<double>());
            }

            sol::object choices = block["choices"];
            if (choices.is<sol::table>()) {
                sol::table list = choices.as<sol::table>();

                for (size_t index = 1; index <= list.size(); ++index) {
                    sol::object option = list[index];
                    if (!option.is<sol::table>()) {
                        continue;
                    }

                    sol::table entry = option.as<sol::table>();

                    EntityChoice choice;
                    choice.value = read_string(entry, "value");
                    choice.display = read_string(entry, "display", choice.value);
                    property.choices.push_back(choice);
                }
            }

            if (property.type == PropertyType::CHOICES && property.choices.empty()) {
                TraceLog(LOG_WARNING, "ENTITY: '%s' property '%s' is a choices property with no choices",
                    classname.c_str(), name.c_str());
            }

            return property;
        }
    }

    const EntityProperty* EntityDefinition::find_property(const std::string& name) const
    {
        for (const EntityProperty& property : this->properties) {
            if (property.name == name) {
                return &property;
            }
        }

        return nullptr;
    }

    const EntityInput* EntityDefinition::find_input(const std::string& name) const
    {
        for (const EntityInput& input : this->inputs) {
            if (input.name == name) {
                return &input;
            }
        }

        return nullptr;
    }

    const EntityOutput* EntityDefinition::find_output(const std::string& name) const
    {
        for (const EntityOutput& output : this->outputs) {
            if (output.name == name) {
                return &output;
            }
        }

        return nullptr;
    }

    EntityDefinition EntityDefinition::from_lua(const sol::table& table, const std::string& source_uri)
    {
        EntityDefinition definition;
        definition.script = table;
        definition.source_uri = source_uri;

        definition.classname = read_string(table, "classname");
        if (definition.classname.empty()) {
            throw std::invalid_argument("EntityDefinition: '" + source_uri + "' has no classname");
        }

        std::string class_kind = read_string(table, "class", "PointClass");
        if (class_kind == "BrushClass") {
            definition.entity_class = EntityClass::BRUSH;
        } else if (class_kind == "PointClass") {
            definition.entity_class = EntityClass::POINT;
        } else {
            throw std::invalid_argument("EntityDefinition: '" + definition.classname + "' has class '"
                + class_kind + "' (expected \"PointClass\" or \"BrushClass\")");
        }

        definition.display = read_string(table, "display", definition.classname);
        definition.category = read_string(table, "category", "Other");
        definition.description = read_string(table, "description");

        read_editor(table, definition.editor, definition.classname);

        // --- BASE PROPERTY SETS --- //

        sol::object base = table["base"];
        if (base.is<sol::table>()) {
            sol::table list = base.as<sol::table>();

            for (size_t index = 1; index <= list.size(); ++index) {
                sol::object name = list[index];
                if (!name.is<std::string>()) {
                    continue;
                }

                std::string set = name.as<std::string>();
                definition.base.push_back(set);

                std::vector<EntityProperty> inherited = BaseProperties::get(set);

                if (inherited.empty()) {
                    TraceLog(LOG_WARNING, "ENTITY: '%s' asks for base set '%s', which does not exist",
                        definition.classname.c_str(), set.c_str());
                    continue;
                }

                for (EntityProperty& property : inherited) {
                    definition.properties.push_back(std::move(property));
                }
            }
        }

        // --- PROPERTIES --- //

        sol::object properties = table["properties"];
        if (properties.is<sol::table>()) {
            properties.as<sol::table>().for_each([&](sol::object key, sol::object value) {
                if (!key.is<std::string>() || !value.is<sol::table>()) {
                    return;
                }

                std::string name = key.as<std::string>();
                EntityProperty property = read_property(name, value.as<sol::table>(), definition.classname);

                // A script redeclaring an inherited property takes it over rather than
                // ending up with two of the same name.
                for (EntityProperty& existing : definition.properties) {
                    if (existing.name == name) {
                        existing = std::move(property);
                        return;
                    }
                }

                definition.properties.push_back(std::move(property));
            });
        }

        // Lua tables have no order, so a property table is read in whatever order the hash
        // happens to give. Sorting by the declared order - and by name for the ones that
        // declared none - is what makes an inspector lay out the same way every run.
        std::stable_sort(definition.properties.begin(), definition.properties.end(),
            [](const EntityProperty& a, const EntityProperty& b) {
                int a_order = a.has_order ? a.order : std::numeric_limits<int>::max();
                int b_order = b.has_order ? b.order : std::numeric_limits<int>::max();

                if (a_order != b_order) {
                    return a_order < b_order;
                }

                return a.name < b.name;
            });

        // --- OUTPUTS --- //

        sol::object outputs = table["outputs"];
        if (outputs.is<sol::table>()) {
            outputs.as<sol::table>().for_each([&](sol::object key, sol::object value) {
                if (!key.is<std::string>()) {
                    return;
                }

                EntityOutput output;
                output.name = key.as<std::string>();

                if (value.is<sol::table>()) {
                    output.description = read_string(value.as<sol::table>(), "description");
                }

                definition.outputs.push_back(std::move(output));
            });
        }

        std::sort(definition.outputs.begin(), definition.outputs.end(),
            [](const EntityOutput& a, const EntityOutput& b) { return a.name < b.name; });

        // --- INPUTS --- //

        sol::object inputs = table["inputs"];
        if (inputs.is<sol::table>()) {
            inputs.as<sol::table>().for_each([&](sol::object key, sol::object value) {
                if (!key.is<std::string>()) {
                    return;
                }

                EntityInput input;
                input.name = key.as<std::string>();

                if (value.is<sol::table>()) {
                    sol::table block = value.as<sol::table>();
                    input.description = read_string(block, "description");

                    sol::object parameters = block["parameters"];
                    if (parameters.is<sol::table>()) {
                        sol::table list = parameters.as<sol::table>();

                        for (size_t index = 1; index <= list.size(); ++index) {
                            sol::object entry = list[index];
                            if (!entry.is<sol::table>()) {
                                continue;
                            }

                            sol::table parameter_block = entry.as<sol::table>();
                            sol::object type_name = parameter_block["type"];

                            EntityIoParameter parameter;
                            parameter.type = type_name.is<std::string>()
                                ? PropertyValue::type_from_name(type_name.as<std::string>())
                                : PropertyType::STRING;
                            parameter.display = read_string(parameter_block, "display");

                            input.parameters.push_back(parameter);
                        }
                    }
                }

                definition.inputs.push_back(std::move(input));
            });
        }

        std::sort(definition.inputs.begin(), definition.inputs.end(),
            [](const EntityInput& a, const EntityInput& b) { return a.name < b.name; });

        // An input the editor offers has to be something the entity can actually do. Catching
        // it here turns a connection that would have failed silently in the middle of a
        // playthrough into an error the moment the script loads.
        for (const EntityInput& input : definition.inputs) {
            if (!table[input.name].valid() || !table[input.name].is<sol::protected_function>()) {
                throw std::invalid_argument("EntityDefinition: '" + definition.classname + "' declares input '"
                    + input.name + "' but has no function of that name");
            }
        }

        return definition;
    }
}
