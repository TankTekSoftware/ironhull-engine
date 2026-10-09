#include <IronHull/map/BspFile.hpp>

#include <cstring>
#include <stdexcept>

#include <IronHull/io/FileSystem.hpp>

namespace IronHull
{
    namespace
    {
        const char MAGIC[4] = { 'I', 'H', 'B', 'S' };

        // Lump order is part of the format: entries may be appended, but never reordered or
        // removed without bumping BspFile::VERSION.
        enum Lump
        {
            LUMP_TEXTURES = 0,
            LUMP_PLANES,
            LUMP_VERTICES,
            LUMP_INDICES,
            LUMP_FACES,
            LUMP_NODES,
            LUMP_LEAVES,
            LUMP_LEAF_FACES,
            LUMP_LEAF_BRUSHES,
            LUMP_BRUSHES,
            LUMP_BRUSH_SIDES,
            LUMP_MODELS,
            LUMP_ENTITIES,

            LUMP_COUNT,
        };

        // --- BYTE WRITER --- //

        class ByteWriter
        {
            private:
                std::vector<unsigned char> bytes;

            public:
                const std::vector<unsigned char>& data() const { return this->bytes; }
                size_t size() const { return this->bytes.size(); }

            public:
                void write_u32(unsigned int value)
                {
                    this->bytes.push_back(static_cast<unsigned char>(value & 0xFFu));
                    this->bytes.push_back(static_cast<unsigned char>((value >> 8) & 0xFFu));
                    this->bytes.push_back(static_cast<unsigned char>((value >> 16) & 0xFFu));
                    this->bytes.push_back(static_cast<unsigned char>((value >> 24) & 0xFFu));
                }

                void write_i32(int value)
                {
                    unsigned int bits = 0;
                    std::memcpy(&bits, &value, sizeof(bits));
                    this->write_u32(bits);
                }

                // Floats go out as their IEEE-754 bit pattern rather than as text, so a
                // vertex read back is bit-identical to the one written.
                void write_float(float value)
                {
                    unsigned int bits = 0;
                    std::memcpy(&bits, &value, sizeof(bits));
                    this->write_u32(bits);
                }

                void write_vector3(Vector3 value)
                {
                    this->write_float(value.x);
                    this->write_float(value.y);
                    this->write_float(value.z);
                }

                void write_vector2(Vector2 value)
                {
                    this->write_float(value.x);
                    this->write_float(value.y);
                }

                void write_bounds(const BoundingBox& value)
                {
                    this->write_vector3(value.min);
                    this->write_vector3(value.max);
                }

                void write_string(const std::string& value)
                {
                    this->write_u32(static_cast<unsigned int>(value.size()));
                    this->bytes.insert(this->bytes.end(), value.begin(), value.end());
                }

                void write_bytes(const std::vector<unsigned char>& value)
                {
                    this->bytes.insert(this->bytes.end(), value.begin(), value.end());
                }

                void pad_to_multiple_of_four()
                {
                    while (this->bytes.size() % 4 != 0) {
                        this->bytes.push_back(0);
                    }
                }
        };

        // --- BYTE READER --- //

        class ByteReader
        {
            private:
                const unsigned char* bytes;
                size_t length;
                size_t position = 0;

            public:
                ByteReader(const unsigned char* data, size_t size) : bytes(data), length(size) { }

            public:
                bool at_end() const { return this->position >= this->length; }

            public:
                unsigned int read_u32()
                {
                    this->require(4);

                    unsigned int value = static_cast<unsigned int>(this->bytes[this->position])
                        | (static_cast<unsigned int>(this->bytes[this->position + 1]) << 8)
                        | (static_cast<unsigned int>(this->bytes[this->position + 2]) << 16)
                        | (static_cast<unsigned int>(this->bytes[this->position + 3]) << 24);

                    this->position += 4;
                    return value;
                }

                int read_i32()
                {
                    unsigned int bits = this->read_u32();

                    int value = 0;
                    std::memcpy(&value, &bits, sizeof(value));
                    return value;
                }

                float read_float()
                {
                    unsigned int bits = this->read_u32();

                    float value = 0.0f;
                    std::memcpy(&value, &bits, sizeof(value));
                    return value;
                }

                Vector3 read_vector3()
                {
                    Vector3 value;
                    value.x = this->read_float();
                    value.y = this->read_float();
                    value.z = this->read_float();
                    return value;
                }

                Vector2 read_vector2()
                {
                    Vector2 value;
                    value.x = this->read_float();
                    value.y = this->read_float();
                    return value;
                }

                BoundingBox read_bounds()
                {
                    BoundingBox value;
                    value.min = this->read_vector3();
                    value.max = this->read_vector3();
                    return value;
                }

