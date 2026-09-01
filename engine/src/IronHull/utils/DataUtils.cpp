#include "IronHull/core/Application.hpp"
#include "box2d/math_functions.h"
#include <IronHull/utils/DataUtils.hpp>
#include <algorithm>

namespace IronHull
{
    unsigned char DataUtils::float_to_byte(float value)
    {
        int clamped = std::clamp(static_cast<int>(value * 255.0f + 0.5f), 0, 255);
        return static_cast<unsigned char>(clamped);
    }

    b2Vec2 DataUtils::to_box2d(Vector2 pixels)
    {
        const float pixels_per_unit = Application::get_physics2d().pixels_per_unit; 
        return b2Vec2{ pixels.x / pixels_per_unit, pixels.y / pixels_per_unit };
    }

    Vector2 DataUtils::from_box2d(b2Vec2 units)
    {
        const float pixels_per_unit = Application::get_physics2d().pixels_per_unit;

        return Vector2{ units.x * pixels_per_unit, units.y * pixels_per_unit };
    }
}
