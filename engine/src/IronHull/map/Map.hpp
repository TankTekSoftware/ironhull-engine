#pragma once

#include <string>
#include <vector>

#include <raylib.h>

#include <IronHull/entity/EntityWorld.hpp>
#include <IronHull/map/CompiledMap.hpp>

namespace IronHull
{
    // A loaded, playable map: the compiled data, the meshes built from it, the textures it
    // asked for, and the entities living in it.
    //
    // This is the one type game code needs. Load a `.ihbsp`, tick it, draw it, and ask it
    // what is where:
    //
    //   IronHull::EntityRegistry::load_directory();
    //   map.load("content://maps/testmap.ihbsp");
    //   ...
    //   map.update(delta);
    //   BeginMode3D(camera);
    //   map.draw();
    //   EndMode3D();
    //
    // The entity registry has to be loaded first: a map can only spawn classnames that have
    // a script behind them.
    class Map
    {
        private:
            // One draw call's worth of geometry: every triangle in a model that uses the
            // same texture, in one mesh.
            //
            // Batching by texture rather than walking the BSP tree per frame is the right
            // trade for maps this size - the tree's job here is answering collision queries,
            // where it saves far more than it would as a visibility cull. The leaf-to-face
            // lists are compiled in and ready for when that changes.
            struct TextureBatch
            {
                int texture = -1;
                Mesh mesh = { 0 };
                bool transparent = false;
            };

            struct ModelBatches
            {
                int model = 0;
                std::vector<TextureBatch> batches;
            };

        private:
            CompiledMap data;

            // Index-aligned with data.textures. A texture that could not be found is the
            // placeholder, so a missing file shows up as obviously wrong geometry rather
            // than as an untextured void.
            std::vector<Texture2D> textures;
            std::vector<ModelBatches> models;

            Texture2D placeholder = { 0 };

            // One material reused for every batch, with the texture swapped before each
            // draw. Each batch owning a material would mean a shader reference per texture
            // for no gain - they would all be the default shader.
            Material material = { 0 };
            bool material_ready = false;

            EntityWorld entities;

            bool loaded = false;
            std::string source_uri;

        public:
            Map() = default;
            ~Map();

            Map(const Map&) = delete;
            Map& operator=(const Map&) = delete;

        public:
            // Loads a compiled map through FileSystem and spawns its entities. Throws
            // std::runtime_error if the file is missing or is not a compiled map.
            void load(const std::string& uri);

            void unload();
            bool is_loaded() const;
            const std::string& get_source_uri() const;

        public:
            void update(float delta);

            // Submits the world and every brush entity. Call between BeginMode3D and
            // EndMode3D.
            void draw();

            // Face windings and leaf bounds as wireframe, for working out why something is
            // where it is.
            void draw_debug_wireframe(Color color = Color{ 0, 255, 0, 90 }) const;

        public:
            const CompiledMap& compiled() const;
            EntityWorld& world();

        public:
            unsigned int point_contents(Vector3 point) const;

            TraceResult trace_ray(Vector3 start, Vector3 end,
                unsigned int mask = CONTENTS_SOLID_MASK) const;

            TraceResult trace_box(Vector3 start, Vector3 end, Vector3 mins, Vector3 maxs,
                unsigned int mask = CONTENTS_SOLID_MASK) const;

        public:
            // Where the player belongs, from the map's info_player_start. False when the map
            // has none, which a game should treat as a map it cannot start.
            bool find_player_start(Vector3& origin, Vector3& angles) const;

        private:
            void load_textures();
            void build_meshes();
            void destroy_meshes();

            // A checkerboard standing in for a texture that could not be loaded.
            static Texture2D generate_placeholder();

            // Finds the image behind a texture name by trying each supported extension, so
            // a map does not have to be recompiled when an artist switches format.
            static std::string resolve_texture_uri(const std::string& name);
    };
}
