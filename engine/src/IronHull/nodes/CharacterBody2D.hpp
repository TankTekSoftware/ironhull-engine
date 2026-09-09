#pragma once

#include "IronHull/nodes/CollisionShape2D.hpp"
#include "IronHull/nodes/PhysicsBody2D.hpp"
#include "raylib.h"
#include <vector>

namespace IronHull
{
    class CharacterBody2D : public PhysicsBody2D 
    {
        private:
            std::vector<CollisionShape2D> collision_shapes;
            // The velocity handed to Box2D on the previous move_and_slide, kept
            // so the next one can tell what the solver changed.
            Vector2 requested_velocity = { 0.0f, 0.0f };
        public:
            Vector2 velocity = { 0.0f, 0.0f };
        protected:
            void init(const std::vector<CollisionShape2D>& shapes);
            void move_and_slide(float delta);
    };   
}
