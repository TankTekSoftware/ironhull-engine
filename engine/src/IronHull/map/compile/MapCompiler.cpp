#include <IronHull/map/compile/MapCompiler.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <unordered_map>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>
#include <IronHull/map/MapSpace.hpp>
#include <IronHull/map/compile/BrushGeometry.hpp>
#include <IronHull/map/compile/BspBuilder.hpp>
#include <IronHull/map/compile/FaceCsg.hpp>
#include <IronHull/map/compile/PortalFlood.hpp>

namespace IronHull
{
    namespace
    {
        // Everything belonging to one separately-rooted piece of the map: the world, or one
        // brush entity.
        struct ModelGroup
        {
            int entity = 0;
            std::vector<int> brushes;
        };

        Vector3 parse_vector(const std::string& text)
        {
            Vector3 value = { 0.0f, 0.0f, 0.0f };

            if (!text.empty()) {
                std::sscanf(text.c_str(), "%f %f %f", &value.x, &value.y, &value.z);
            }

            return value;
        }

        std::string format_vector(Vector3 value)
        {
            char buffer[96];
            std::snprintf(buffer, sizeof(buffer), "%.6g %.6g %.6g",
                static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z));

            return std::string(buffer);
        }

        // Interns a texture name, so the compiled map carries each one once no matter how
        // many faces use it.
        class TextureTable
        {
            private:
                std::unordered_map<std::string, int> lookup;

            public:
                std::vector<BspTexture> textures;

