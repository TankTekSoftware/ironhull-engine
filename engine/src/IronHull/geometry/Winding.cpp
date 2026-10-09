#include <IronHull/geometry/Winding.hpp>

#include <cmath>
#include <stdexcept>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>

namespace IronHull
{
    namespace
    {
        // Below this a winding is treated as having no geometry at all. Clipping routinely
        // produces slivers this thin where a plane grazes a corner, and carrying them
        // forward only yields degenerate triangles and junk split planes.
        constexpr float MIN_WINDING_AREA = 0.001f;

        // Interpolates the crossing point of the edge p1 -> p2 against the plane.
        //
        // For an axial plane the coordinate along the plane normal is written exactly rather
        // than interpolated. The interpolated value is mathematically the same but lands a
        // rounding error or two off the plane, and since this same crossing is computed
        // independently for each of the two faces that share the edge, those errors are what
        // open visible cracks between neighbouring faces.
        Vector3 crossing_point(const Plane& plane, Vector3 p1, Vector3 p2, float d1, float d2)
        {
            float fraction = d1 / (d1 - d2);

            Vector3 mid;
            const float* normal[3] = { &plane.normal.x, &plane.normal.y, &plane.normal.z };
            const float* from[3] = { &p1.x, &p1.y, &p1.z };
            const float* to[3] = { &p2.x, &p2.y, &p2.z };
            float* out[3] = { &mid.x, &mid.y, &mid.z };

            for (int axis = 0; axis < 3; ++axis) {
                if (*normal[axis] == 1.0f) {
                    *out[axis] = plane.dist;
                } else if (*normal[axis] == -1.0f) {
                    *out[axis] = -plane.dist;
                } else {
                    *out[axis] = *from[axis] + fraction * (*to[axis] - *from[axis]);
                }
            }

            return mid;
        }
    }

    Winding Winding::from_plane(const Plane& plane, float radius)
    {
        // Pick the world axis least aligned with the normal to build an "up" from, so the
        // cross products below never collapse: using the most aligned axis would give a
        // near-zero tangent.
        int major_axis = 0;
        float largest = -1.0f;
        const float* components[3] = { &plane.normal.x, &plane.normal.y, &plane.normal.z };

        for (int axis = 0; axis < 3; ++axis) {
            float magnitude = std::fabs(*components[axis]);
            if (magnitude > largest) {
                largest = magnitude;
                major_axis = axis;
            }
        }

        Vector3 up = { 0.0f, 0.0f, 0.0f };
        if (major_axis == 2) {
            up.x = 1.0f;
        } else {
            up.z = 1.0f;
        }

        // Project the chosen axis into the plane so up lies in the plane itself.
        up = Vector3Normalize(Vector3Subtract(up, Vector3Scale(plane.normal, Vector3DotProduct(up, plane.normal))));

        // right = normal x up (rather than up x normal) so the quad below comes out wound
        // counter-clockwise seen from the front, which is this winding type's convention.
        Vector3 right = Vector3CrossProduct(plane.normal, up);

        Vector3 origin = Vector3Scale(plane.normal, plane.dist);
        up = Vector3Scale(up, radius);
        right = Vector3Scale(right, radius);

        Winding winding;
        winding.points.reserve(4);
        winding.points.push_back(Vector3Add(Vector3Subtract(origin, right), up));
        winding.points.push_back(Vector3Add(Vector3Add(origin, right), up));
        winding.points.push_back(Vector3Subtract(Vector3Add(origin, right), up));
        winding.points.push_back(Vector3Subtract(Vector3Subtract(origin, right), up));

        return winding;
    }

    bool Winding::is_valid() const
    {
        return this->points.size() >= 3 && this->area() > MIN_WINDING_AREA;
    }

    float Winding::area() const
    {
        if (this->points.size() < 3) {
            return 0.0f;
        }

        // Fan the polygon from its first point; valid for any convex winding.
        float total = 0.0f;
        for (size_t index = 2; index < this->points.size(); ++index) {
            Vector3 edge1 = Vector3Subtract(this->points[index - 1], this->points[0]);
            Vector3 edge2 = Vector3Subtract(this->points[index], this->points[0]);
            total += Vector3Length(Vector3CrossProduct(edge1, edge2)) * 0.5f;
        }

        return total;
    }

    Vector3 Winding::centroid() const
    {
        Vector3 total = { 0.0f, 0.0f, 0.0f };

        if (this->points.empty()) {
            return total;
        }

        // The plain average of the corners, not the area-weighted centroid. For a convex
        // polygon the average is guaranteed to land inside it, which is all this is used for
        // (picking a representative interior point), and it cannot be skewed by a long thin
        // tail the way a weighted centroid can.
        for (Vector3 point : this->points) {
            total = Vector3Add(total, point);
        }

        return Vector3Scale(total, 1.0f / static_cast<float>(this->points.size()));
    }

    BoundingBox Winding::bounds() const
    {
        return Bounds::from_points(this->points);
    }

    Plane Winding::plane() const
    {
        if (this->points.size() < 3) {
            return Plane{};
        }

        // Counter-clockwise winding: normal = (p1 - p0) x (p2 - p0).
        return Plane::from_normal_point(
            Vector3CrossProduct(
                Vector3Subtract(this->points[1], this->points[0]),
                Vector3Subtract(this->points[2], this->points[0])),
            this->points[0]);
    }

