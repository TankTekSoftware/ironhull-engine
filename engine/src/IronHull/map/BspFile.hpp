#pragma once

#include <string>
#include <vector>

#include <IronHull/map/CompiledMap.hpp>

namespace IronHull
{
    // Reads and writes `.ihbsp`, the compiled counterpart of a `.map`: what the map compiler
    // emits and what the runtime actually loads.
    //
    // The layout is a short header followed by a directory of lumps, each lump being one of
    // CompiledMap's arrays:
    //
    //   char     magic[4]      "IHBS"
    //   uint32   version
    //   uint32   lump_count
    //   { uint32 offset, uint32 length } * lump_count
    //
    // Indexing the lumps by position rather than naming them keeps the header fixed-size,
    // and storing a count per lump means a reader can skip a lump it does not understand.
    // All scalars are written explicitly little-endian so a map compiled on one machine
    // loads on any other.
    class BspFile
    {
        public:
            // Bumped whenever a lump's layout changes. A file written by a different version
            // is rejected rather than misread: the compiler is a build step, so the fix is
            // always just to recompile the map.
            static constexpr unsigned int VERSION = 1u;

        public:
            static std::vector<unsigned char> write(const CompiledMap& map);

            // Throws std::runtime_error for a file that is truncated, not an .ihbsp at all,
            // or written by a different version of the format.
            static CompiledMap read(const std::vector<unsigned char>& bytes);

        public:
            // Loads through FileSystem, so the uri is a `content://` or `user://` path.
            static CompiledMap load(const std::string& uri);

            // Saves through FileSystem, which only permits writes to `user://`. The compiler
            // tool writes its output straight to disk instead, since a build step has no
            // business writing into the packaged content directory.
            static void save(const std::string& uri, const CompiledMap& map);
    };
}
