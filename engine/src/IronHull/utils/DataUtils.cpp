#include "IronHull/core/Application.hpp"
#include "box3d/math_functions.h"
#include <IronHull/utils/DataUtils.hpp>
#include <algorithm>

namespace IronHull
{
    unsigned char DataUtils::float_to_byte(float value)
    {
        int clamped = std::clamp(static_cast<int>(value * 255.0f + 0.5f), 0, 255);
        return static_cast<unsigned char>(clamped);
    }
    
    // Box3D and raylib 3D both work in meters, so unlike Box2D there's no
    // pixels-per-unit scaling - just a change of struct layout.
    b3Vec3 DataUtils::to_box3d(Vector3 v)
    {
        return b3Vec3{ v.x, v.y, v.z };
    }

    Vector3 DataUtils::from_box3d(b3Vec3 v)
    {
        return Vector3{ v.x, v.y, v.z };
    }

    b3Quat DataUtils::to_box3d(Quaternion q)
    {
        return b3Quat{ { q.x, q.y, q.z }, q.w };
    }

    Quaternion DataUtils::from_box3d(b3Quat q)
    {
        return Quaternion{ q.v.x, q.v.y, q.v.z, q.s };
    }
}
