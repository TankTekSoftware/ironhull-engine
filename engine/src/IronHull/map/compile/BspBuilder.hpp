#pragma once

#include <vector>

#include <IronHull/geometry/ConvexVolume.hpp>
#include <IronHull/geometry/Plane.hpp>
#include <IronHull/map/compile/BrushGeometry.hpp>

namespace IronHull
{
    // A BSP tree as the compiler builds it, still in authored map space and still carrying
    // the working data - cell volumes, brush lists - that the later stages need and the
    // finished file does not.
    struct BspTree
    {
        struct Node
        {
            int plane_index = -1;
            Plane plane;

            // Child encoding matches BspNode: non-negative is a node, negative is a leaf
            // stored as -(leaf + 1). Index 0 is the front side.
            int children[2] = { 0, 0 };

            BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        };

        struct Leaf
        {
            unsigned int contents = CONTENTS_EMPTY;
            BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

            // Brushes near enough to this cell to be worth testing a sweep against.
            std::vector<int> brushes;

            // Surfaces that face into this cell, filled in after the tree is built.
            std::vector<int> faces;

            // The cell itself. Kept because finding which leaves are neighbours - and so
            // which parts of the map are reachable from the player - means intersecting
            // these against each other.
            ConvexVolume volume;

            // Flood-filled region id, or -1 for solid and for open space the player can
            // never reach.
            int area = -1;
        };

        std::vector<Node> nodes;
        std::vector<Leaf> leaves;

        // Root in the same child encoding as Node::children, so a map whose tree collapsed
        // to a single empty cell is still addressable.
        int root = 0;

        BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    };

    class BspBuilder
    {
        public:
            struct Options
            {
                // A hard stop on recursion. Reaching it turns the cell into a leaf, which
                // leaves a map slightly wrong rather than hanging the compiler on
                // pathological brushwork.
                int max_depth = 96;
            };

            struct Stats
            {
                int nodes = 0;
                int leaves = 0;
                int solid_leaves = 0;
                int max_depth = 0;
            };

        public:
            // Partitions space over the chosen brushes. `brush_indices` selects which of
            // `brushes` take part, which is how the world and each brush entity each get
            // their own tree while sharing one brush array.
            //
            // Split planes are taken from the brushes' own sides, which is what makes the
            // tree agree exactly with the geometry: every cell ends up either wholly inside
            // a brush or wholly outside every brush, so a cell's contents can be settled by
            // testing a single point in it.
            //
            // Planes are interned into `planes` as they are chosen, so node plane indices
            // are already the ones the compiled file will use.
            static BspTree build(const std::vector<CompileBrush>& brushes,
                const std::vector<int>& brush_indices, PlaneSet& planes,
                const Options& options, Stats& stats);
    };
}
