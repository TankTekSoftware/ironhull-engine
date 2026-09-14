#pragma once

#include "IronHull/nodes/Node3D.hpp"

namespace IronHull
{
    enum class CollisionShapeType3D
    {
        BOX,
        SPHERE,
        CAPSULE
    };

    // position/rotation are the shape's offset from the body it's attached to.
    class CollisionShape3D : public Node3D
    {
        public:
            CollisionShapeType3D shape_type = CollisionShapeType3D::BOX;
        public:
            Vector3 extends = { 1.0f, 1.0f, 1.0f };    // (BOX) full size, in meters
            float radius = 0.5f;                        // (SPHERE/CAPSULE)
            float length = 1.0f;                        // (CAPSULE) distance between hemisphere centers, along local Y
        public:
            float density = 1.0f;
            float friction = 0.3f;
    };
}
