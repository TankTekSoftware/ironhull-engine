#pragma once

#include <string>
#include <vector>

#include <raylib.h>

#include <IronHull/geometry/Plane.hpp>
#include <IronHull/map/MapFile.hpp>

namespace IronHull
{
    // The output of the map compiler: a binary-ready description of a compiled map, held in
    // flat index-referenced arrays so it serializes without pointer fixups and loads without
    // allocation per element.
    //
    // Unlike MapFile, which is authored map space (Z-up, as brush editors work), everything
    // here is in engine space (Y-up, as raylib and Box3D work). The compiler performs that
    // conversion once while emitting; nothing downstream has to think about it.

    // What a volume is made of. Flags rather than an enum because a brush can be more than
    // one thing at once, and because a BSP leaf inherits the union of what fills it.
    enum ContentFlags : unsigned int
    {
        CONTENTS_EMPTY = 0u,

        // Blocks movement.
        CONTENTS_SOLID = 1u << 0,

        // Blocks movement and is never drawn - a clip brush, used to smooth off detail the
        // player should slide along rather than catch on.
        CONTENTS_CLIP = 1u << 1,

        // Swimmable volumes. Not solid, but something a mover needs to know it is inside.
        CONTENTS_WATER = 1u << 2,
        CONTENTS_SLIME = 1u << 3,
        CONTENTS_LAVA = 1u << 4,

        // The void outside the map. Drawn as the sky and treated as solid, but faces looking
        // at it are dropped rather than textured.
        CONTENTS_SKY = 1u << 5,

        // A volume that only reports overlap, used by brush entities such as triggers.
        CONTENTS_TRIGGER = 1u << 6,

        // Everything a body cannot pass through.
        CONTENTS_SOLID_MASK = CONTENTS_SOLID | CONTENTS_CLIP | CONTENTS_SKY,

        // Everything that counts as a liquid.
        CONTENTS_LIQUID_MASK = CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA,
    };

    // How a face is drawn, as opposed to what its brush is made of.
    enum SurfaceFlags : unsigned int
    {
        SURFACE_NONE = 0u,

        // Not submitted for drawing at all: the invisible side of a clip brush, a trigger,
        // or a face the compiler found to be facing solid or the void.
        SURFACE_NODRAW = 1u << 0,

        SURFACE_SKY = 1u << 1,

        // Drawn with blending, so it has to be submitted after the opaque world.
        SURFACE_TRANSPARENT = 1u << 2,
    };

    // A texture the map refers to. The name has no directory or extension; the runtime
    // resolves it against `content://assets/textures/` and tries each supported extension,
    // so the same compiled map works whether an artist saved a .png or a .jpg.
    struct BspTexture
    {
        std::string name;
        unsigned int surface_flags = SURFACE_NONE;
    };

    // UVs are in texels, not normalized. The compiler never needs to open an image, and a
    // texture can be re-exported at a different resolution without recompiling the map; the
    // division by texture size happens when the mesh is built and the size is known.
    struct BspVertex
    {
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        Vector2 uv = { 0.0f, 0.0f };
        Vector3 normal = { 0.0f, 0.0f, 0.0f };
    };

    // A convex polygon of the map's surface, already triangulated into the shared index
    // buffer so loading is a straight copy to the GPU.
    struct BspFace
    {
        int plane = 0;
        int texture = 0;

        int first_vertex = 0;
        int vertex_count = 0;

        // Indices are absolute into CompiledMap::vertices, not relative to first_vertex.
        int first_index = 0;
        int index_count = 0;

        unsigned int surface_flags = SURFACE_NONE;
    };

    // An interior node of the BSP tree: a plane, and what lies on each side of it.
    //
    // A child is a node index when non-negative and a leaf index encoded as -(leaf + 1) when
    // negative, which is how a single int can address either without a second field. Note
    // that this makes leaf 0 encode as -1, so there is no ambiguity with a zero node index.
    struct BspNode
    {
        int plane = 0;
        int children[2] = { 0, 0 };
        BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    };

    // A convex cell of space. Empty leaves hold the faces that look into them and the
    // brushes that touch them; solid leaves hold nothing and exist only to stop a traversal.
    struct BspLeaf
    {
        unsigned int contents = CONTENTS_EMPTY;

        int first_leaf_face = 0;
        int leaf_face_count = 0;

        int first_leaf_brush = 0;
        int leaf_brush_count = 0;

        BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

        // Which flood-filled region of open space this leaf belongs to, or -1 for a solid
        // leaf and for open space sealed off from where the players spawn. Faces that only
        // face a region of -1 are outside the playable map and get dropped.
        int area = -1;
    };

