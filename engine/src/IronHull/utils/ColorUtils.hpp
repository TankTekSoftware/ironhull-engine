#pragma once

#include "raylib.h"
namespace IronHull
{
    class ColorUtils
    {
        public:
            static Color to_rlcolor(float r, float g, float b, float a = 1.0f);
    };
}
