#pragma once

#include "IronHull/nodes/CollisionShape3D.hpp"
#include "IronHull/nodes/PhysicsBody3D.hpp"
#include <vector>

namespace IronHull
{
    class StaticBody3D : public PhysicsBody3D
    {
        private:
            std::vector<CollisionShape3D> collision_shapes;
        protected:
            void init(const std::vector<CollisionShape3D>& shapes);
    };
}
