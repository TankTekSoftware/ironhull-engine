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
        // The solver only runs during the world step, which happens after every
        // node has updated - so what sits on the body right now is the *result*
        // of the previous step, not of the velocity we're about to hand over.
        // Reconcile with it first: whatever the solver took off what we last
        // asked for (a blocked axis, friction, a bounce) comes off the
        // accumulator too, leaving the input this frame added untouched.
        //
        // Without this, `velocity` is write-only and a character held against a
        // wall keeps piling up speed it never gets to spend. Turning around then
        // has to burn all of that off before the character moves at all.
        Vector2 resolved = DataUtils::from_box2d(b2Body_GetLinearVelocity(this->body_id));
        this->velocity = Vector2Add(this->velocity, Vector2Subtract(resolved, this->requested_velocity));

        // Hand the desired velocity to the Box2D body rather than integrating a
        // position that's disconnected from the physics world. Letting Box2D's
        // solver resolve the motion means a blocked axis actually stops there
        // instead of the character continuing to accumulate velocity into
        // whatever it hit.
        b2Body_SetLinearVelocity(this->body_id, DataUtils::to_box2d(this->velocity));
        this->requested_velocity = this->velocity;

        // Position is likewise the post-step one, so callers and the renderer
        // see where the character actually ended up.
        this->position = DataUtils::from_box2d(b2Body_GetPosition(this->body_id));
    }
}
