#pragma once

#include <string>
#include <vector>

#include <raylib.h>

#include <IronHull/geometry/Plane.hpp>
#include <IronHull/geometry/Winding.hpp>
#include <IronHull/map/CompiledMap.hpp>
#include <IronHull/map/MapFile.hpp>

namespace IronHull
{
    // One side of a brush as the compiler works with it: the authored plane, plus the
    // polygon that plane actually contributes once the brush's other sides have cut it back.
    //
    // Everything here is still in authored map space (Z-up). The conversion to engine space
    // happens at the very end of compilation, in MapCompiler.
    struct CompileBrushSide
    {
        Plane plane;
        MapFaceTexture texture;

        // The part of the plane that lies on the brush's surface. Comes back invalid for a
        // plane that encloses the brush without touching it - a bevel the mapper left behind
        // after dragging a vertex - in which case the plane still bounds the solid but draws
        // nothing.
        Winding winding;

        unsigned int surface_flags = SURFACE_NONE;

        // A plane the compiler added itself to square the brush off for box sweeps. It has
        // no texture and never draws.
        bool is_bevel = false;

        // Cleared by face CSG when another brush covers this polygon, and by the flood fill
        // when it turns out to face the void. A side can be solid and still draw nothing.
        bool visible = true;
    };

    struct CompileBrush
    {
        std::vector<CompileBrushSide> sides;

        unsigned int contents = CONTENTS_SOLID;
        BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

        // Which map entity owns this brush - 0 is worldspawn, anything else is a brush
        // entity - and the brush's position in the whole map's brush order, which is what
        // breaks ties between two brushes sharing a face so that the result is the same on
        // every compile.
        int entity = 0;
        int order = 0;

        bool contains(Vector3 point, float epsilon = ON_EPSILON) const;
        bool is_solid() const;
    };

    class BrushGeometry
    {
        public:
            // Brush contents and surface behaviour come from the texture name, which is how
            // a mapper says "this brush is water" or "this face does not draw" without any
            // per-face UI. The name is matched case-insensitively against its last path
            // component, so `tools/clip` and `CLIP` behave the same:
            //
            //   clip              solid, never drawn - smooths off detail a player would
            //                     otherwise catch on
            //   skip, null,
            //   nodraw            solid, this face not drawn
            //   sky*              solid, drawn as sky; faces looking at it are dropped
            //   trigger           not solid, reports overlap only
            //   water*, slime*,
            //   lava*             liquid, not solid, drawn with blending
            //   anything else     ordinary solid world geometry
            static unsigned int contents_from_texture(const std::string& name);
            static unsigned int surface_flags_from_texture(const std::string& name);

        public:
            // Turns the authored planes of a brush into polygons by starting each side as an
            // unbounded plane and cutting it back with all the others - the step that makes
            // a set of half-spaces into something that can be drawn.
            //
            // Returns false when the brush encloses no volume, which is what a mapper gets
            // for dragging a face through the other side of a solid. `out` is left
            // half-filled in that case and should be discarded.
            static bool build(const MapBrush& brush, int entity, int order, CompileBrush& out);

        public:
            // Texel coordinates for a point on a face, in authored map space - the texture
            // axes are authored there, so this has to run before the conversion to engine
            // space, not after.
            static Vector2 texture_uv(const MapFaceTexture& texture, Vector3 point);

        private:
            // Adds the axis-aligned planes a brush is missing, so that sweeping a box
            // against it is correct.
            //
            // The box sweep works by pushing each of a brush's planes outwards by the box's
            // extent, which is only right where the brush has a plane facing the direction
            // the box corner leads with. On a slope there is none, and the box clips the
            // sloped edge early. Adding the brush's own bounding planes fills those in.
            static void add_bevels(CompileBrush& brush);
    };
}
