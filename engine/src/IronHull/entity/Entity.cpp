#include <IronHull/entity/Entity.hpp>

#include <cstdio>
#include <cstdlib>

#include <IronHull/entity/EntityWorld.hpp>
#include <IronHull/entity/PropertyValue.hpp>
#include <IronHull/map/MapSpace.hpp>
#include <IronHull/script/LuaVM.hpp>

namespace IronHull
{
    namespace
    {
        Vector3 parse_vector(const std::string& text)
        {
            Vector3 value = { 0.0f, 0.0f, 0.0f };
            std::sscanf(text.c_str(), "%f %f %f", &value.x, &value.y, &value.z);

            return value;
        }

        const std::string* find_keyvalue(const std::vector<MapKeyValue>& keyvalues, const std::string& key)
        {
            for (const MapKeyValue& pair : keyvalues) {
                if (pair.key == key) {
                    return &pair.value;
                }
            }

            return nullptr;
        }
    }

    Entity::Entity(EntityWorld& owner, const EntityDefinition& type, int entity_id)
    {
        this->world = &owner;
        this->definition = &type;
        this->id = entity_id;
    }

    Entity::~Entity()
    {
        this->detach_lua();
    }

    void Entity::configure(const std::vector<MapKeyValue>& keyvalues,
        const std::vector<MapConnection>& wiring)
    {
        this->connections = wiring;
        this->fire_counts.assign(this->connections.size(), 0);

        const std::string* targetname = find_keyvalue(keyvalues, "targetname");
        if (targetname != nullptr) {
            this->targetname = *targetname;
        }

        const std::string* origin = find_keyvalue(keyvalues, "origin");
        if (origin != nullptr) {
            this->origin = parse_vector(*origin);
        }

        const std::string* angles = find_keyvalue(keyvalues, "angles");
        if (angles != nullptr) {
            this->angles = parse_vector(*angles);
        } else {
            // Quake-lineage maps often carry just a yaw under "angle", and editors still
            // write it for entities that only ever turn on the spot.
            const std::string* angle = find_keyvalue(keyvalues, "angle");
            if (angle != nullptr) {
                this->angles = Vector3{ 0.0f, static_cast<float>(std::strtod(angle->c_str(), nullptr)), 0.0f };
            }
        }

        // A brush entity is pointed at its compiled geometry with a "*1"-style key.
        const std::string* model = find_keyvalue(keyvalues, "model");
        if (model != nullptr && !model->empty() && (*model)[0] == '*') {
            this->model = std::atoi(model->c_str() + 1);
        }

        this->bind_lua(keyvalues);
    }

    void Entity::bind_lua(const std::vector<MapKeyValue>& keyvalues)
    {
        sol::state& lua = LuaVM::get();

        this->self = lua.create_table();

        // Reads that miss the instance fall through to the script's table, which is where
        // the methods live. Writes land on the instance, so one zombie's state never leaks
        // into another's.
        sol::table metatable = lua.create_table();
        metatable["__index"] = this->definition->script;
        this->self[sol::metatable_key] = metatable;

        // Properties: the script's declared default, overridden by whatever the map
        // authored. Resolving them up front is what lets a script read self.props.health
        // without having to know or care that the mapper changed it.
        sol::table properties = lua.create_table();

        for (const EntityProperty& property : this->definition->properties) {
            PropertyValue value = property.default_value;

            const std::string* authored = find_keyvalue(keyvalues, property.name);
            if (authored != nullptr) {
                value = PropertyValue::parse(property.type, *authored);
            }

            // A vector property is authored in the editor's Z-up space, so "0 0 1" written
            // for a direction means up - and would arrive pointing sideways if handed to a
            // script unchanged. The compiler converts the geometry and the origin, but it
            // has no way to know which keyvalues are vectors; this does, because the script
            // said so.
            //
            // Rotated only, never scaled. A direction must not be scaled, and that is what
            // the overwhelming majority of vector properties are; a script wanting a scaled
            // position can convert one itself with ironhull.to_engine_space().
            if (property.type == PropertyType::VECTOR) {
                value.vector = MapSpace::to_engine(value.vector);
            }

            properties[property.name] = value.to_lua(lua);
        }

        this->self["props"] = properties;
        this->self["classname"] = this->definition->classname;
        this->self["targetname"] = this->targetname;
        this->self["id"] = this->id;

        // Instance methods capture the world and this entity's id rather than a pointer to
        // the entity, so a table a script holds on to past the entity's lifetime finds
        // nothing instead of reaching freed memory.
        EntityWorld* owner = this->world;
        int entity_id = this->id;

        this->self["fire"] = [owner, entity_id](sol::table, const std::string& output, sol::optional<std::string> parameter) {
            Entity* entity = owner->find_by_id(entity_id);
            if (entity != nullptr) {
                entity->fire(output, parameter.value_or(std::string()));
            }
        };

        this->self["get_origin"] = [owner, entity_id](sol::table) {
            Entity* entity = owner->find_by_id(entity_id);
            return entity != nullptr ? entity->get_origin() : Vector3{ 0.0f, 0.0f, 0.0f };
        };

        this->self["set_origin"] = [owner, entity_id](sol::table, Vector3 value) {
            Entity* entity = owner->find_by_id(entity_id);
            if (entity != nullptr) {
                entity->set_origin(value);
            }
        };

        this->self["get_angles"] = [owner, entity_id](sol::table) {
            Entity* entity = owner->find_by_id(entity_id);
            return entity != nullptr ? entity->get_angles() : Vector3{ 0.0f, 0.0f, 0.0f };
        };

        this->self["set_angles"] = [owner, entity_id](sol::table, Vector3 value) {
            Entity* entity = owner->find_by_id(entity_id);
            if (entity != nullptr) {
                entity->set_angles(value);
            }
        };

        this->self["forward"] = [owner, entity_id](sol::table) {
            Entity* entity = owner->find_by_id(entity_id);
            return entity != nullptr ? entity->forward() : Vector3{ 0.0f, 0.0f, 0.0f };
        };

        this->self["destroy"] = [owner, entity_id](sol::table) {
            Entity* entity = owner->find_by_id(entity_id);
            if (entity != nullptr) {
                entity->destroy();
            }
        };

        this->self["log"] = [owner, entity_id](sol::table, const std::string& message) {
            Entity* entity = owner->find_by_id(entity_id);
            const char* classname = entity != nullptr ? entity->get_classname().c_str() : "entity";

            TraceLog(LOG_INFO, "SCRIPT: %s: %s", classname, message.c_str());
        };
    }

