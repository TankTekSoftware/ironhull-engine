#include "IronHull/nodes/CollisionShape2D.hpp"
#include "IronHull/nodes/PhysicsBody2D.hpp"
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
        this->position = Vector2Add(this->position, Vector2Scale(this->velocity, delta));
    }
}
