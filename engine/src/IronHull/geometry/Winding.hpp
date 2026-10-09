#pragma once

#include <vector>

#include <raylib.h>

#include <IronHull/geometry/Plane.hpp>

namespace IronHull
{
    // A convex polygon in 3D: the single shape the whole brush and BSP pipeline is built out
    // of. A brush face, a BSP leaf wall and a portal between two leaves are all windings.
    //
    // Points are wound counter-clockwise seen from the front of the winding plane, which is
    // the front-facing order OpenGL (and so raylib) expects, letting a winding be
    // triangulated straight into a renderable fan without reversal.
    //
    // Every operation that produces a new winding can produce an invalid one - clipping a
    // polygon by a plane it does not reach leaves nothing behind. Callers are expected to
    // check is_valid() rather than assume a result has geometry.
    class Winding
    {
        public:
            std::vector<Vector3> points;

        public:
            // The largest quad that fits on the plane within the compiler world bounds. Brush
            // faces are built by starting here and clipping the quad back with every other
            // plane of the brush, which is how a set of half-spaces becomes a polygon.
            static Winding from_plane(const Plane& plane, float radius = MAX_WORLD_COORD);

        public:
            // At least three points, and enough area to have a meaningful normal. Brush
            // planes that contribute nothing to the final shape (a bevel sliced off by its
            // neighbours) collapse to an invalid winding, which is how they get dropped.
            bool is_valid() const;

            float area() const;
            Vector3 centroid() const;
            BoundingBox bounds() const;

            // The plane this winding lies in, derived from the points themselves rather than
            // from whatever plane it was cut from. Useful for verifying a winding has not
            // drifted off its source plane.
            Plane plane() const;

            Winding reversed() const;

        public:
            // FRONT/BACK when every point is on one side (points lying on the plane do not
            // count against either), ON when the winding is coplanar, CROSS when it has
            // points on both sides and so needs splitting.
            PlaneSide classify(const Plane& plane, float epsilon = ON_EPSILON) const;

            // The part of the winding on the requested side of the plane, or an invalid
            // winding when nothing of it is there. `keep` must be FRONT or BACK.
            Winding clipped(const Plane& plane, PlaneSide keep, float epsilon = ON_EPSILON) const;

            // Cuts the winding in two along the plane. Either output can come back invalid
            // when the winding lies wholly on one side. Returns true when both sides got
            // geometry, i.e. the plane really did pass through it.
            bool split(const Plane& plane, Winding& front, Winding& back, float epsilon = ON_EPSILON) const;

        public:
            // Drops points that repeat and points that sit on the straight line between
            // their neighbours. Repeated clipping leaves these behind, and they turn into
            // degenerate (zero-area) triangles at render time.
            void simplify(float epsilon = POINT_EPSILON);
    };
}
