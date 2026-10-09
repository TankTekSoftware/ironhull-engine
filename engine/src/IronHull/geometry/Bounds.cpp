#include <IronHull/geometry/Bounds.hpp>

#include <algorithm>
#include <limits>

namespace IronHull
{
    BoundingBox Bounds::empty()
    {
        constexpr float infinity = std::numeric_limits<float>::max();

        return BoundingBox{
            { infinity, infinity, infinity },
            { -infinity, -infinity, -infinity },
        };
    }

    bool Bounds::is_empty(const BoundingBox& box)
    {
        return box.min.x > box.max.x || box.min.y > box.max.y || box.min.z > box.max.z;
    }

    BoundingBox Bounds::from_points(const std::vector<Vector3>& points)
    {
        BoundingBox box = Bounds::empty();

        for (Vector3 point : points) {
            box = Bounds::add_point(box, point);
        }

        return box;
    }

    BoundingBox Bounds::add_point(const BoundingBox& box, Vector3 point)
    {
        return BoundingBox{
            {
                std::min(box.min.x, point.x),
                std::min(box.min.y, point.y),
                std::min(box.min.z, point.z),
            },
            {
                std::max(box.max.x, point.x),
                std::max(box.max.y, point.y),
                std::max(box.max.z, point.z),
            },
        };
    }

    BoundingBox Bounds::merge(const BoundingBox& a, const BoundingBox& b)
    {
        return BoundingBox{
            {
                std::min(a.min.x, b.min.x),
                std::min(a.min.y, b.min.y),
                std::min(a.min.z, b.min.z),
            },
            {
                std::max(a.max.x, b.max.x),
                std::max(a.max.y, b.max.y),
                std::max(a.max.z, b.max.z),
            },
        };
    }

    BoundingBox Bounds::expanded(const BoundingBox& box, float amount)
    {
        return BoundingBox{
            { box.min.x - amount, box.min.y - amount, box.min.z - amount },
            { box.max.x + amount, box.max.y + amount, box.max.z + amount },
        };
    }

    bool Bounds::overlaps(const BoundingBox& a, const BoundingBox& b, float epsilon)
    {
        return a.min.x <= b.max.x + epsilon && a.max.x >= b.min.x - epsilon
            && a.min.y <= b.max.y + epsilon && a.max.y >= b.min.y - epsilon
            && a.min.z <= b.max.z + epsilon && a.max.z >= b.min.z - epsilon;
    }

    bool Bounds::contains(const BoundingBox& box, Vector3 point, float epsilon)
    {
        return point.x >= box.min.x - epsilon && point.x <= box.max.x + epsilon
            && point.y >= box.min.y - epsilon && point.y <= box.max.y + epsilon
            && point.z >= box.min.z - epsilon && point.z <= box.max.z + epsilon;
    }

    Vector3 Bounds::center(const BoundingBox& box)
    {
        return Vector3{
            (box.min.x + box.max.x) * 0.5f,
            (box.min.y + box.max.y) * 0.5f,
            (box.min.z + box.max.z) * 0.5f,
        };
    }

    Vector3 Bounds::size(const BoundingBox& box)
    {
        return Vector3{
            box.max.x - box.min.x,
            box.max.y - box.min.y,
            box.max.z - box.min.z,
        };
    }
}
