#pragma once

#include "raylib.h"

namespace IronHull
{
    class Node3D
    {
        public:
            Vector3 position = { 0.0f, 0.0f, 0.0f };
            Quaternion rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
            Vector3 scale = { 1.0f, 1.0f, 1.0f };
    };
}