            public:
                int add(const std::string& name)
                {
                    auto existing = this->lookup.find(name);
                    if (existing != this->lookup.end()) {
                        return existing->second;
                    }

                    BspTexture texture;
                    texture.name = name;
                    texture.surface_flags = BrushGeometry::surface_flags_from_texture(name);

                    int index = static_cast<int>(this->textures.size());
                    this->textures.push_back(texture);
                    this->lookup.emplace(name, index);

                    return index;
                }
        };
    }

    CompiledMap MapCompiler::compile(const MapFile& map, const Options& options, Report& report)
    {
        std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();

        CompiledMap compiled;
        PlaneSet planes;
        TextureTable textures;

        // --- STAGE 1: BRUSH GEOMETRY --- //

        std::vector<CompileBrush> brushes;
        std::vector<ModelGroup> groups;

        // The world is always model 0, whether or not it has any brushes, so that an entity
        // referring to "*0" always means the world.
        groups.push_back(ModelGroup{ -1, {} });

        int brush_order = 0;

        for (size_t entity_index = 0; entity_index < map.entities.size(); ++entity_index) {
            const MapEntity& entity = map.entities[entity_index];

            if (entity.brushes.empty()) {
                continue;
            }

            // Worldspawn's brushes are the world; any other entity with brushes is a brush
            // entity, and gets a model of its own so it can move and be collided with
            // independently of the level around it.
            ModelGroup* group = nullptr;

            if (entity.is_worldspawn()) {
                groups[0].entity = static_cast<int>(entity_index);
                group = &groups[0];
            } else {
                groups.push_back(ModelGroup{ static_cast<int>(entity_index), {} });
                group = &groups.back();
            }

            for (size_t brush_index = 0; brush_index < entity.brushes.size(); ++brush_index) {
                CompileBrush brush;

                if (!BrushGeometry::build(entity.brushes[brush_index], static_cast<int>(entity_index), brush_order, brush)) {
                    ++report.invalid_brushes;
                    report.warnings.push_back("entity " + std::to_string(entity_index) + " brush "
                        + std::to_string(brush_index) + " encloses no volume and was skipped");
                    continue;
                }

                ++brush_order;
                group->brushes.push_back(static_cast<int>(brushes.size()));
                brushes.push_back(std::move(brush));
            }
        }

        report.brushes = static_cast<int>(brushes.size());

        // --- STAGE 2: HIDDEN SURFACE REMOVAL --- //

        std::vector<CompileFace> all_faces;

        if (options.face_csg) {
            all_faces = FaceCsg::run(brushes);
        } else {
            for (size_t index = 0; index < brushes.size(); ++index) {
                for (const CompileBrushSide& side : brushes[index].sides) {
                    if (side.is_bevel || side.winding.points.empty() || (side.surface_flags & SURFACE_NODRAW) != 0u) {
                        continue;
                    }

                    CompileFace face;
                    face.plane = side.plane;
                    face.winding = side.winding;
                    face.texture = side.texture;
                    face.surface_flags = side.surface_flags;
                    face.brush = static_cast<int>(index);
                    face.entity = brushes[index].entity;

                    all_faces.push_back(std::move(face));
                }
            }
        }

        // --- STAGE 3 AND 4: ONE TREE PER MODEL --- //

        // Flooding starts from the point entities. An entity sitting in open space outside
        // the walls will therefore flood the void and be reported as a leak, which is the
        // intended behaviour: it is a mapping mistake either way.
        std::vector<Vector3> seeds;
        std::vector<std::string> seed_names;

        for (const MapEntity& entity : map.entities) {
            if (!entity.brushes.empty()) {
                continue;
            }

            const std::string* origin = entity.find("origin");
            if (origin == nullptr) {
                continue;
            }

            seeds.push_back(parse_vector(*origin));
            seed_names.push_back(entity.classname());
        }

        if (seeds.empty() && options.cull_outside) {
            report.warnings.push_back("no point entities to flood from, so the map's outward-facing"
                " surfaces could not be identified - add an info_player_start");
        }

        // Which model each entity's brushes ended up in, so the entity can be pointed at it.
        std::unordered_map<int, int> entity_model;

        for (size_t group_index = 0; group_index < groups.size(); ++group_index) {
            const ModelGroup& group = groups[group_index];
            bool is_world = group_index == 0;

            BspBuilder::Options build_options;
            build_options.max_depth = options.max_depth;

            BspBuilder::Stats build_stats;
            BspTree tree = BspBuilder::build(brushes, group.brushes, planes, build_options, build_stats);

            report.nodes += build_stats.nodes;
            report.leaves += build_stats.leaves;
            report.solid_leaves += build_stats.solid_leaves;
            report.max_depth = std::max(report.max_depth, build_stats.max_depth);

            // Only the world is flooded. A brush entity is a closed object a few brushes
            // big - there is no inside or outside of a door to tell apart, and flooding it
            // would just report it as leaking.
            bool cull = false;

            if (is_world) {
                PortalFlood::Result flood = PortalFlood::run(tree, seeds, seed_names);

                report.areas = flood.area_count;
                report.leaked = flood.leaked;
                report.leak_point = flood.leak_point;
                report.leak_entity = flood.leak_entity;

                if (flood.leaked) {
                    report.warnings.push_back("the map leaks: open space reached the edge of the world from "
                        + (flood.leak_entity.empty() ? std::string("an entity") : flood.leak_entity)
                        + " at " + format_vector(flood.leak_point)
                        + " - seal the hole, or nothing outward-facing can be culled");
                }

                cull = options.cull_outside && !flood.leaked && flood.area_count > 0;
            }

            // Faces belonging to this model only. Their indices are local to this list,
            // which is what the tree's leaves will refer to.
            std::vector<CompileFace> model_faces;

            for (const CompileFace& face : all_faces) {
                if (face.entity == group.entity) {
                    model_faces.push_back(face);
                }
            }

            PortalFlood::assign_faces(tree, model_faces, cull);

            // --- STAGE 5: EMIT --- //

            int node_offset = static_cast<int>(compiled.nodes.size());
            int leaf_offset = static_cast<int>(compiled.leaves.size());
            int face_offset = static_cast<int>(compiled.faces.size());
            int brush_offset = static_cast<int>(compiled.brushes.size());

            // Children are indices into arrays several models share, so each one has to be
            // shifted by where this model's own nodes and leaves landed.
            auto rebase_child = [&](int child) {
                if (CompiledMap::child_is_leaf(child)) {
                    return CompiledMap::leaf_to_child(leaf_offset + CompiledMap::child_to_leaf(child));
                }

                return node_offset + child;
            };

            BspModel model;
            model.first_face = face_offset;
            model.first_brush = brush_offset;
            model.root_node = rebase_child(tree.root);
            model.bounds = MapSpace::bounds_to_engine(tree.bounds, options.scale);

            // Brushes, which is what box sweeps are tested against.
            std::unordered_map<int, int> brush_remap;

            for (int brush_index : group.brushes) {
                const CompileBrush& brush = brushes[static_cast<size_t>(brush_index)];

                BspBrush out_brush;
                out_brush.contents = brush.contents;
                out_brush.bounds = MapSpace::bounds_to_engine(brush.bounds, options.scale);
                out_brush.first_side = static_cast<int>(compiled.brush_sides.size());

                for (const CompileBrushSide& side : brush.sides) {
                    BspBrushSide out_side;
                    out_side.plane = planes.add(side.plane);
                    out_side.texture = side.is_bevel ? -1 : textures.add(side.texture.name);

                    compiled.brush_sides.push_back(out_side);
                }

                out_brush.side_count = static_cast<int>(compiled.brush_sides.size()) - out_brush.first_side;

                brush_remap.emplace(brush_index, static_cast<int>(compiled.brushes.size()));
                compiled.brushes.push_back(out_brush);
            }

            model.brush_count = static_cast<int>(compiled.brushes.size()) - model.first_brush;

            // Faces, triangulated as they go out.
            for (const CompileFace& face : model_faces) {
                if (!face.visible || !face.winding.is_valid()) {
                    ++report.culled_faces;
                    continue;
                }

                BspFace out_face;
                out_face.plane = planes.add(face.plane);
                out_face.texture = textures.add(face.texture.name);
                out_face.surface_flags = face.surface_flags;
                out_face.first_vertex = static_cast<int>(compiled.vertices.size());
                out_face.vertex_count = static_cast<int>(face.winding.points.size());

                Vector3 normal = MapSpace::to_engine(face.plane.normal);

                for (Vector3 point : face.winding.points) {
                    BspVertex vertex;

                    // Texture axes are authored in map space, so the texel coordinate has to
                    // be taken from the map-space position - before the point is converted.
                    vertex.uv = BrushGeometry::texture_uv(face.texture, point);
                    vertex.position = MapSpace::to_engine(point, options.scale);
                    vertex.normal = normal;

                    compiled.vertices.push_back(vertex);
                }

                // A convex polygon fans from any one of its corners. The map-space winding
                // is wound counter-clockwise seen from the front and the conversion to
                // engine space is a rotation, so the triangles come out front-facing
                // without needing to be reversed.
                out_face.first_index = static_cast<int>(compiled.indices.size());

                for (int corner = 2; corner < out_face.vertex_count; ++corner) {
                    compiled.indices.push_back(static_cast<unsigned int>(out_face.first_vertex));
                    compiled.indices.push_back(static_cast<unsigned int>(out_face.first_vertex + corner - 1));
                    compiled.indices.push_back(static_cast<unsigned int>(out_face.first_vertex + corner));
                }

                out_face.index_count = static_cast<int>(compiled.indices.size()) - out_face.first_index;

                compiled.faces.push_back(out_face);
            }

            model.face_count = static_cast<int>(compiled.faces.size()) - model.first_face;

            // A face that was culled leaves a gap in the numbering, so a leaf's face list
            // cannot simply be shifted - each entry is matched to where that face actually
            // landed, and dropped if it never made it out.
            std::unordered_map<int, int> face_remap;
            {
                int emitted = face_offset;

                for (size_t index = 0; index < model_faces.size(); ++index) {
                    if (model_faces[index].visible && model_faces[index].winding.is_valid()) {
                        face_remap.emplace(static_cast<int>(index), emitted);
                        ++emitted;
                    }
                }
            }

            for (const BspTree::Node& node : tree.nodes) {
                BspNode out_node;
                out_node.plane = node.plane_index;
                out_node.children[0] = rebase_child(node.children[0]);
                out_node.children[1] = rebase_child(node.children[1]);
                out_node.bounds = MapSpace::bounds_to_engine(node.bounds, options.scale);

                compiled.nodes.push_back(out_node);
            }

            for (const BspTree::Leaf& leaf : tree.leaves) {
                BspLeaf out_leaf;
                out_leaf.contents = leaf.contents;
                out_leaf.area = leaf.area;
                out_leaf.bounds = MapSpace::bounds_to_engine(leaf.bounds, options.scale);

                out_leaf.first_leaf_face = static_cast<int>(compiled.leaf_faces.size());

                for (int face_index : leaf.faces) {
                    auto emitted = face_remap.find(face_index);
                    if (emitted != face_remap.end()) {
                        compiled.leaf_faces.push_back(emitted->second);
                    }
                }

                out_leaf.leaf_face_count = static_cast<int>(compiled.leaf_faces.size()) - out_leaf.first_leaf_face;

                out_leaf.first_leaf_brush = static_cast<int>(compiled.leaf_brushes.size());

                for (int brush_index : leaf.brushes) {
                    auto emitted = brush_remap.find(brush_index);
                    if (emitted != brush_remap.end()) {
                        compiled.leaf_brushes.push_back(emitted->second);
                    }
                }

                out_leaf.leaf_brush_count = static_cast<int>(compiled.leaf_brushes.size()) - out_leaf.first_leaf_brush;

                compiled.leaves.push_back(out_leaf);
            }

            if (group.entity >= 0) {
                entity_model.emplace(group.entity, static_cast<int>(compiled.models.size()));
            }

            compiled.models.push_back(model);
        }

        // --- ENTITIES --- //

        for (size_t entity_index = 0; entity_index < map.entities.size(); ++entity_index) {
            const MapEntity& entity = map.entities[entity_index];

            BspEntity out_entity;
            out_entity.connections = entity.connections;

            for (const MapKeyValue& pair : entity.keyvalues) {
                MapKeyValue out_pair = pair;

                // Positions move into engine space with the geometry. Angles deliberately
                // do not: they are an Euler convention rather than a coordinate, and
                // rewriting them would stop an entity's "angles" matching what was authored.
                // MapSpace::angles_to_forward() is where that convention is applied.
                if (pair.key == "origin") {
                    out_pair.value = format_vector(MapSpace::to_engine(parse_vector(pair.value), options.scale));
                }

                out_entity.keyvalues.push_back(out_pair);
            }

            // Brush entities are pointed at the model holding their geometry, the same way a
            // .map refers to one. Worldspawn is model 0 by definition and needs no key.
            auto model = entity_model.find(static_cast<int>(entity_index));
            if (model != entity_model.end() && !entity.is_worldspawn()) {
                out_entity.keyvalues.push_back(MapKeyValue{ "model", "*" + std::to_string(model->second) });
            }

            compiled.entities.push_back(std::move(out_entity));
        }

        // --- PLANES --- //

        // Every plane the compiler interned is in map space; converting the whole table in
        // one pass keeps all the indices already handed out valid.
        compiled.planes.reserve(static_cast<size_t>(planes.count()));

        for (const Plane& plane : planes.all()) {
            compiled.planes.push_back(MapSpace::to_engine(plane, options.scale));
        }

        compiled.textures = std::move(textures.textures);

        report.faces = static_cast<int>(compiled.faces.size());
        report.vertices = static_cast<int>(compiled.vertices.size());
        report.triangles = static_cast<int>(compiled.indices.size() / 3);
        report.models = static_cast<int>(compiled.models.size());
        report.entities = static_cast<int>(compiled.entities.size());
        report.textures = static_cast<int>(compiled.textures.size());

        std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;
        report.seconds = elapsed.count();

        return compiled;
    }
}