    // Brushes survive compilation alongside the tree because they are what box sweeps test
    // against: a convex set of planes is far easier to sweep a box through than a polygon
    // soup, and the tree is only used to narrow down which brushes to bother with.
    struct BspBrushSide
    {
        int plane = 0;
        int texture = -1;
    };

    struct BspBrush
    {
        int first_side = 0;
        int side_count = 0;
        unsigned int contents = CONTENTS_EMPTY;
        BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    };

    // A separately-rooted piece of the map. Model 0 is the world; every brush entity (a
    // door, a platform, a trigger volume) compiles to its own model so that it can be moved
    // and collided with independently, and its entity refers to it by a "*1"-style model key.
    struct BspModel
    {
        int root_node = 0;

        int first_face = 0;
        int face_count = 0;

        int first_brush = 0;
        int brush_count = 0;

        BoundingBox bounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    };

    // An entity to spawn, carried through from the .map unchanged apart from the origin and
    // angles having been converted into engine space. Reuses the authored keyvalue and
    // connection types: there is nothing to gain from a second representation of the same
    // thing, and the entity system reads both the same way.
    struct BspEntity
    {
        std::vector<MapKeyValue> keyvalues;
        std::vector<MapConnection> connections;

        std::string get(const std::string& key, const std::string& fallback = "") const;
        std::string classname() const;
    };

    // The result of sweeping a point or a box through the map.
    struct TraceResult
    {
        // Fraction of the way from start to end that the sweep got. 1 means it completed.
        float fraction = 1.0f;

        Vector3 position = { 0.0f, 0.0f, 0.0f };

        // Surface normal at the point of contact, pointing back towards the sweep.
        Vector3 normal = { 0.0f, 0.0f, 0.0f };

        // What was struck, and what was struck first if the sweep began already embedded.
        unsigned int contents = CONTENTS_EMPTY;

        bool hit = false;

        // The sweep began inside something solid. Game code generally has to push out
        // rather than treat this as a normal collision, so it is reported separately.
        bool started_solid = false;

        // The sweep was inside something solid the whole way.
        bool all_solid = false;
    };

    class CompiledMap
    {
        public:
            std::vector<BspTexture> textures;
            std::vector<Plane> planes;
            std::vector<BspVertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<BspFace> faces;
            std::vector<BspNode> nodes;
            std::vector<BspLeaf> leaves;
            std::vector<int> leaf_faces;
            std::vector<int> leaf_brushes;
            std::vector<BspBrush> brushes;
            std::vector<BspBrushSide> brush_sides;
            std::vector<BspModel> models;
            std::vector<BspEntity> entities;

        public:
            // Descends the tree to the leaf containing the point. Returns -1 when the map
            // has no tree at all.
            int find_leaf(Vector3 point, int model = 0) const;

            // What fills the point: CONTENTS_EMPTY out in the open, CONTENTS_SOLID inside a
            // wall, a liquid flag when submerged.
            unsigned int point_contents(Vector3 point, int model = 0) const;

            // Sweeps a point along a line, stopping at the first surface matching `mask`.
            // Walks the tree directly, so it is exact and cheap - the right call for a line
            // of sight check or a hitscan weapon.
            TraceResult trace_ray(Vector3 start, Vector3 end, unsigned int mask = CONTENTS_SOLID_MASK, int model = 0) const;

            // Sweeps an axis-aligned box, expressed as offsets from the swept point, by
            // testing it against the brushes near the line. This is what a moving body
            // should use; `mins` is expected to be negative on each axis and `maxs` positive.
            TraceResult trace_box(Vector3 start, Vector3 end, Vector3 mins, Vector3 maxs,
                unsigned int mask = CONTENTS_SOLID_MASK, int model = 0) const;

        public:
            // Every entity whose classname matches, in map order.
            std::vector<const BspEntity*> find_entities(const std::string& classname) const;

            const BspEntity* worldspawn() const;

        public:
            // Conversions for the negative-index child encoding BspNode uses. Public because
            // any code walking the tree by hand needs them.
            static int leaf_to_child(int leaf_index);
            static int child_to_leaf(int child);
            static bool child_is_leaf(int child);

        private:
            void collect_leaf_brushes(int child, const BoundingBox& box, unsigned int mask,
                std::vector<int>& out_brushes) const;

            void clip_box_to_brush(const BspBrush& brush, Vector3 start, Vector3 end,
                Vector3 mins, Vector3 maxs, TraceResult& result) const;
    };
}
