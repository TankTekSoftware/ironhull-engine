#include <IronHull/entity/EntityWorld.hpp>

#include <algorithm>
#include <unordered_set>

#include <raylib.h>

#include <IronHull/entity/EntityRegistry.hpp>

namespace IronHull
{
    namespace
    {
        // A ceiling on how much I/O one frame may deliver. Inputs are queued, so a pair of
        // entities firing at each other with no delay would otherwise spin here forever;
        // this turns that mapping mistake into a reported warning and a dropped frame of
        // I/O rather than a hang.
        constexpr int MAX_INPUTS_PER_FRAME = 1024;

        // Connection targets that mean something other than a name in the map.
        const char* TARGET_SELF = "!self";
        const char* TARGET_ACTIVATOR = "!activator";
    }

    EntityWorld::~EntityWorld()
    {
        this->clear();
    }

    int EntityWorld::load(const CompiledMap& map)
    {
        int spawned = 0;

        // Reported once per classname rather than once per entity: a map with forty missing
        // lights should say so once.
        std::unordered_set<std::string> missing;

        // Every entity is created before any of them spawns, so that an OnSpawn looking up
        // another entity by name finds it whatever order the map listed them in.
        size_t first_new = this->entities.size();

        for (const BspEntity& entity : map.entities) {
            std::string classname = entity.classname();

            if (classname.empty()) {
                continue;
            }

            const EntityDefinition* definition = EntityRegistry::find(classname);
            if (definition == nullptr) {
                // Worldspawn is the map itself. It only needs a script if the game wants
                // map-wide properties on it, so its absence is not worth reporting.
                if (classname != "worldspawn" && missing.insert(classname).second) {
                    TraceLog(LOG_WARNING, "ENTITY: map wants '%s', which has no script in content://entities/",
                        classname.c_str());
                }

                continue;
            }

            std::unique_ptr<Entity> instance = std::make_unique<Entity>(*this, *definition, this->next_id++);
            instance->configure(entity.keyvalues, entity.connections);

            this->entities.push_back(std::move(instance));
            ++spawned;
        }

        for (size_t index = first_new; index < this->entities.size(); ++index) {
            this->entities[index]->spawn();
        }

        return spawned;
    }

    Entity* EntityWorld::spawn(const std::string& classname,
        const std::vector<MapKeyValue>& keyvalues,
        const std::vector<MapConnection>& connections)
    {
        const EntityDefinition* definition = EntityRegistry::find(classname);
        if (definition == nullptr) {
            TraceLog(LOG_WARNING, "ENTITY: cannot spawn '%s': no script in content://entities/", classname.c_str());
            return nullptr;
        }

        std::unique_ptr<Entity> instance = std::make_unique<Entity>(*this, *definition, this->next_id++);
        instance->configure(keyvalues, connections);

        Entity* raw = instance.get();
        this->entities.push_back(std::move(instance));

        raw->spawn();
        return raw;
    }

    void EntityWorld::update(float delta)
    {
        // Indexed rather than iterated: an entity's OnUpdate may spawn another, and that
        // would invalidate an iterator. New entities spawn already having run OnSpawn, and
        // are picked up by the next frame's tick.
        size_t live = this->entities.size();

        for (size_t index = 0; index < live && index < this->entities.size(); ++index) {
            if (this->entities[index]->is_alive()) {
                this->entities[index]->update(delta);
            }
        }

        for (PendingInput& input : this->pending) {
            input.remaining -= delta;
        }

        // Inputs that come due can fire outputs of their own, which queue further inputs. A
        // chain with no delays should resolve within the same frame, so this keeps going
        // until nothing more is ready.
        int delivered = 0;

        while (delivered < MAX_INPUTS_PER_FRAME) {
            std::vector<PendingInput> ready;

            // Taken out of the queue before any are delivered, so that inputs queued while
            // delivering land in the queue rather than in the batch being walked.
            for (size_t index = 0; index < this->pending.size();) {
                if (this->pending[index].remaining <= 0.0f) {
                    ready.push_back(this->pending[index]);
                    this->pending.erase(this->pending.begin() + static_cast<long>(index));
                } else {
                    ++index;
                }
            }

            if (ready.empty()) {
                break;
            }

            for (const PendingInput& input : ready) {
                Entity* target = this->find_by_id(input.target);

                // The target may well have destroyed itself between the output firing and
                // the input coming due, which is ordinary rather than exceptional.
                if (target != nullptr && target->is_alive()) {
                    target->accept_input(input.input, input.parameter, input.activator);
                }

                ++delivered;
            }
        }

        if (delivered >= MAX_INPUTS_PER_FRAME) {
            TraceLog(LOG_WARNING, "ENTITY: hit the %d input limit in one frame - check for entities"
                " wired into a loop with no delay", MAX_INPUTS_PER_FRAME);
        }

        // Destroyed entities are cleared out only here, at the end of the frame, so that
        // nothing being walked above has the ground pulled out from under it.
        this->entities.erase(
            std::remove_if(this->entities.begin(), this->entities.end(),
                [](const std::unique_ptr<Entity>& entity) { return !entity->is_alive(); }),
            this->entities.end());
    }

