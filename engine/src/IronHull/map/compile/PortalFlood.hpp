#pragma once

#include <string>
#include <vector>

#include <raylib.h>

#include <IronHull/map/compile/BspBuilder.hpp>
#include <IronHull/map/compile/FaceCsg.hpp>

namespace IronHull
{
    // Works out which of the tree's open cells a player can actually get to, and uses the
    // answer to throw away everything they cannot.
    //
    // A brush-built map is a solid block of world with rooms hollowed out of it, and the
    // compiler has no other way to tell the inside of a room from the infinite open space
    // surrounding the map. Flooding outwards from where the players spawn settles it: cells
    // the flood reaches are rooms, cells it does not are either solid or the void, and the
    // outward-facing skin of the map - every face that only ever looks at the void - can go.
    //
    // It also catches the single most common mapping mistake. If the flood escapes the map
    // entirely and reaches the edge of the world, there is a hole in the walls somewhere:
    // the map "leaks". Nothing can be culled from a leaking map, because the compiler can no
    // longer tell inside from outside, so it says where to look instead.
    class PortalFlood
    {
        public:
            struct Result
            {
                // How many separate regions of reachable space were found. More than one
                // means parts of the map are sealed off from each other, which is legal but
                // worth knowing about.
                int area_count = 0;

                bool leaked = false;

                // Where the escaping flood started, so the mapper has somewhere to look.
                Vector3 leak_point = { 0.0f, 0.0f, 0.0f };
                std::string leak_entity;

                int reachable_leaves = 0;
                int sealed_leaves = 0;
            };

        public:
            // Floods out from each seed - normally the origin of every player start and
            // other point entity - marking reachable cells with an area id.
            static Result run(BspTree& tree, const std::vector<Vector3>& seeds,
                const std::vector<std::string>& seed_names);

            // Hands every face to the cell it faces into, and when `cull_outside` is set,
            // hides the ones whose cell the flood never reached.
            //
            // Culling has to be skipped for a leaking map: with inside and outside
            // indistinguishable, it would strip the map back to nothing.
            static void assign_faces(BspTree& tree, std::vector<CompileFace>& faces, bool cull_outside);

        private:
            static int find_leaf(const BspTree& tree, Vector3 point);

            // Builds the neighbour lists the flood travels along. Two open cells are
            // neighbours when their boundaries share an area of surface - the portal
            // between them.
            static std::vector<std::vector<int>> build_adjacency(const BspTree& tree);
    };
}
