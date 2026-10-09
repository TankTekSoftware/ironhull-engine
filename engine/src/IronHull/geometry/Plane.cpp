#include <IronHull/geometry/Plane.hpp>

#include <cmath>
#include <stdexcept>

#include <raymath.h>

namespace IronHull
{
    Plane Plane::from_points(Vector3 a, Vector3 b, Vector3 c)
    {
        Vector3 ab = Vector3Subtract(a, b);
        Vector3 cb = Vector3Subtract(c, b);

        Plane plane;
        plane.normal = Vector3Normalize(Vector3CrossProduct(ab, cb));
        plane.dist = Vector3DotProduct(b, plane.normal);
        plane.snap();

        return plane;
    }

    Plane Plane::from_normal_point(Vector3 normal, Vector3 point)
    {
        Plane plane;
        plane.normal = Vector3Normalize(normal);
        plane.dist = Vector3DotProduct(point, plane.normal);
        plane.snap();

        return plane;
    }

    float Plane::distance_to(Vector3 point) const
    {
        return Vector3DotProduct(this->normal, point) - this->dist;
    }

    PlaneSide Plane::classify_point(Vector3 point, float epsilon) const
    {
        float distance = this->distance_to(point);

        if (distance > epsilon) {
            return PlaneSide::FRONT;
        }

        if (distance < -epsilon) {
            return PlaneSide::BACK;
        }

        return PlaneSide::ON;
    }

    PlaneType Plane::type() const
    {
        // snap() has already pulled anything near an axis exactly onto it, so an exact
        // comparison is the right test here rather than an epsilon one.
        if (std::fabs(this->normal.x) == 1.0f) {
            return PlaneType::AXIAL_X;
        }

        if (std::fabs(this->normal.y) == 1.0f) {
            return PlaneType::AXIAL_Y;
        }

        if (std::fabs(this->normal.z) == 1.0f) {
            return PlaneType::AXIAL_Z;
        }

        return PlaneType::NON_AXIAL;
    }

    Plane Plane::flipped() const
    {
        Plane plane;
        plane.normal = Vector3Negate(this->normal);
        plane.dist = -this->dist;

        return plane;
    }

    bool Plane::is_valid() const
    {
        // Vector3Normalize hands back a zero vector rather than failing when given one, so a
        // degenerate face plane shows up here as a normal that is not unit length.
        float length_squared = Vector3LengthSqr(this->normal);
        return std::fabs(length_squared - 1.0f) < 0.01f;
    }

    bool Plane::equals(const Plane& other) const
    {
        return std::fabs(this->normal.x - other.normal.x) < NORMAL_EPSILON
            && std::fabs(this->normal.y - other.normal.y) < NORMAL_EPSILON
            && std::fabs(this->normal.z - other.normal.z) < NORMAL_EPSILON
            && std::fabs(this->dist - other.dist) < DIST_EPSILON;
    }

    void Plane::snap()
    {
        float* components[3] = { &this->normal.x, &this->normal.y, &this->normal.z };

        for (int axis = 0; axis < 3; ++axis) {
            float value = *components[axis];

            // Snapping an axis to +-1 forces the other two to zero: a unit normal with one
            // component at full magnitude has nothing left over for the others.
            if (std::fabs(value - 1.0f) < NORMAL_EPSILON) {
                this->normal = { 0.0f, 0.0f, 0.0f };
                *components[axis] = 1.0f;
                break;
            }

            if (std::fabs(value + 1.0f) < NORMAL_EPSILON) {
                this->normal = { 0.0f, 0.0f, 0.0f };
                *components[axis] = -1.0f;
                break;
            }
        }

        // Normalize away negative zero. It compares equal to positive zero so it never
        // breaks a predicate, but it does serialize to different bytes, which would leave
        // two identical planes looking different in a compiled file.
        for (int axis = 0; axis < 3; ++axis) {
            if (*components[axis] == 0.0f) {
                *components[axis] = 0.0f;
            }
        }

        float rounded = std::round(this->dist);
        if (std::fabs(this->dist - rounded) < DIST_EPSILON) {
            this->dist = rounded;
        }
    }

    int PlaneSet::add(const Plane& plane)
    {
        Plane snapped = plane;
        snapped.snap();

        // A linear scan is fine here: plane counts run to a few thousand for a large map and
        // this only runs during compilation, never at load or draw time.
        for (size_t index = 0; index < this->planes.size(); ++index) {
            if (this->planes[index].equals(snapped)) {
                return static_cast<int>(index);
            }
        }

        this->planes.push_back(snapped);
        return static_cast<int>(this->planes.size() - 1);
    }

    const Plane& PlaneSet::get(int index) const
    {
        if (index < 0 || index >= static_cast<int>(this->planes.size())) {
            throw std::out_of_range("PlaneSet: plane index out of range");
        }

        return this->planes[static_cast<size_t>(index)];
    }

    const std::vector<Plane>& PlaneSet::all() const
    {
        return this->planes;
    }

    int PlaneSet::count() const
    {
        return static_cast<int>(this->planes.size());
    }
}
