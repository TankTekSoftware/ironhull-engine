#pragma once

#include "IronHull/nodes/CollisionShape3D.hpp"
#include "IronHull/nodes/Node3D.hpp"
#include "box3d/id.h"
#include "box3d/types.h"
#include <vector>

namespace IronHull
{
    class PhysicsBody3D : public IronHull::Node3D
    {
        protected:
            b3BodyId body_id;
        public:
            void init(const std::vector<CollisionShape3D*>& shapes, b3BodyType type);
        public:
            virtual void on_collision_enter(PhysicsBody3D* other) {}
            virtual void on_collision_exit(PhysicsBody3D* other) {}
    };
}
