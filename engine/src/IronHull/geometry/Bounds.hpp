#pragma once

#include <vector>

#include <raylib.h>

namespace IronHull
{
    // Axis-aligned bounding box helpers on top of raylib's BoundingBox, which is a bare pair
    // of corners with no operations of its own.
    //
    // An "empty" box is represented inverted (min above max) rather than zero-sized, so that
    // growing one point at a time from empty() lands on exactly the points added instead of
    // always swallowing the origin.
    class Bounds
    {
        public:
            static BoundingBox empty();
            static bool is_empty(const BoundingBox& box);

        public:
            static BoundingBox from_points(const std::vector<Vector3>& points);
            static BoundingBox add_point(const BoundingBox& box, Vector3 point);
            static BoundingBox merge(const BoundingBox& a, const BoundingBox& b);
            static BoundingBox expanded(const BoundingBox& box, float amount);

        public:
            static bool overlaps(const BoundingBox& a, const BoundingBox& b, float epsilon = 0.0f);
            static bool contains(const BoundingBox& box, Vector3 point, float epsilon = 0.0f);

        public:
            static Vector3 center(const BoundingBox& box);
            static Vector3 size(const BoundingBox& box);
    };
}
