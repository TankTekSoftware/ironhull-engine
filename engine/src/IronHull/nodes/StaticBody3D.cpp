#include "IronHull/nodes/CollisionShape3D.hpp"
#include "IronHull/nodes/PhysicsBody3D.hpp"
#include "box3d/types.h"
#include <IronHull/nodes/StaticBody3D.hpp>

namespace IronHull
{
    void StaticBody3D::init(const std::vector<CollisionShape3D>& shapes)
    {
        this->collision_shapes = shapes;

        std::vector<CollisionShape3D*> shapePtrs;
        for (auto& s : this->collision_shapes) {
            shapePtrs.push_back(&s);
        }

        PhysicsBody3D::init(shapePtrs, b3_staticBody);
    }
}
