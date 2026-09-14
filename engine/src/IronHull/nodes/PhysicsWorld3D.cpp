#include "IronHull/core/Application.hpp"
#include "IronHull/nodes/PhysicsBody3D.hpp"
#include "IronHull/utils/DataUtils.hpp"
#include "box3d/box3d.h"
#include "box3d/id.h"
#include "box3d/types.h"
#include <IronHull/nodes/PhysicsWorld3D.hpp>

namespace IronHull
{
    PhysicsWorld3D* PhysicsWorld3D::instance = nullptr;

    PhysicsWorld3D* PhysicsWorld3D::get_singleton()
    {
        return instance;
    }

    void PhysicsWorld3D::compose()
    {
        b3WorldDef world_def = b3DefaultWorldDef();
        world_def.gravity = DataUtils::to_box3d(Application::get_physics3d().gravity);
        this->world_id = b3CreateWorld(&world_def);

        PhysicsWorld3D::instance = this;
    }

    void PhysicsWorld3D::step(float delta)
    {
        const float time_step = 1.0f / 60.0f;
        const int sub_step_count = 4;

        accumulator += delta;
        while (accumulator >= time_step) {
            b3World_Step(this->world_id, time_step, sub_step_count);
            this->process_contact_events();
            accumulator -= time_step;
        }
    }

    void PhysicsWorld3D::dispose()
    {
        b3DestroyWorld(this->world_id);
        if (PhysicsWorld3D::instance) {
            PhysicsWorld3D::instance = nullptr;
        }
    }

    b3WorldId PhysicsWorld3D::get_world_id() const
    {
        return this->world_id;
    }

    void PhysicsWorld3D::process_contact_events()
    {
        b3ContactEvents events = b3World_GetContactEvents(this->world_id);

        for (int i = 0; i < events.beginCount; ++i) {
            const b3ContactBeginTouchEvent& e = events.beginEvents[i];
            auto* a = static_cast<PhysicsBody3D*>(b3Body_GetUserData(b3Shape_GetBody(e.shapeIdA)));
            auto* b = static_cast<PhysicsBody3D*>(b3Body_GetUserData(b3Shape_GetBody(e.shapeIdB)));

            if (a != b) {
                if (a) a->on_collision_enter(b);
                if (b) b->on_collision_enter(a);
            }
        }

        for (int i = 0; i < events.endCount; ++i) {
            const b3ContactEndTouchEvent& e = events.endEvents[i];

            // End events can refer to shapes destroyed since the last step.
            if (!b3Shape_IsValid(e.shapeIdA) || !b3Shape_IsValid(e.shapeIdB)) {
                continue;
            }

            auto* a = static_cast<PhysicsBody3D*>(b3Body_GetUserData(b3Shape_GetBody(e.shapeIdA)));
            auto* b = static_cast<PhysicsBody3D*>(b3Body_GetUserData(b3Shape_GetBody(e.shapeIdB)));

            if (a != b) {
                if (a) a->on_collision_exit(b);
                if (b) b->on_collision_exit(a);
            }
        }
    }
}
