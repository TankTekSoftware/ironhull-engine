#include "IronHull/nodes/CollisionShape3D.hpp"
#include "IronHull/nodes/PhysicsBody3D.hpp"
#include "IronHull/utils/DataUtils.hpp"
#include "box3d/box3d.h"
#include "box3d/types.h"
#include <IronHull/nodes/CharacterBody3D.hpp>

#include <raymath.h>

namespace IronHull
{
    void CharacterBody3D::init(const std::vector<CollisionShape3D>& shapes)
    {
        this->collision_shapes = shapes;

        std::vector<CollisionShape3D*> shapePtrs;
        for (auto& s : this->collision_shapes) {
            shapePtrs.push_back(&s);
        }

        PhysicsBody3D::init(shapePtrs, b3_dynamicBody);

        // Box3D's equivalent of Box2D's fixed rotation: a character shouldn't
        // tip over or spin when it brushes past something.
        b3MotionLocks locks = { 0 };
        locks.angularX = true;
        locks.angularY = true;
        locks.angularZ = true;
        b3Body_SetMotionLocks(this->body_id, locks);
    }

    void CharacterBody3D::move_and_slide(float delta)
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
        Vector3 resolved = DataUtils::from_box3d(b3Body_GetLinearVelocity(this->body_id));
        this->velocity = Vector3Add(this->velocity, Vector3Subtract(resolved, this->requested_velocity));

        // Hand the desired velocity to the Box3D body rather than integrating a
        // position that's disconnected from the physics world. Letting Box3D's
        // solver resolve the motion means a blocked axis actually stops there
        // instead of the character continuing to accumulate velocity into
        // whatever it hit.
        b3Body_SetLinearVelocity(this->body_id, DataUtils::to_box3d(this->velocity));
        this->requested_velocity = this->velocity;

        // Position is likewise the post-step one, so callers and the renderer
        // see where the character actually ended up.
        this->position = DataUtils::from_box3d(b3ToVec3(b3Body_GetPosition(this->body_id)));
    }
}
