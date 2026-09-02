#include "IronHull/core/Application.hpp"
#include "IronHull/nodes/PhysicsBody2D.hpp"
#include "box2d/box2d.h"
#include "box2d/id.h"
#include "box2d/math_functions.h"
#include "box2d/types.h"
#include "raylib.h"
#include <IronHull/nodes/PhysicsWorld2D.hpp>

namespace IronHull
{
    PhysicsWorld2D* PhysicsWorld2D::instance = nullptr;

    PhysicsWorld2D* PhysicsWorld2D::get_singleton()
    {
        return instance;
    }

    void PhysicsWorld2D::compose()
    {
        b2WorldDef world_def = b2DefaultWorldDef();
        Vector2 gravity = Application::get_physics2d().gravity;
        world_def.gravity = (b2Vec2){ gravity.x, gravity.y };
        this->world_id = b2CreateWorld(&world_def);

        PhysicsWorld2D::instance = this;
    }

    void PhysicsWorld2D::step(float delta)
    {
        const float time_step = 1.0f / 60.0f;
        const int sub_step_count = 4;

        accumulator += delta;
        while(accumulator >= time_step) {
            b2World_Step(this->world_id, time_step, sub_step_count);
            this->process_contact_events();
            accumulator -= time_step;
        }
    }

    void PhysicsWorld2D::dispose()
    {
        b2DestroyWorld(this->world_id);
        if (PhysicsWorld2D::instance) {
            PhysicsWorld2D::instance = nullptr;
        }
    }

    b2WorldId PhysicsWorld2D::get_world_id() const
    {
        return this->world_id;
    }

    void PhysicsWorld2D::process_contact_events()
    {
        b2ContactEvents events = b2World_GetContactEvents(this->world_id);

        for (int i = 0; i < events.beginCount; ++i) {
            const b2ContactBeginTouchEvent& e = events.beginEvents[i];
            auto* a = static_cast<PhysicsBody2D*>(b2Body_GetUserData(b2Shape_GetBody(e.shapeIdA)));
            auto* b = static_cast<PhysicsBody2D*>(b2Body_GetUserData(b2Shape_GetBody(e.shapeIdB)));

            if (a != b) {
                if (a) a->on_collision_enter(b);
                if (b) b->on_collision_enter(a);
            }
        }

        for (int i = 0; i < events.endCount; ++i) {
            const b2ContactEndTouchEvent& e = events.endEvents[i];
            auto* a = static_cast<PhysicsBody2D*>(b2Body_GetUserData(b2Shape_GetBody(e.shapeIdA)));
            auto* b = static_cast<PhysicsBody2D*>(b2Body_GetUserData(b2Shape_GetBody(e.shapeIdB)));

            if (a != b) {
                if (a) a->on_collision_exit(b);
                if (b) b->on_collision_exit(a);
            }
        }
    }


}
