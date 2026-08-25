#include "IronHull/utils/DataUtils.hpp"
#include "raylib.h"
#include <IronHull/utils/ColorUtils.hpp>

namespace IronHull
{
    Color ColorUtils::to_rlcolor(float r, float g, float b, float a)
    {
        unsigned char red = DataUtils::float_to_byte(red);
        unsigned char green = DataUtils::float_to_byte(green);
        unsigned char blue = DataUtils::float_to_byte(blue);
        unsigned char alpha = DataUtils::float_to_byte(alpha);

        return (Color){ red, green, blue, alpha };
    }
}
