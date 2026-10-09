#pragma once

#include <vector>

#include <raylib.h>

namespace IronHull
{
    // Tolerances shared by every geometric predicate in the map pipeline. Brush geometry is
    // authored on an integer grid in map units (Quake-style, 1 unit ~ 1 inch), so these are
    // absolute distances in map units rather than relative epsilons.

    // How far off a plane a point may sit and still count as lying on it. Deliberately
    // generous: it has to absorb the drift of a vertex produced by intersecting three
    // near-parallel brush planes, and misclassifying such a vertex as off-plane is exactly
    // what tears holes in a compiled BSP.
    constexpr float ON_EPSILON = 0.1f;

    // Two normals closer than this are treated as the same direction, and two plane
    // distances closer than this as the same offset. Used both to snap authored planes onto
    // clean axes and to collapse duplicate planes into a single entry.
    constexpr float NORMAL_EPSILON = 0.00001f;
    constexpr float DIST_EPSILON = 0.01f;

    // Vertices closer together than this are welded into one. Must stay below ON_EPSILON so
    // welding can never move a vertex far enough to change which side of a plane it is on.
    constexpr float POINT_EPSILON = 0.01f;

    // Half-extent of the world the compiler is willing to work in. Face windings start life
    // as a quad this big before being clipped down by the brush they belong to, so it has to
    // comfortably exceed any real map while staying well away from float precision trouble.
    constexpr float MAX_WORLD_COORD = 65536.0f;

    // Which side of a plane something sits on. FRONT is the half-space the normal points
    // into. Brush face normals point out of the brush, so a brush interior is BACK of all of
    // its own faces. CROSS is only ever returned for shapes (windings, volumes) that have
    // geometry on both sides; a single point is always FRONT, BACK or ON.
    enum class PlaneSide
    {
        FRONT,
        BACK,
        ON,
        CROSS,
    };

    // Whether a plane is perpendicular to a world axis. Axial planes are cheaper to test
    // against and make for better BSP splits, so the tree builder prefers them when choosing
    // where to divide space.
    enum class PlaneType
    {
        AXIAL_X,
        AXIAL_Y,
        AXIAL_Z,
        NON_AXIAL,
    };

    // An oriented plane in the form dot(normal, point) = dist.
    struct Plane
    {
        Vector3 normal = { 0.0f, 0.0f, 0.0f };
        float dist = 0.0f;

        // Builds the plane through three points using the winding convention Quake .map
        // files are written in: normal = cross(a - b, c - b), which comes out pointing out of
        // the brush for the point order editors emit. Changing this flips every brush in
        // every map inside out, so it has to match the format exactly.
        static Plane from_points(Vector3 a, Vector3 b, Vector3 c);
        static Plane from_normal_point(Vector3 normal, Vector3 point);

        // Signed distance from the plane: positive in front, negative behind.
        float distance_to(Vector3 point) const;

        PlaneSide classify_point(Vector3 point, float epsilon = ON_EPSILON) const;
        PlaneType type() const;

        // Same surface, opposite facing.
        Plane flipped() const;

        // False for a plane whose normal failed to normalize (three colinear or coincident
        // points), which is how a malformed brush face announces itself.
        bool is_valid() const;

        // True when both planes describe the same surface with the same facing. A flipped
        // copy of a plane is NOT equal to it.
        bool equals(const Plane& other) const;

        // Pulls a near-axial normal onto its exact axis and a near-integer distance onto that
        // integer. Authored brush planes are derived from integer points but arrive with
        // normals like (0, 0.9999998, 0); left alone, that tiny tilt accumulates through every
        // clip and split until coplanar faces stop agreeing with each other.
        void snap();
    };

    // The compiled plane table. Planes are shared aggressively - every brush side, BSP node
    // and face refers to one by index - so a surface authored by a dozen brushes costs a
    // single entry.
    class PlaneSet
    {
        private:
            std::vector<Plane> planes;

        public:
            // Returns the index of an equal existing plane, or appends and returns the new
            // one. The plane is snapped first, so planes that differ only by authoring noise
            // collapse together.
            int add(const Plane& plane);

            const Plane& get(int index) const;
            const std::vector<Plane>& all() const;
            int count() const;
    };
}
