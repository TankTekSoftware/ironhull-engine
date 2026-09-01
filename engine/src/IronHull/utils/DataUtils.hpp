#pragma once

#include "box2d/math_functions.h"
#include "raylib.h"
namespace IronHull
{
    class DataUtils
    {
        public:
            static unsigned char float_to_byte(float value);
            static b2Vec2 to_box2d(Vector2 pixels);
            static Vector2 from_box2d(b2Vec2 units);
    };
}
