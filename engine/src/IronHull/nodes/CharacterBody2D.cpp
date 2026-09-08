#include "IronHull/nodes/CollisionShape2D.hpp"
#include "IronHull/nodes/PhysicsBody2D.hpp"
#include "IronHull/utils/DataUtils.hpp"
#include "box2d/box2d.h"
#include "box2d/types.h"
#include <IronHull/nodes/CharacterBody2D.hpp>

#include <raymath.h>

namespace IronHull
{
    void CharacterBody2D::init(const std::vector<CollisionShape2D>& shapes)
    {
        this->collision_shapes = shapes;

        std::vector<CollisionShape2D*> shapePtrs;
        for (auto& s : this->collision_shapes) {
            shapePtrs.push_back(&s);
        }

        PhysicsBody2D::init(shapePtrs, b2_dynamicBody);

        b2Body_SetFixedRotation(body_id, true);
    }

    void CharacterBody2D::move_and_slide(float delta)
    {
        // Hand the desired velocity to the Box2D body rather than integrating a
        // position that's disconnected from the physics world. Letting Box2D's
        // solver resolve the motion means a blocked axis actually stops there
        // instead of the character continuing to accumulate velocity into
        // whatever it hit.
        b2Body_SetLinearVelocity(this->body_id, DataUtils::to_box2d(this->velocity));

        // Pull the resolved (post-collision) transform and velocity back out so
        // callers see what actually happened, not what was requested.
        this->position = DataUtils::from_box2d(b2Body_GetPosition(this->body_id));
        this->velocity = DataUtils::from_box2d(b2Body_GetLinearVelocity(this->body_id));
    }
}
