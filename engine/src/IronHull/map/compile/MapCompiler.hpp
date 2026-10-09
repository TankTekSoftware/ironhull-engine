#pragma once

#include <string>
#include <vector>

#include <raylib.h>

#include <IronHull/map/CompiledMap.hpp>
#include <IronHull/map/MapFile.hpp>

namespace IronHull
{
    // Turns an authored brush map into the compiled form the runtime loads.
    //
    // The stages, in order:
    //
    //   1. Each brush's authored planes become polygons (BrushGeometry), and the texture on
    //      each side decides what the brush is made of.
    //   2. Surfaces buried inside other brushes are carved away (FaceCsg), so a wall sunk
    //      into a floor does not ship the part nobody can see.
    //   3. Space is partitioned into convex cells over the brushes' own planes
    //      (BspBuilder), giving a tree that answers "what is at this point" and narrows down
    //      what a moving body could possibly hit.
    //   4. Open cells are flooded from the entities (PortalFlood) to find what the player
    //      can reach, which both culls the map's outward-facing skin and catches a map with
    //      a hole in its walls.
    //   5. Everything is converted from authored Z-up map space into engine Y-up space and
    //      written into flat arrays.
    //
    // The compiler takes text and gives back data, touching no files of its own: the `ihbsp`
    // tool reads and writes through the filesystem, and the editor will hand over the
    // document it has in memory.
    class MapCompiler
    {
        public:
            struct Options
            {
                // Scales every distance on the way into engine space. Map units are inches
                // by convention; 1/32 gives roughly metres, which is the scale Box3D's
                // default gravity assumes.
                float scale = 1.0f;

                // Carve away surfaces buried inside other brushes.
                bool face_csg = true;

                // Throw away the map's outward-facing skin. Ignored for a map that leaks,
                // where inside and outside cannot be told apart.
                bool cull_outside = true;

                int max_depth = 96;
            };

            struct Report
            {
                int brushes = 0;
                int invalid_brushes = 0;

                int faces = 0;
                int culled_faces = 0;
                int vertices = 0;
                int triangles = 0;

                int nodes = 0;
                int leaves = 0;
                int solid_leaves = 0;
                int max_depth = 0;

                int models = 0;
                int entities = 0;
                int textures = 0;

                int areas = 0;
                bool leaked = false;
                Vector3 leak_point = { 0.0f, 0.0f, 0.0f };
                std::string leak_entity;

                // Anything the mapper should hear about but which did not stop the compile.
                std::vector<std::string> warnings;

                double seconds = 0.0;
            };

        public:
            static CompiledMap compile(const MapFile& map, const Options& options, Report& report);
    };
}
