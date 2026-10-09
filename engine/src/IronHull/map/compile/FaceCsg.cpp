#include <IronHull/map/compile/FaceCsg.hpp>

#include <cmath>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>

namespace IronHull
{
    namespace
    {
        enum class Coplanar
        {
            NO,

            // The two planes describe the same surface facing the same way: two brushes
            // butted together share this wall, and only one of them should draw it.
            SAME_FACING,

            // Same surface, opposite facings: the face is pressed flat against the other
            // brush's skin, so nothing can ever see it.
            OPPOSITE_FACING,
        };

        Coplanar classify_coplanar(const Plane& a, const Plane& b)
        {
            float alignment = Vector3DotProduct(a.normal, b.normal);

            if (alignment > 1.0f - 0.001f && std::fabs(a.dist - b.dist) < DIST_EPSILON) {
                return Coplanar::SAME_FACING;
            }

            if (alignment < -1.0f + 0.001f && std::fabs(a.dist + b.dist) < DIST_EPSILON) {
                return Coplanar::OPPOSITE_FACING;
            }

            return Coplanar::NO;
        }
    }

    void FaceCsg::subtract_brush(const Winding& winding, const Plane& winding_plane,
        const CompileBrush& brush, bool winding_wins_ties, std::vector<Winding>& out)
    {
        // Settle the shared-surface question before cutting anything, because the answer
        // can be "this brush removes nothing at all" - and that has to be decided while the
        // winding is still whole, not once pieces of it have already been peeled off.
        for (const CompileBrushSide& side : brush.sides) {
            if (side.is_bevel) {
                continue;
            }

            if (classify_coplanar(winding_plane, side.plane) == Coplanar::SAME_FACING && winding_wins_ties) {
                out.push_back(winding);
                return;
            }
        }

        // Walking the brush's planes one at a time, each step peels off the part of what is
        // left that lies outside that plane - which is therefore outside the brush - and
        // carries the inside part forward. Whatever survives to the end was inside every
        // plane, so it was inside the brush, and is dropped.
        Winding remaining = winding;

        for (const CompileBrushSide& side : brush.sides) {
            // Bevels only repeat the brush's own bounds, and they sit exactly on its
            // extremes where they would shave slivers off faces that should be left alone.
            if (side.is_bevel) {
                continue;
            }

            if (!remaining.is_valid()) {
                return;
            }

            // A coplanar plane cannot split the winding - every point lies on it - so for
            // the cases that reach here (the other brush owns the shared surface, or this
            // face is buried flat against its skin) the face counts as inside this plane
            // and it is simply stepped over.
            if (classify_coplanar(winding_plane, side.plane) != Coplanar::NO) {
                continue;
            }

            Winding front;
            Winding back;
            remaining.split(side.plane, front, back);

            front.simplify();
            if (front.is_valid()) {
                out.push_back(front);
            }

            back.simplify();
            remaining = back;
        }
    }

    std::vector<CompileFace> FaceCsg::run(const std::vector<CompileBrush>& brushes)
    {
        std::vector<CompileFace> faces;

        for (size_t index = 0; index < brushes.size(); ++index) {
            const CompileBrush& brush = brushes[index];

            for (const CompileBrushSide& side : brush.sides) {
                if (side.is_bevel || !side.visible || side.winding.points.empty()) {
                    continue;
                }

                if ((side.surface_flags & SURFACE_NODRAW) != 0u) {
                    continue;
                }

                std::vector<Winding> fragments = { side.winding };

                for (size_t other = 0; other < brushes.size(); ++other) {
                    if (other == index || fragments.empty()) {
                        continue;
                    }

                    const CompileBrush& cutter = brushes[other];

                    // Only solids bury anything, and only within one entity - the world
                    // must not carve a door, and a door must not carve the world.
                    if (!cutter.is_solid() || cutter.entity != brush.entity) {
                        continue;
                    }

                    if (!Bounds::overlaps(brush.bounds, cutter.bounds, ON_EPSILON)) {
                        continue;
                    }

                    // Of two brushes sharing a surface, the one authored first keeps it.
                    // Using brush order rather than anything geometric is what makes the
                    // choice stable: the same map compiles to the same faces every time.
                    bool wins_ties = brush.order < cutter.order;

                    std::vector<Winding> survivors;
                    for (const Winding& fragment : fragments) {
                        FaceCsg::subtract_brush(fragment, side.plane, cutter, wins_ties, survivors);
                    }

                    fragments = std::move(survivors);
                }

                for (Winding& fragment : fragments) {
                    fragment.simplify();

                    if (!fragment.is_valid()) {
                        continue;
                    }

                    CompileFace face;
                    face.plane = side.plane;
                    face.winding = fragment;
                    face.texture = side.texture;
                    face.surface_flags = side.surface_flags;
                    face.brush = static_cast<int>(index);
                    face.entity = brush.entity;

                    faces.push_back(std::move(face));
                }
            }
        }

        return faces;
    }
}
