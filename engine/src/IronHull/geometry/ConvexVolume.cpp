#include <IronHull/geometry/ConvexVolume.hpp>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>

namespace IronHull
{
    ConvexVolume ConvexVolume::from_bounds(const BoundingBox& box)
    {
        ConvexVolume volume;
        volume.planes.reserve(6);

        volume.planes.push_back(Plane{ { 1.0f, 0.0f, 0.0f }, box.max.x });
        volume.planes.push_back(Plane{ { -1.0f, 0.0f, 0.0f }, -box.min.x });
        volume.planes.push_back(Plane{ { 0.0f, 1.0f, 0.0f }, box.max.y });
        volume.planes.push_back(Plane{ { 0.0f, -1.0f, 0.0f }, -box.min.y });
        volume.planes.push_back(Plane{ { 0.0f, 0.0f, 1.0f }, box.max.z });
        volume.planes.push_back(Plane{ { 0.0f, 0.0f, -1.0f }, -box.min.z });

        return volume;
    }

    ConvexVolume ConvexVolume::split(const Plane& plane, PlaneSide keep) const
    {
        ConvexVolume volume;
        volume.planes = this->planes;
        volume.planes.push_back(keep == PlaneSide::FRONT ? plane.flipped() : plane);

        return volume;
    }

    std::vector<Winding> ConvexVolume::faces() const
    {
        std::vector<Winding> windings(this->planes.size());

        for (size_t index = 0; index < this->planes.size(); ++index) {
            Winding winding = Winding::from_plane(this->planes[index]);

            // Cut the plane back with every other half-space. What is left is the part of
            // this plane that actually bounds the region.
            for (size_t other = 0; other < this->planes.size() && winding.points.size() >= 3; ++other) {
                if (other == index) {
                    continue;
                }

                winding = winding.clipped(this->planes[other], PlaneSide::BACK);
            }

            winding.simplify();

            if (winding.is_valid()) {
                windings[index] = winding;
            }
        }

        return windings;
    }

    std::vector<Vector3> ConvexVolume::corners() const
    {
        std::vector<Vector3> points;

        for (const Winding& winding : this->faces()) {
            points.insert(points.end(), winding.points.begin(), winding.points.end());
        }

        return points;
    }

    bool ConvexVolume::is_empty() const
    {
        int valid = 0;

        for (const Winding& winding : this->faces()) {
            if (winding.is_valid()) {
                ++valid;
            }
        }

        // It takes four bounding faces to enclose any volume at all; fewer means the planes
        // have cut each other down to a sliver, an open region, or nothing.
        return valid < 4;
    }

    BoundingBox ConvexVolume::bounds() const
    {
        return Bounds::from_points(this->corners());
    }

    Vector3 ConvexVolume::interior_point() const
    {
        std::vector<Vector3> points = this->corners();

        Vector3 total = { 0.0f, 0.0f, 0.0f };
        if (points.empty()) {
            return total;
        }

        for (Vector3 point : points) {
            total = Vector3Add(total, point);
        }

        return Vector3Scale(total, 1.0f / static_cast<float>(points.size()));
    }

    bool ConvexVolume::contains(Vector3 point, float epsilon) const
    {
        for (const Plane& plane : this->planes) {
            if (plane.distance_to(point) > epsilon) {
                return false;
            }
        }

        return true;
    }

    ConvexVolume ConvexVolume::simplified() const
    {
        std::vector<Winding> windings = this->faces();

        ConvexVolume volume;
        volume.planes.reserve(this->planes.size());

        for (size_t index = 0; index < this->planes.size(); ++index) {
            if (windings[index].is_valid()) {
                volume.planes.push_back(this->planes[index]);
            }
        }

        return volume;
    }
}