    void EntityWorld::clear()
    {
        this->pending.clear();
        this->entities.clear();
        this->next_id = 1;
    }

    Entity* EntityWorld::find_by_id(int id)
    {
        for (const std::unique_ptr<Entity>& entity : this->entities) {
            if (entity->get_id() == id) {
                return entity.get();
            }
        }

        return nullptr;
    }

    Entity* EntityWorld::find_by_targetname(const std::string& name)
    {
        if (name.empty()) {
            return nullptr;
        }

        for (const std::unique_ptr<Entity>& entity : this->entities) {
            if (entity->get_targetname() == name) {
                return entity.get();
            }
        }

        return nullptr;
    }

    std::vector<Entity*> EntityWorld::find_all_by_targetname(const std::string& name)
    {
        std::vector<Entity*> found;

        if (name.empty()) {
            return found;
        }

        for (const std::unique_ptr<Entity>& entity : this->entities) {
            if (entity->get_targetname() == name) {
                found.push_back(entity.get());
            }
        }

        return found;
    }

    std::vector<Entity*> EntityWorld::find_by_classname(const std::string& classname)
    {
        std::vector<Entity*> found;

        for (const std::unique_ptr<Entity>& entity : this->entities) {
            if (entity->get_classname() == classname) {
                found.push_back(entity.get());
            }
        }

        return found;
    }

    std::vector<Entity*> EntityWorld::all()
    {
        std::vector<Entity*> found;
        found.reserve(this->entities.size());

        for (const std::unique_ptr<Entity>& entity : this->entities) {
            found.push_back(entity.get());
        }

        return found;
    }

    int EntityWorld::count() const
    {
        return static_cast<int>(this->entities.size());
    }

    int EntityWorld::pending_input_count() const
    {
        return static_cast<int>(this->pending.size());
    }

    void EntityWorld::dispatch_output(Entity& source, const std::string& output, const std::string& parameter)
    {
        const std::vector<MapConnection>& connections = source.get_connections();

        for (size_t index = 0; index < connections.size(); ++index) {
            const MapConnection& connection = connections[index];

            if (connection.output != output) {
                continue;
            }

            // A connection with a limit goes dead once it is used up, which is how a mapper
            // makes something happen only the first time.
            int fired = index < source.get_fire_counts().size() ? source.get_fire_counts()[index] : 0;
            if (connection.times_to_fire >= 0 && fired >= connection.times_to_fire) {
                continue;
            }

            // The connection's own parameter wins when it has one; otherwise whatever the
            // output sent is passed along.
            std::string value = connection.parameter.empty() ? parameter : connection.parameter;

            std::vector<Entity*> targets;

            if (connection.target == TARGET_SELF) {
                targets.push_back(&source);
            } else if (connection.target == TARGET_ACTIVATOR) {
                Entity* activator = this->find_by_id(source.get_activator());
                if (activator != nullptr) {
                    targets.push_back(activator);
                }
            } else {
                targets = this->find_all_by_targetname(connection.target);
            }

            if (targets.empty()) {
                TraceLog(LOG_WARNING, "ENTITY: '%s' fired %s at '%s', which no entity answers to",
                    source.get_classname().c_str(), output.c_str(), connection.target.c_str());
                continue;
            }

            // Counted as fired even if several entities share the name, so a "once only"
            // connection means one firing rather than one per listener.
            source.note_connection_fired(static_cast<int>(index));

            for (Entity* target : targets) {
                this->queue_input(target->get_id(), connection.input, value,
                    source.get_id(), connection.delay);
            }
        }
    }

    void EntityWorld::queue_input(int target, const std::string& input, const std::string& parameter,
        int activator, float delay)
    {
        PendingInput queued;
        queued.target = target;
        queued.activator = activator;
        queued.input = input;
        queued.parameter = parameter;
        queued.remaining = delay;

        this->pending.push_back(std::move(queued));
    }
}
