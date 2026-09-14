#pragma once

#include "IronHull/nodes/CollisionShape3D.hpp"
#include "IronHull/nodes/PhysicsBody3D.hpp"
#include "raylib.h"
#include <vector>

namespace IronHull
{
    class CharacterBody3D : public PhysicsBody3D
    {
        private:
            std::vector<CollisionShape3D> collision_shapes;
            // The velocity handed to Box3D on the previous move_and_slide, kept
            // so the next one can tell what the solver changed.
            Vector3 requested_velocity = { 0.0f, 0.0f, 0.0f };
        public:
            Vector3 velocity = { 0.0f, 0.0f, 0.0f };
        protected:
            void init(const std::vector<CollisionShape3D>& shapes);
            void move_and_slide(float delta);
    };
}