    void Entity::detach_lua()
    {
        if (!this->self.valid()) {
            return;
        }

        for (const char* name : { "fire", "get_origin", "set_origin", "get_angles", "set_angles",
                "forward", "destroy", "log" }) {
            this->self[name] = sol::lua_nil;
        }

        this->self = sol::table();
    }

    bool Entity::call_method(const std::string& name, const std::vector<sol::object>& arguments)
    {
        if (!this->self.valid()) {
            return false;
        }

        sol::object member = this->self[name];
        if (!member.valid() || !member.is<sol::protected_function>()) {
            return false;
        }

        sol::protected_function method = member.as<sol::protected_function>();

        // Scripts are content, so a mistake in one has to be survivable: the error is
        // reported against the entity that caused it and the frame carries on.
        sol::protected_function_result result = arguments.empty()
            ? method(this->self)
            : method(this->self, sol::as_args(arguments));

        if (!result.valid()) {
            sol::error error = result;
            TraceLog(LOG_WARNING, "SCRIPT: %s:%s() failed: %s",
                this->definition->classname.c_str(), name.c_str(), error.what());
            return false;
        }

        return true;
    }

    void Entity::spawn()
    {
        if (this->spawned) {
            return;
        }

        this->spawned = true;
        this->call_method("OnSpawn");
    }

    void Entity::update(float delta)
    {
        if (!this->alive) {
            return;
        }

        sol::state& lua = LuaVM::get();
        this->call_method("OnUpdate", { sol::make_object(lua, delta) });
    }

    void Entity::fire(const std::string& output, const std::string& parameter)
    {
        if (this->definition->find_output(output) == nullptr) {
            TraceLog(LOG_WARNING, "ENTITY: '%s' fired output '%s', which it does not declare",
                this->definition->classname.c_str(), output.c_str());
        }

        this->world->dispatch_output(*this, output, parameter);
    }

    bool Entity::accept_input(const std::string& input, const std::string& parameter, int activator_id)
    {
        if (!this->alive) {
            return false;
        }

        const EntityInput* declared = this->definition->find_input(input);
        if (declared == nullptr) {
            TraceLog(LOG_WARNING, "ENTITY: '%s' has no input '%s'",
                this->definition->classname.c_str(), input.c_str());
            return false;
        }

        this->activator = activator_id;

        // The connection carries its parameter as text, since that is all a map file can
        // hold. The input's declaration says what it was meant to be, so it arrives at the
        // script as a number or a boolean rather than as a string the script has to convert.
        std::vector<sol::object> arguments;

        if (!declared->parameters.empty()) {
            sol::state& lua = LuaVM::get();
            PropertyValue value = PropertyValue::parse(declared->parameters[0].type, parameter);
            arguments.push_back(value.to_lua(lua));
        }

        return this->call_method(input, arguments);
    }

    int Entity::get_id() const
    {
        return this->id;
    }

    const EntityDefinition& Entity::get_definition() const
    {
        return *this->definition;
    }

    const std::string& Entity::get_classname() const
    {
        return this->definition->classname;
    }

    const std::string& Entity::get_targetname() const
    {
        return this->targetname;
    }

    Vector3 Entity::get_origin() const
    {
        return this->origin;
    }

    void Entity::set_origin(Vector3 value)
    {
        this->origin = value;
    }

    Vector3 Entity::get_angles() const
    {
        return this->angles;
    }

    void Entity::set_angles(Vector3 value)
    {
        this->angles = value;
    }

    Vector3 Entity::forward() const
    {
        return MapSpace::angles_to_forward(this->angles);
    }

    int Entity::get_model() const
    {
        return this->model;
    }

    bool Entity::is_brush_entity() const
    {
        return this->model >= 0;
    }

    bool Entity::is_alive() const
    {
        return this->alive;
    }

    void Entity::destroy()
    {
        this->alive = false;
    }

    const std::vector<MapConnection>& Entity::get_connections() const
    {
        return this->connections;
    }

    const std::vector<int>& Entity::get_fire_counts() const
    {
        return this->fire_counts;
    }

    int Entity::get_activator() const
    {
        return this->activator;
    }

    void Entity::note_connection_fired(int connection_index)
    {
        if (connection_index < 0 || connection_index >= static_cast<int>(this->fire_counts.size())) {
            return;
        }

        ++this->fire_counts[static_cast<size_t>(connection_index)];
    }

    sol::table Entity::lua_table() const
    {
        return this->self;
    }
}
