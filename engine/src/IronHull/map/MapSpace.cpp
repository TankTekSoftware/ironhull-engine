#include <IronHull/map/MapSpace.hpp>

#include <cmath>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>

namespace IronHull
{
    namespace
    {
        // The Quake angle convention, evaluated in authored Z-up space: yaw turns about Z,
        // pitch tips down about the horizontal, roll spins about the facing direction. Note
        // that positive pitch points downwards, which is a quirk of the original convention
        // that every map editor has inherited and so has to be reproduced exactly.
        void angle_vectors(Vector3 angles, Vector3& forward, Vector3& right, Vector3& up)
        {
            float pitch = angles.x * DEG2RAD;
            float yaw = angles.y * DEG2RAD;
            float roll = angles.z * DEG2RAD;

            float sin_pitch = std::sin(pitch);
            float cos_pitch = std::cos(pitch);
            float sin_yaw = std::sin(yaw);
            float cos_yaw = std::cos(yaw);
            float sin_roll = std::sin(roll);
            float cos_roll = std::cos(roll);

            forward = {
                cos_pitch * cos_yaw,
                cos_pitch * sin_yaw,
                -sin_pitch,
            };

            right = {
                -sin_roll * sin_pitch * cos_yaw + cos_roll * sin_yaw,
                -sin_roll * sin_pitch * sin_yaw - cos_roll * cos_yaw,
                -sin_roll * cos_pitch,
            };

            up = {
                cos_roll * sin_pitch * cos_yaw + sin_roll * sin_yaw,
                cos_roll * sin_pitch * sin_yaw - sin_roll * cos_yaw,
                cos_roll * cos_pitch,
            };
        }
    }

    Vector3 MapSpace::to_engine(Vector3 point, float scale)
    {
        return Vector3{ point.x * scale, point.z * scale, -point.y * scale };
    }

    Vector3 MapSpace::to_map(Vector3 point, float scale)
    {
        return Vector3{ point.x / scale, -point.z / scale, point.y / scale };
    }

    Plane MapSpace::to_engine(const Plane& plane, float scale)
    {
        Plane converted;
        converted.normal = Vector3{ plane.normal.x, plane.normal.z, -plane.normal.y };
        converted.dist = plane.dist * scale;
        converted.snap();

        return converted;
    }

    BoundingBox MapSpace::bounds_to_engine(const BoundingBox& box, float scale)
    {
        // Rotating the two corners swaps which is the minimum on the axes that got negated,
        // so the result has to be rebuilt from both rather than converted in place.
        BoundingBox converted = Bounds::empty();
        converted = Bounds::add_point(converted, MapSpace::to_engine(box.min, scale));
        converted = Bounds::add_point(converted, MapSpace::to_engine(box.max, scale));

        return converted;
    }

    Vector3 MapSpace::angles_to_forward(Vector3 angles)
    {
        Vector3 forward;
        Vector3 right;
        Vector3 up;
        angle_vectors(angles, forward, right, up);

        return MapSpace::to_engine(forward);
    }

    Vector3 MapSpace::angles_to_right(Vector3 angles)
    {
        Vector3 forward;
        Vector3 right;
        Vector3 up;
        angle_vectors(angles, forward, right, up);

        return MapSpace::to_engine(right);
    }

    Vector3 MapSpace::angles_to_up(Vector3 angles)
    {
        Vector3 forward;
        Vector3 right;
        Vector3 up;
        angle_vectors(angles, forward, right, up);

        return MapSpace::to_engine(up);
    }
}