                std::string read_string()
                {
                    unsigned int size = this->read_u32();
                    this->require(size);

                    std::string value(reinterpret_cast<const char*>(this->bytes + this->position), size);
                    this->position += size;
                    return value;
                }

                // Guards every read against a truncated or hand-tampered file, so a bad
                // .ihbsp throws instead of walking off the end of the buffer.
                void require(size_t needed) const
                {
                    if (this->position + needed > this->length) {
                        throw std::runtime_error("BspFile: unexpected end of data - the file is truncated or corrupt");
                    }
                }
        };

        // Rejects an element count that could not possibly fit in the lump, before it is used
        // to reserve memory. Without this a corrupt count of four billion would be a
        // multi-gigabyte allocation before the first read failed.
        void require_plausible_count(unsigned int count, size_t bytes_per_element, size_t lump_length)
        {
            if (bytes_per_element > 0 && static_cast<size_t>(count) > lump_length / bytes_per_element) {
                throw std::runtime_error("BspFile: lump declares more elements than it has room for - the file is corrupt");
            }
        }
    }

    std::vector<unsigned char> BspFile::write(const CompiledMap& map)
    {
        std::vector<ByteWriter> lumps(LUMP_COUNT);

        {
            ByteWriter& lump = lumps[LUMP_TEXTURES];
            lump.write_u32(static_cast<unsigned int>(map.textures.size()));

            for (const BspTexture& texture : map.textures) {
                lump.write_string(texture.name);
                lump.write_u32(texture.surface_flags);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_PLANES];
            lump.write_u32(static_cast<unsigned int>(map.planes.size()));

            for (const Plane& plane : map.planes) {
                lump.write_vector3(plane.normal);
                lump.write_float(plane.dist);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_VERTICES];
            lump.write_u32(static_cast<unsigned int>(map.vertices.size()));

            for (const BspVertex& vertex : map.vertices) {
                lump.write_vector3(vertex.position);
                lump.write_vector2(vertex.uv);
                lump.write_vector3(vertex.normal);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_INDICES];
            lump.write_u32(static_cast<unsigned int>(map.indices.size()));

            for (unsigned int index : map.indices) {
                lump.write_u32(index);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_FACES];
            lump.write_u32(static_cast<unsigned int>(map.faces.size()));

            for (const BspFace& face : map.faces) {
                lump.write_i32(face.plane);
                lump.write_i32(face.texture);
                lump.write_i32(face.first_vertex);
                lump.write_i32(face.vertex_count);
                lump.write_i32(face.first_index);
                lump.write_i32(face.index_count);
                lump.write_u32(face.surface_flags);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_NODES];
            lump.write_u32(static_cast<unsigned int>(map.nodes.size()));

            for (const BspNode& node : map.nodes) {
                lump.write_i32(node.plane);
                lump.write_i32(node.children[0]);
                lump.write_i32(node.children[1]);
                lump.write_bounds(node.bounds);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_LEAVES];
            lump.write_u32(static_cast<unsigned int>(map.leaves.size()));

            for (const BspLeaf& leaf : map.leaves) {
                lump.write_u32(leaf.contents);
                lump.write_i32(leaf.first_leaf_face);
                lump.write_i32(leaf.leaf_face_count);
                lump.write_i32(leaf.first_leaf_brush);
                lump.write_i32(leaf.leaf_brush_count);
                lump.write_bounds(leaf.bounds);
                lump.write_i32(leaf.area);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_LEAF_FACES];
            lump.write_u32(static_cast<unsigned int>(map.leaf_faces.size()));

            for (int face : map.leaf_faces) {
                lump.write_i32(face);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_LEAF_BRUSHES];
            lump.write_u32(static_cast<unsigned int>(map.leaf_brushes.size()));

            for (int brush : map.leaf_brushes) {
                lump.write_i32(brush);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_BRUSHES];
            lump.write_u32(static_cast<unsigned int>(map.brushes.size()));

            for (const BspBrush& brush : map.brushes) {
                lump.write_i32(brush.first_side);
                lump.write_i32(brush.side_count);
                lump.write_u32(brush.contents);
                lump.write_bounds(brush.bounds);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_BRUSH_SIDES];
            lump.write_u32(static_cast<unsigned int>(map.brush_sides.size()));

            for (const BspBrushSide& side : map.brush_sides) {
                lump.write_i32(side.plane);
                lump.write_i32(side.texture);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_MODELS];
            lump.write_u32(static_cast<unsigned int>(map.models.size()));

            for (const BspModel& model : map.models) {
                lump.write_i32(model.root_node);
                lump.write_i32(model.first_face);
                lump.write_i32(model.face_count);
                lump.write_i32(model.first_brush);
                lump.write_i32(model.brush_count);
                lump.write_bounds(model.bounds);
            }
        }

        {
            ByteWriter& lump = lumps[LUMP_ENTITIES];
            lump.write_u32(static_cast<unsigned int>(map.entities.size()));

            for (const BspEntity& entity : map.entities) {
                lump.write_u32(static_cast<unsigned int>(entity.keyvalues.size()));

                for (const MapKeyValue& pair : entity.keyvalues) {
                    lump.write_string(pair.key);
                    lump.write_string(pair.value);
                }

                lump.write_u32(static_cast<unsigned int>(entity.connections.size()));

                for (const MapConnection& connection : entity.connections) {
                    lump.write_string(connection.output);
                    lump.write_string(connection.target);
                    lump.write_string(connection.input);
                    lump.write_string(connection.parameter);
                    lump.write_float(connection.delay);
                    lump.write_i32(connection.times_to_fire);
                }
            }
        }

        // The header is a fixed size, so lump offsets can be worked out before any lump
        // bytes are copied in.
        size_t header_size = sizeof(MAGIC) + 4 + 4 + (LUMP_COUNT * 8);

        ByteWriter out;
        out.write_bytes(std::vector<unsigned char>(MAGIC, MAGIC + sizeof(MAGIC)));
        out.write_u32(BspFile::VERSION);
        out.write_u32(static_cast<unsigned int>(LUMP_COUNT));

        size_t offset = header_size;
        for (int index = 0; index < LUMP_COUNT; ++index) {
            size_t length = lumps[static_cast<size_t>(index)].size();

            // Lumps are padded to a 4-byte boundary so every lump starts aligned, which is
            // what lets a reader cast straight into one if it ever wants to.
            size_t padded = (length + 3) & ~static_cast<size_t>(3);

            out.write_u32(static_cast<unsigned int>(offset));
            out.write_u32(static_cast<unsigned int>(length));

            offset += padded;
        }

        for (int index = 0; index < LUMP_COUNT; ++index) {
            out.write_bytes(lumps[static_cast<size_t>(index)].data());
            out.pad_to_multiple_of_four();
        }

        return out.data();
    }

