#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include <IronHull/geometry/Plane.hpp>

namespace IronHull
{
    // The authored, uncompiled map document: `content://maps/*.map`.
    //
    // This is the brush-based text format editors read and write, and the input to the map
    // compiler. It is a superset of the Quake .map format, so a map exported from an
    // existing editor parses as-is, with one addition of our own: entities may carry a
    // `connections` block describing map I/O (see MapConnection).
    //
    //   // entity 0
    //   {
    //   "classname" "worldspawn"
    //   // brush 0
    //   {
    //   ( -64 -64 -16 ) ( -64 -63 -16 ) ( -64 -64 -15 ) brick [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
    //   ...
    //   }
    //   }
    //   // entity 1
    //   {
    //   "classname" "npc_zombie"
    //   "targetname" "zombie_1"
    //   "origin" "0 0 24"
    //   connections
    //   {
    //   "OnDied" "cell_door" "Open" "" 0 -1
    //   }
    //   }
    //
    // Map space is Z-up with one unit to the inch, as every brush editor and the entity
    // bounds in `content://entities/*.lua` assume. The engine renders Y-up, so the compiler
    // - not this parser - is what converts between the two. Everything in this file is in
    // authored map space.
    //
    // A worldspawn entity holds the map's structural brushes; any other entity with brushes
    // is a brush entity (a door, a trigger) whose geometry moves with it.

    // Thrown by MapFile::parse() with the offending line number, so an editor can jump
    // straight to a malformed brush instead of reporting "bad map".
    class MapParseError : public std::runtime_error
    {
        private:
            int line_number;

        public:
            MapParseError(int line, const std::string& message);

        public:
            int line() const;
    };

    // How a face's texture is projected onto it.
    //
    // Internally this is always the explicit-axis form the Valve 220 variant of the format
    // uses: a U and a V direction in world space, an offset in texels, and a scale. Faces
    // read from a plain Quake .map (which only stores offset/rotation/scale, with the axes
    // implied by the face normal) have their axes derived at parse time, so everything
    // downstream of the parser sees one representation.
    //
    // Texel coordinates for a point on the face are:
    //
    //   u = dot(point, u_axis) / u_scale + u_shift
    //   v = dot(point, v_axis) / v_scale + v_shift
    //
    // which the compiler stores as-is. Dividing by the texture's pixel size to get the 0..1
    // coordinates a GPU wants happens at load time, where the texture is actually available
    // - so a map does not have to be recompiled when a texture is resized.
    struct MapFaceTexture
    {
        // Texture name without directory or extension, resolved against
        // `content://assets/textures/` at load time: "brick" or "walls/brick".
        std::string name;

        Vector3 u_axis = { 1.0f, 0.0f, 0.0f };
        Vector3 v_axis = { 0.0f, 1.0f, 0.0f };

        float u_shift = 0.0f;
        float v_shift = 0.0f;

        // Retained so the value survives a load/save round trip and the editor can show the
        // number the mapper typed. The rotation is already baked into the axes above, so
        // nothing reading this struct needs to apply it.
        float rotation = 0.0f;

        float u_scale = 1.0f;
        float v_scale = 1.0f;
    };

    // One side of a brush. The three points define the plane; which part of that plane ends
    // up as a polygon depends on every other face of the brush.
    //
    // Storing a plane as three points rather than a normal and a distance is what keeps
    // brushwork exact: the points sit on the editor's integer grid, so a wall shared by two
    // brushes yields a bit-identical plane for both.
    struct MapFace
    {
        Vector3 points[3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        MapFaceTexture texture;

        // The outward-facing plane through the three points.
        Plane plane() const;
    };

    // A convex solid, defined as the intersection of the half-spaces behind all of its
    // faces. Concave shapes are built from several brushes.
    struct MapBrush
    {
        std::vector<MapFace> faces;
    };

    // One wire in the map's I/O graph: when `output` fires on the entity holding this
    // connection, `input` is called on every entity named `target`.
    //
    // This is what lets a mapper wire a trigger to a door without writing code, and it is
    // the runtime counterpart of the `outputs` and `inputs` tables an entity script
    // declares in `content://entities/*.lua`.
    struct MapConnection
    {
        std::string output;
        std::string target;
        std::string input;

        // Passed to the input instead of the value the firing entity supplied. Empty means
        // "use whatever the output sent".
        std::string parameter;

        // Seconds to wait before delivering the input.
        float delay = 0.0f;

        // How many times this connection may fire before it goes dead. -1 is unlimited.
        int times_to_fire = -1;
    };

    // An entity property as authored. Kept as an ordered list rather than a map so that
    // saving a map back out preserves the order the keys were written in, which keeps
    // version control diffs readable.
    struct MapKeyValue
    {
        std::string key;
        std::string value;
    };

    struct MapEntity
    {
        std::vector<MapKeyValue> keyvalues;
        std::vector<MapBrush> brushes;
        std::vector<MapConnection> connections;

        const std::string* find(const std::string& key) const;
        std::string get(const std::string& key, const std::string& fallback = "") const;
        void set(const std::string& key, const std::string& value);

        std::string classname() const;
        bool is_worldspawn() const;
    };

    class MapFile
    {
        public:
            std::vector<MapEntity> entities;

        public:
            // Throws MapParseError on malformed input. The text is the whole file; read it
            // through FileSystem (or off disk in a tool) and hand it over as a string.
            static MapFile parse(const std::string& text);

            // Serializes back to the format above, always using explicit texture axes.
            std::string write() const;

        public:
            // The first worldspawn entity, or nullptr for a map that has none - which is
            // itself an error worth reporting, since it means the map has no world geometry.
            const MapEntity* worldspawn() const;
    };
}
