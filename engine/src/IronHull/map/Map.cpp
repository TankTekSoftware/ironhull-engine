#include <IronHull/map/Map.hpp>

#include <algorithm>
#include <cstdio>
#include <unordered_map>

#include <raymath.h>

#include <IronHull/io/FileSystem.hpp>
#include <IronHull/map/BspFile.hpp>

namespace IronHull
{
    namespace
    {
        // Where a face's texture name is resolved against, and the formats tried in order.
        const char* TEXTURE_ROOT = "content://assets/textures/";
        const char* TEXTURE_EXTENSIONS[] = { ".png", ".jpg", ".jpeg" };

        // There is no lighting system yet, and a map lit by nothing at all reads as a flat
        // silhouette - walls, floor and ceiling all exactly the same shade, with no edge
        // anywhere. Baking a fixed directional term into the vertex colours costs nothing at
        // draw time and is enough to make the shape of a room legible. It is a stand-in, not
        // a lighting model: when real lights arrive this goes away.
        const Vector3 FILL_LIGHT_DIRECTION = { 0.37f, 0.84f, 0.40f };
        constexpr float FILL_AMBIENT = 0.55f;
        constexpr float FILL_RANGE = 0.45f;

        unsigned char shade_for_normal(Vector3 normal)
        {
            float lambert = Vector3DotProduct(Vector3Normalize(normal), FILL_LIGHT_DIRECTION);
            float shade = FILL_AMBIENT + FILL_RANGE * std::max(lambert, 0.0f);

            return static_cast<unsigned char>(std::clamp(shade, 0.0f, 1.0f) * 255.0f);
        }
    }

    Map::~Map()
    {
        this->unload();
    }

    std::string Map::resolve_texture_uri(const std::string& name)
    {
        for (const char* extension : TEXTURE_EXTENSIONS) {
            std::string uri = TEXTURE_ROOT + name + extension;

            if (FileSystem::exists(uri)) {
                return uri;
            }
        }

        return std::string();
    }