    CompiledMap BspFile::read(const std::vector<unsigned char>& bytes)
    {
        if (bytes.size() < sizeof(MAGIC) + 8) {
            throw std::runtime_error("BspFile: data is too short to be a compiled map");
        }

        if (std::memcmp(bytes.data(), MAGIC, sizeof(MAGIC)) != 0) {
            throw std::runtime_error("BspFile: data is not an .ihbsp file (bad magic)");
        }

        ByteReader header(bytes.data() + sizeof(MAGIC), bytes.size() - sizeof(MAGIC));

        unsigned int version = header.read_u32();
        if (version != BspFile::VERSION) {
            throw std::runtime_error("BspFile: compiled map is version " + std::to_string(version)
                + " but this build reads version " + std::to_string(BspFile::VERSION) + " - recompile the map");
        }

        unsigned int lump_count = header.read_u32();

        struct LumpEntry
        {
            size_t offset = 0;
            size_t length = 0;
        };

        std::vector<LumpEntry> directory(lump_count);

        for (unsigned int index = 0; index < lump_count; ++index) {
            directory[index].offset = header.read_u32();
            directory[index].length = header.read_u32();

            if (directory[index].offset + directory[index].length > bytes.size()) {
                throw std::runtime_error("BspFile: lump " + std::to_string(index) + " runs past the end of the file");
            }
        }

        // A file written by an older build simply has fewer lumps; treating a missing lump as
        // empty is what makes appending a lump a backwards-compatible change.
        auto reader_for = [&](Lump lump) {
            if (static_cast<unsigned int>(lump) >= lump_count) {
                return ByteReader(bytes.data(), 0);
            }

            const LumpEntry& entry = directory[static_cast<size_t>(lump)];
            return ByteReader(bytes.data() + entry.offset, entry.length);
        };

        CompiledMap map;

        {
            ByteReader lump = reader_for(LUMP_TEXTURES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                map.textures.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspTexture texture;
                    texture.name = lump.read_string();
                    texture.surface_flags = lump.read_u32();
                    map.textures.push_back(texture);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_PLANES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 16, directory[LUMP_PLANES].length);
                map.planes.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    Plane plane;
                    plane.normal = lump.read_vector3();
                    plane.dist = lump.read_float();
                    map.planes.push_back(plane);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_VERTICES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 32, directory[LUMP_VERTICES].length);
                map.vertices.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspVertex vertex;
                    vertex.position = lump.read_vector3();
                    vertex.uv = lump.read_vector2();
                    vertex.normal = lump.read_vector3();
                    map.vertices.push_back(vertex);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_INDICES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 4, directory[LUMP_INDICES].length);
                map.indices.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    map.indices.push_back(lump.read_u32());
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_FACES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 28, directory[LUMP_FACES].length);
                map.faces.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspFace face;
                    face.plane = lump.read_i32();
                    face.texture = lump.read_i32();
                    face.first_vertex = lump.read_i32();
                    face.vertex_count = lump.read_i32();
                    face.first_index = lump.read_i32();
                    face.index_count = lump.read_i32();
                    face.surface_flags = lump.read_u32();
                    map.faces.push_back(face);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_NODES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 36, directory[LUMP_NODES].length);
                map.nodes.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspNode node;
                    node.plane = lump.read_i32();
                    node.children[0] = lump.read_i32();
                    node.children[1] = lump.read_i32();
                    node.bounds = lump.read_bounds();
                    map.nodes.push_back(node);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_LEAVES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 48, directory[LUMP_LEAVES].length);
                map.leaves.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspLeaf leaf;
                    leaf.contents = lump.read_u32();
                    leaf.first_leaf_face = lump.read_i32();
                    leaf.leaf_face_count = lump.read_i32();
                    leaf.first_leaf_brush = lump.read_i32();
                    leaf.leaf_brush_count = lump.read_i32();
                    leaf.bounds = lump.read_bounds();
                    leaf.area = lump.read_i32();
                    map.leaves.push_back(leaf);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_LEAF_FACES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 4, directory[LUMP_LEAF_FACES].length);
                map.leaf_faces.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    map.leaf_faces.push_back(lump.read_i32());
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_LEAF_BRUSHES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 4, directory[LUMP_LEAF_BRUSHES].length);
                map.leaf_brushes.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    map.leaf_brushes.push_back(lump.read_i32());
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_BRUSHES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 36, directory[LUMP_BRUSHES].length);
                map.brushes.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspBrush brush;
                    brush.first_side = lump.read_i32();
                    brush.side_count = lump.read_i32();
                    brush.contents = lump.read_u32();
                    brush.bounds = lump.read_bounds();
                    map.brushes.push_back(brush);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_BRUSH_SIDES);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 8, directory[LUMP_BRUSH_SIDES].length);
                map.brush_sides.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspBrushSide side;
                    side.plane = lump.read_i32();
                    side.texture = lump.read_i32();
                    map.brush_sides.push_back(side);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_MODELS);
            if (!lump.at_end()) {
                unsigned int count = lump.read_u32();
                require_plausible_count(count, 44, directory[LUMP_MODELS].length);
                map.models.reserve(count);

                for (unsigned int index = 0; index < count; ++index) {
                    BspModel model;
                    model.root_node = lump.read_i32();
                    model.first_face = lump.read_i32();
                    model.face_count = lump.read_i32();
                    model.first_brush = lump.read_i32();
                    model.brush_count = lump.read_i32();
                    model.bounds = lump.read_bounds();
                    map.models.push_back(model);
                }
            }
        }

        {
            ByteReader lump = reader_for(LUMP_ENTITIES);
            if (!lump.at_end()) {
                unsigned int entity_count = lump.read_u32();

                for (unsigned int index = 0; index < entity_count; ++index) {
                    BspEntity entity;

                    unsigned int keyvalue_count = lump.read_u32();
                    for (unsigned int pair_index = 0; pair_index < keyvalue_count; ++pair_index) {
                        MapKeyValue pair;
                        pair.key = lump.read_string();
                        pair.value = lump.read_string();
                        entity.keyvalues.push_back(pair);
                    }

                    unsigned int connection_count = lump.read_u32();
                    for (unsigned int connection_index = 0; connection_index < connection_count; ++connection_index) {
                        MapConnection connection;
                        connection.output = lump.read_string();
                        connection.target = lump.read_string();
                        connection.input = lump.read_string();
                        connection.parameter = lump.read_string();
                        connection.delay = lump.read_float();
                        connection.times_to_fire = lump.read_i32();
                        entity.connections.push_back(connection);
                    }

                    map.entities.push_back(std::move(entity));
                }
            }
        }

        return map;
    }

    CompiledMap BspFile::load(const std::string& uri)
    {
        return BspFile::read(FileSystem::read_bytes(uri));
    }

    void BspFile::save(const std::string& uri, const CompiledMap& map)
    {
        std::vector<unsigned char> bytes = BspFile::write(map);
        FileSystem::write_bytes(uri, bytes.data(), bytes.size());
    }
}
