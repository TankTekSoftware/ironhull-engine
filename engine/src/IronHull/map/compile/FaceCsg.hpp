#pragma once

#include <vector>

#include <IronHull/map/compile/BrushGeometry.hpp>

namespace IronHull
{
    // A finished surface polygon. One brush side can produce several of these, because
    // carving away the buried part of a side generally leaves more than one piece behind.
    struct CompileFace
    {
        Plane plane;
        Winding winding;
        MapFaceTexture texture;

        unsigned int surface_flags = SURFACE_NONE;

        int brush = -1;
        int entity = 0;

        // Cleared by the flood fill for a face that only ever faces sealed-off space.
        bool visible = true;
    };

    class FaceCsg
    {
        public:
            // Cuts away the parts of every brush side that are buried inside another solid
            // brush of the same entity, and resolves two brushes that share a surface down
            // to one face.
            //
            // Without this, a map built the way mappers actually build them - overlapping
            // blocks, a pillar sunk into a floor - would render every buried surface, and
            // every shared surface twice, which shows up as z-fighting on walls that look
            // like they should be flat.
            //
            // Only brushes belonging to the same entity carve each other: a door is a
            // separate entity that moves, so the world must not be allowed to cut holes in
            // the geometry it is standing in.
            static std::vector<CompileFace> run(const std::vector<CompileBrush>& brushes);

        private:
            // The pieces of `winding` left over after removing the part inside `brush`.
            // Appends to `out` and leaves it untouched when nothing of the winding is
            // inside. `winding_wins_ties` decides which of two brushes sharing a surface
            // keeps the face.
            static void subtract_brush(const Winding& winding, const Plane& winding_plane,
                const CompileBrush& brush, bool winding_wins_ties, std::vector<Winding>& out);
    };
}
