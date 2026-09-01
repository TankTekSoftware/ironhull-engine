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
        public:
            Vector2 velocity;
        protected:
            void init(const std::vector<CollisionShape2D>& shapes);
            void move_and_slide(float delta);
    };   
}
