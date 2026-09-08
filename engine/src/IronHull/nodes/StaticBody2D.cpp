#include "IronHull/nodes/CollisionShape2D.hpp"
#include "IronHull/nodes/PhysicsBody2D.hpp"
#include "box2d/types.h"
#include <IronHull/nodes/StaticBody2D.hpp>

namespace IronHull
{
    void StaticBody2D::init(const std::vector<CollisionShape2D>& shapes)
    {
        this->collision_shapes = shapes;

        std::vector<CollisionShape2D*> shapePtrs;
        for (auto& s : this->collision_shapes) {
            shapePtrs.push_back(&s);
        }

        PhysicsBody2D::init(shapePtrs, b2_staticBody);
    }
}
