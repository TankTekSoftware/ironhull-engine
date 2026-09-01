#pragma once

#include "IronHull/nodes/Node2D.hpp"

namespace IronHull
{
    enum class CollisionShapeType2D 
    {
        BOX,
        CIRCLE,
        CAPSULE
    };

    class CollisionShape2D : public Node2D
    {
        public:
            CollisionShapeType2D shape_type = CollisionShapeType2D::BOX;
        public: 
            Vector2 extends;    // (BOX)
            float radius;       // (CIRCLE/CAPSULE)
            float length;       // (CAPSULE)
        public:
            float density = 1.0f;
            float friction = 0.3f;
    };

}