    Texture2D Map::generate_placeholder()
    {
        // Magenta against black: unmistakably a missing texture rather than something an
        // artist might have meant.
        Image image = GenImageChecked(64, 64, 8, 8, Color{ 255, 0, 255, 255 }, Color{ 24, 24, 24, 255 });
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);

        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);

        return texture;
    }

    void Map::load(const std::string& uri)
    {
        this->unload();

        this->data = BspFile::load(uri);
        this->source_uri = uri;
        this->loaded = true;

        this->load_textures();
        this->build_meshes();

        int spawned = this->entities.load(this->data);

        TraceLog(LOG_INFO, "MAP: Loaded '%s': %zu faces, %zu leaves, %zu models, %d entities",
            uri.c_str(), this->data.faces.size(), this->data.leaves.size(),
            this->data.models.size(), spawned);
    }

    void Map::load_textures()
    {
        this->placeholder = Map::generate_placeholder();
        this->textures.clear();
        this->textures.reserve(this->data.textures.size());

        for (const BspTexture& texture : this->data.textures) {
            std::string uri = Map::resolve_texture_uri(texture.name);

            if (uri.empty()) {
                TraceLog(LOG_WARNING, "MAP: texture '%s' not found under %s", texture.name.c_str(), TEXTURE_ROOT);
                this->textures.push_back(this->placeholder);
                continue;
            }

            Texture2D loaded_texture = LoadTexture(uri.c_str());

            if (loaded_texture.id == 0) {
                TraceLog(LOG_WARNING, "MAP: texture '%s' could not be loaded from %s",
                    texture.name.c_str(), uri.c_str());
                this->textures.push_back(this->placeholder);
                continue;
            }

            // Map surfaces are tiled, and a retro look wants crisp texels rather than a
            // smooth blur, so these differ from the engine's default sprite filtering.
            SetTextureFilter(loaded_texture, TEXTURE_FILTER_POINT);
            SetTextureWrap(loaded_texture, TEXTURE_WRAP_REPEAT);
            GenTextureMipmaps(&loaded_texture);

            this->textures.push_back(loaded_texture);
        }
    }

    void Map::build_meshes()
    {
        if (!this->material_ready) {
            this->material = LoadMaterialDefault();
            this->material_ready = true;
        }

        for (size_t model_index = 0; model_index < this->data.models.size(); ++model_index) {
            const BspModel& model = this->data.models[model_index];

            // Triangles are collected unindexed, per texture. raylib's Mesh indices are
            // 16-bit, which would cap a batch at 65535 vertices and force splitting; a plain
            // triangle list has no such limit and uploads just as well.
            std::unordered_map<int, std::vector<BspVertex>> by_texture;

            for (int offset = 0; offset < model.face_count; ++offset) {
                size_t face_index = static_cast<size_t>(model.first_face + offset);
                if (face_index >= this->data.faces.size()) {
                    break;
                }

                const BspFace& face = this->data.faces[face_index];

                if ((face.surface_flags & SURFACE_NODRAW) != 0u) {
                    continue;
                }

                std::vector<BspVertex>& triangles = by_texture[face.texture];

                for (int index = 0; index < face.index_count; ++index) {
                    size_t slot = static_cast<size_t>(face.first_index + index);
                    if (slot >= this->data.indices.size()) {
                        break;
                    }

                    unsigned int vertex_index = this->data.indices[slot];
                    if (vertex_index < this->data.vertices.size()) {
                        triangles.push_back(this->data.vertices[vertex_index]);
                    }
                }
            }

            ModelBatches batches;
            batches.model = static_cast<int>(model_index);

            for (auto& [texture_index, triangles] : by_texture) {
                if (triangles.size() < 3) {
                    continue;
                }

                Texture2D texture = texture_index >= 0 && texture_index < static_cast<int>(this->textures.size())
                    ? this->textures[static_cast<size_t>(texture_index)]
                    : this->placeholder;

                float texture_width = texture.width > 0 ? static_cast<float>(texture.width) : 64.0f;
                float texture_height = texture.height > 0 ? static_cast<float>(texture.height) : 64.0f;

                Mesh mesh = { 0 };
                mesh.vertexCount = static_cast<int>(triangles.size());
                mesh.triangleCount = mesh.vertexCount / 3;

                // raylib frees these with its own allocator in UnloadMesh, so they have to
                // come from MemAlloc rather than new or malloc.
                mesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh.vertexCount * 3 * sizeof(float))));
                mesh.texcoords = static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh.vertexCount * 2 * sizeof(float))));
                mesh.normals = static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh.vertexCount * 3 * sizeof(float))));
                mesh.colors = static_cast<unsigned char*>(MemAlloc(static_cast<unsigned int>(mesh.vertexCount * 4)));

                for (int vertex = 0; vertex < mesh.vertexCount; ++vertex) {
                    const BspVertex& source = triangles[static_cast<size_t>(vertex)];

                    mesh.vertices[vertex * 3 + 0] = source.position.x;
                    mesh.vertices[vertex * 3 + 1] = source.position.y;
                    mesh.vertices[vertex * 3 + 2] = source.position.z;

                    // The compiled UVs are in texels, which is what lets the same map work
                    // with a texture at any resolution. Dividing by the size it actually
                    // loaded at is what turns them into the 0..1 the GPU wants.
                    mesh.texcoords[vertex * 2 + 0] = source.uv.x / texture_width;
                    mesh.texcoords[vertex * 2 + 1] = source.uv.y / texture_height;

                    mesh.normals[vertex * 3 + 0] = source.normal.x;
                    mesh.normals[vertex * 3 + 1] = source.normal.y;
                    mesh.normals[vertex * 3 + 2] = source.normal.z;

                    unsigned char shade = shade_for_normal(source.normal);
                    mesh.colors[vertex * 4 + 0] = shade;
                    mesh.colors[vertex * 4 + 1] = shade;
                    mesh.colors[vertex * 4 + 2] = shade;
                    mesh.colors[vertex * 4 + 3] = 255;
                }

                UploadMesh(&mesh, false);

                TextureBatch batch;
                batch.texture = texture_index;
                batch.mesh = mesh;
                batch.transparent = texture_index >= 0 && texture_index < static_cast<int>(this->data.textures.size())
                    && (this->data.textures[static_cast<size_t>(texture_index)].surface_flags & SURFACE_TRANSPARENT) != 0u;

                batches.batches.push_back(batch);
            }

            // Transparent surfaces have to go down after everything behind them, or they
            // blend against whatever happened to be drawn first.
            std::stable_sort(batches.batches.begin(), batches.batches.end(),
                [](const TextureBatch& a, const TextureBatch& b) {
                    return !a.transparent && b.transparent;
                });

            this->models.push_back(std::move(batches));
        }
    }

    void Map::destroy_meshes()
    {
        for (ModelBatches& model : this->models) {
            for (TextureBatch& batch : model.batches) {
                UnloadMesh(batch.mesh);
            }
        }

        this->models.clear();
    }

    void Map::unload()
    {
        if (!this->loaded) {
            return;
        }

        this->entities.clear();
        this->destroy_meshes();

        // Several names can resolve to the placeholder, so unloading the list blindly would
        // free the same texture more than once.
        for (Texture2D& texture : this->textures) {
            if (texture.id != 0 && texture.id != this->placeholder.id) {
                UnloadTexture(texture);
            }
        }

        this->textures.clear();

        if (this->placeholder.id != 0) {
            UnloadTexture(this->placeholder);
            this->placeholder = Texture2D{ 0 };
        }

        if (this->material_ready) {
            // Only the map array needs releasing. UnloadMaterial would also take the shader
            // and the bound texture with it - the shader is raylib's shared default, and the
            // texture has already been unloaded above.
            MemFree(this->material.maps);
            this->material.maps = nullptr;
            this->material_ready = false;
        }

        this->data = CompiledMap();
        this->source_uri.clear();
        this->loaded = false;
    }

    bool Map::is_loaded() const
    {
        return this->loaded;
    }

    const std::string& Map::get_source_uri() const
    {
        return this->source_uri;
    }

    void Map::update(float delta)
    {
        if (!this->loaded) {
            return;
        }

        this->entities.update(delta);
    }

    void Map::draw()
    {
        if (!this->loaded || !this->material_ready) {
            return;
        }

        // A brush entity's geometry is compiled where it was authored, and the entity's
        // origin is an offset from there - which is how a door that has slid open draws in
        // its new position without its mesh being rebuilt.
        std::unordered_map<int, Vector3> model_offsets;

        for (Entity* entity : this->entities.all()) {
            if (entity->is_brush_entity()) {
                model_offsets[entity->get_model()] = entity->get_origin();
            }
        }

        for (const ModelBatches& model : this->models) {
            Matrix transform = MatrixIdentity();

            auto offset = model_offsets.find(model.model);
            if (offset != model_offsets.end()) {
                transform = MatrixTranslate(offset->second.x, offset->second.y, offset->second.z);
            }

            for (const TextureBatch& batch : model.batches) {
                Texture2D texture = batch.texture >= 0 && batch.texture < static_cast<int>(this->textures.size())
                    ? this->textures[static_cast<size_t>(batch.texture)]
                    : this->placeholder;

                this->material.maps[MATERIAL_MAP_DIFFUSE].texture = texture;
                this->material.maps[MATERIAL_MAP_DIFFUSE].color = batch.transparent
                    ? Color{ 255, 255, 255, 160 }
                    : WHITE;

                DrawMesh(batch.mesh, this->material, transform);
            }
        }
    }

    void Map::draw_debug_wireframe(Color color) const
    {
        if (!this->loaded) {
            return;
        }

        for (const BspFace& face : this->data.faces) {
            for (int corner = 0; corner < face.vertex_count; ++corner) {
                size_t from = static_cast<size_t>(face.first_vertex + corner);
                size_t to = static_cast<size_t>(face.first_vertex + (corner + 1) % face.vertex_count);

                if (from < this->data.vertices.size() && to < this->data.vertices.size()) {
                    DrawLine3D(this->data.vertices[from].position, this->data.vertices[to].position, color);
                }
            }
        }
    }

    const CompiledMap& Map::compiled() const
    {
        return this->data;
    }

    EntityWorld& Map::world()
    {
        return this->entities;
    }

    unsigned int Map::point_contents(Vector3 point) const
    {
        return this->data.point_contents(point);
    }

    TraceResult Map::trace_ray(Vector3 start, Vector3 end, unsigned int mask) const
    {
        return this->data.trace_ray(start, end, mask);
    }

    TraceResult Map::trace_box(Vector3 start, Vector3 end, Vector3 mins, Vector3 maxs, unsigned int mask) const
    {
        return this->data.trace_box(start, end, mins, maxs, mask);
    }

    bool Map::find_player_start(Vector3& origin, Vector3& angles) const
    {
        std::vector<const BspEntity*> starts = this->data.find_entities("info_player_start");

        if (starts.empty()) {
            return false;
        }

        std::string origin_text = starts[0]->get("origin");
        std::string angles_text = starts[0]->get("angles");

        origin = Vector3{ 0.0f, 0.0f, 0.0f };
        angles = Vector3{ 0.0f, 0.0f, 0.0f };

        std::sscanf(origin_text.c_str(), "%f %f %f", &origin.x, &origin.y, &origin.z);
        std::sscanf(angles_text.c_str(), "%f %f %f", &angles.x, &angles.y, &angles.z);

        return true;
    }
}
