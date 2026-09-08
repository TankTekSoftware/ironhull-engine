#pragma once

#include "IronHull/nodes/CollisionShape2D.hpp"
#include "IronHull/nodes/PhysicsBody2D.hpp"
#include <vector>

namespace IronHull
{
    class StaticBody2D : public PhysicsBody2D
    {
        private:
            std::vector<CollisionShape2D> collision_shapes;
        protected:
            void init(const std::vector<CollisionShape2D>& shapes);
    };
}