    Winding Winding::reversed() const
    {
        Winding winding;
        winding.points.assign(this->points.rbegin(), this->points.rend());

        return winding;
    }

    PlaneSide Winding::classify(const Plane& plane, float epsilon) const
    {
        bool has_front = false;
        bool has_back = false;

        for (Vector3 point : this->points) {
            PlaneSide side = plane.classify_point(point, epsilon);

            if (side == PlaneSide::FRONT) {
                has_front = true;
            } else if (side == PlaneSide::BACK) {
                has_back = true;
            }
        }

        if (has_front && has_back) {
            return PlaneSide::CROSS;
        }

        if (has_front) {
            return PlaneSide::FRONT;
        }

        if (has_back) {
            return PlaneSide::BACK;
        }

        return PlaneSide::ON;
    }

    Winding Winding::clipped(const Plane& plane, PlaneSide keep, float epsilon) const
    {
        if (keep != PlaneSide::FRONT && keep != PlaneSide::BACK) {
            throw std::invalid_argument("Winding: clipped() can only keep PlaneSide::FRONT or PlaneSide::BACK");
        }

        Winding front;
        Winding back;
        this->split(plane, front, back, epsilon);

        return keep == PlaneSide::FRONT ? front : back;
    }

    bool Winding::split(const Plane& plane, Winding& front, Winding& back, float epsilon) const
    {
        front.points.clear();
        back.points.clear();

        size_t count = this->points.size();
        if (count < 3) {
            return false;
        }

        // Classify every point once up front, with the first entry repeated at the end so
        // the edge walk below can look at point[i + 1] without wrapping by hand.
        std::vector<float> distances(count + 1);
        std::vector<PlaneSide> sides(count + 1);

        int front_count = 0;
        int back_count = 0;

        for (size_t index = 0; index < count; ++index) {
            distances[index] = plane.distance_to(this->points[index]);

            if (distances[index] > epsilon) {
                sides[index] = PlaneSide::FRONT;
                ++front_count;
            } else if (distances[index] < -epsilon) {
                sides[index] = PlaneSide::BACK;
                ++back_count;
            } else {
                sides[index] = PlaneSide::ON;
            }
        }

        distances[count] = distances[0];
        sides[count] = sides[0];

        // Wholly on one side (or coplanar): hand the whole winding back on that side rather
        // than rebuilding an identical copy point by point.
        if (front_count == 0 && back_count == 0) {
            front = *this;
            back = *this;
            return false;
        }

        if (front_count == 0) {
            back = *this;
            return false;
        }

        if (back_count == 0) {
            front = *this;
            return false;
        }

        front.points.reserve(count + 4);
        back.points.reserve(count + 4);

        for (size_t index = 0; index < count; ++index) {
            Vector3 point = this->points[index];

            // A point lying on the plane belongs to both halves: it is a corner of the new
            // edge each half gains along the cut.
            if (sides[index] == PlaneSide::ON) {
                front.points.push_back(point);
                back.points.push_back(point);
                continue;
            }

            if (sides[index] == PlaneSide::FRONT) {
                front.points.push_back(point);
            } else {
                back.points.push_back(point);
            }

            if (sides[index + 1] == PlaneSide::ON || sides[index + 1] == sides[index]) {
                continue;
            }

            Vector3 next = this->points[(index + 1) % count];
            Vector3 mid = crossing_point(plane, point, next, distances[index], distances[index + 1]);

            front.points.push_back(mid);
            back.points.push_back(mid);
        }

        return true;
    }

    void Winding::simplify(float epsilon)
    {
        // Repeated clipping leaves two kinds of junk behind: points duplicated where a cut
        // landed exactly on an existing corner, and points left sitting mid-edge after a
        // neighbour was removed. Both produce zero-area triangles downstream. Removing one
        // point can expose another, so this runs until a pass changes nothing.
        bool changed = true;

        while (changed && this->points.size() >= 3) {
            changed = false;

            for (size_t index = 0; index < this->points.size(); ++index) {
                size_t next_index = (index + 1) % this->points.size();

                if (Vector3Distance(this->points[index], this->points[next_index]) < epsilon) {
                    this->points.erase(this->points.begin() + static_cast<long>(next_index));
                    changed = true;
                    break;
                }
            }

            if (changed || this->points.size() < 3) {
                continue;
            }

            for (size_t index = 0; index < this->points.size(); ++index) {
                size_t previous_index = (index + this->points.size() - 1) % this->points.size();
                size_t next_index = (index + 1) % this->points.size();

                Vector3 edge = Vector3Subtract(this->points[next_index], this->points[previous_index]);
                float length = Vector3Length(edge);

                if (length < epsilon) {
                    continue;
                }

                // Perpendicular distance from the middle point to the line joining its
                // neighbours; below the tolerance the three are colinear and the middle one
                // contributes no shape.
                Vector3 offset = Vector3Subtract(this->points[index], this->points[previous_index]);
                float distance = Vector3Length(Vector3CrossProduct(offset, edge)) / length;

                if (distance < epsilon) {
                    this->points.erase(this->points.begin() + static_cast<long>(index));
                    changed = true;
                    break;
                }
            }
        }
    }
}
