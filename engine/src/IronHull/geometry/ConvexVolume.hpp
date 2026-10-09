#pragma once

#include <vector>

#include <raylib.h>

#include <IronHull/geometry/Plane.hpp>
#include <IronHull/geometry/Winding.hpp>

namespace IronHull
{
    // A convex region of space held as an intersection of half-spaces, where inside means
    // behind every one of the planes. That is the same convention a brush uses - a brush is
    // literally this plus a texture per side - so the same type describes both a brush and a
    // cell of the BSP tree.
    //
    // Carrying a cell around as planes rather than as a polyhedron is what makes the tree
    // builder simple: splitting a cell is appending one plane, and the actual boundary
    // polygons are only worked out when something needs them.
    class ConvexVolume
    {
        public:
            std::vector<Plane> planes;

        public:
            // The box as six outward-facing planes. This is where the tree starts: the root
            // cell is the whole world, and every cell below it is this box with cuts taken
            // out of it.
            static ConvexVolume from_bounds(const BoundingBox& box);

        public:
            // The same region cut by a plane. `keep` picks which side survives: FRONT adds
            // the plane flipped (so the kept side is in front of the original plane), BACK
            // adds it as given.
            ConvexVolume split(const Plane& plane, PlaneSide keep) const;

            // The boundary polygon of each plane, obtained by clipping that plane back with
            // all the others. A plane that contributes no surface - a cut that misses the
            // region, or one made redundant by later cuts - yields an invalid winding, and
            // the returned vector stays index-aligned with `planes` so the caller can tell
            // which plane produced which.
            std::vector<Winding> faces() const;

            // Every corner of the region, gathered from its boundary polygons. Empty for a
            // region that encloses nothing.
            std::vector<Vector3> corners() const;

        public:
            // True when the planes enclose no actual volume. Cells like this appear
            // constantly while building a tree, because a cut can easily slice off a corner
            // that was already cut away.
            bool is_empty() const;

            BoundingBox bounds() const;

            // A point guaranteed to be strictly inside, used to ask what fills a cell. It is
            // the average of the corners, which for a convex region is always interior.
            Vector3 interior_point() const;

            bool contains(Vector3 point, float epsilon = ON_EPSILON) const;

        public:
            // Drops planes that contribute no boundary polygon. Cuts accumulate as a cell
            // descends the tree and most end up redundant; shedding them keeps faces() from
            // getting quadratically slower the deeper the tree goes.
            ConvexVolume simplified() const;
    };
}
