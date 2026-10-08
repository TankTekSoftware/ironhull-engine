#pragma once

#include "box3d/math_functions.h"
#include "raylib.h"

namespace IronHull
{
    class DataUtils
    {
        public:
            static unsigned char float_to_byte(float value);
            static b3Vec3 to_box3d(Vector3 v);
            static Vector3 from_box3d(b3Vec3 v);
            static b3Quat to_box3d(Quaternion q);
            static Quaternion from_box3d(b3Quat q);
    };
}
