#pragma once

#include "IronHull/nodes/CollisionShape2D.hpp"
#include "IronHull/nodes/Node2D.hpp"
#include "box2d/id.h"
#include "box2d/types.h"
#include <vector>

namespace IronHull
{
    class PhysicsBody2D : public IronHull::Node2D
    {
        protected:
            b2BodyId body_id;
        public:
            void init(const std::vector<CollisionShape2D*>& shapes, b2BodyType type);
    };
}
