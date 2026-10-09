#include <IronHull/map/compile/BrushGeometry.hpp>

#include <algorithm>
#include <cctype>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>

namespace IronHull
{
    namespace
    {
        // The part of a texture name the content rules look at: no directory, lower case.
        // A mapper keeping tool textures in `tools/` should get the same behaviour as one
        // keeping them loose.
        std::string content_key(const std::string& name)
        {
            size_t slash = name.find_last_of("/\\");
            std::string base = slash == std::string::npos ? name : name.substr(slash + 1);

            for (char& character : base) {
                character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            }

            return base;
        }

        bool starts_with(const std::string& text, const char* prefix)
        {
            return text.rfind(prefix, 0) == 0;
        }

        const Vector3 AXIAL_NORMALS[6] = {
            { 1.0f, 0.0f, 0.0f },
            { -1.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
            { 0.0f, -1.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, -1.0f },
        };
    }

    bool CompileBrush::contains(Vector3 point, float epsilon) const
    {
        // A convex brush is the intersection of the half-spaces behind its planes, so a
        // point is inside exactly when it is behind every one of them.
        for (const CompileBrushSide& side : this->sides) {
            if (side.plane.distance_to(point) > epsilon) {
                return false;
            }
        }

        return !this->sides.empty();
    }

    bool CompileBrush::is_solid() const
    {
        return (this->contents & CONTENTS_SOLID_MASK) != 0u;
    }

    unsigned int BrushGeometry::contents_from_texture(const std::string& name)
    {
        std::string key = content_key(name);

        if (key == "clip") {
            return CONTENTS_SOLID | CONTENTS_CLIP;
        }

        if (key == "trigger") {
            return CONTENTS_TRIGGER;
        }

        if (starts_with(key, "sky")) {
            return CONTENTS_SOLID | CONTENTS_SKY;
        }

        if (starts_with(key, "water")) {
            return CONTENTS_WATER;
        }

        if (starts_with(key, "slime")) {
            return CONTENTS_SLIME;
        }

        if (starts_with(key, "lava")) {
            return CONTENTS_LAVA;
        }

        return CONTENTS_SOLID;
    }

    unsigned int BrushGeometry::surface_flags_from_texture(const std::string& name)
    {
        std::string key = content_key(name);

        if (key == "clip" || key == "trigger" || key == "skip" || key == "null" || key == "nodraw") {
            return SURFACE_NODRAW;
        }

        if (starts_with(key, "sky")) {
            // Sky faces are marked but not drawn as geometry: the renderer fills the
            // background behind them instead, so a textured polygon would only get in the
            // way.
            return SURFACE_SKY | SURFACE_NODRAW;
        }

        if (starts_with(key, "water") || starts_with(key, "slime") || starts_with(key, "lava")) {
            return SURFACE_TRANSPARENT;
        }

        return SURFACE_NONE;
    }

    Vector2 BrushGeometry::texture_uv(const MapFaceTexture& texture, Vector3 point)
    {
        float u_scale = texture.u_scale != 0.0f ? texture.u_scale : 1.0f;
        float v_scale = texture.v_scale != 0.0f ? texture.v_scale : 1.0f;

        return Vector2{
            Vector3DotProduct(point, texture.u_axis) / u_scale + texture.u_shift,
            Vector3DotProduct(point, texture.v_axis) / v_scale + texture.v_shift,
        };
    }

    bool BrushGeometry::build(const MapBrush& brush, int entity, int order, CompileBrush& out)
    {
        out.sides.clear();
        out.entity = entity;
        out.order = order;
        out.contents = CONTENTS_EMPTY;

        if (brush.faces.size() < 4) {
            return false;
        }

        for (const MapFace& face : brush.faces) {
            Plane plane = face.plane();

            // Three colinear or coincident points describe no plane. The brush is salvaged
            // by ignoring the bad side rather than thrown away, so one sloppy face does not
            // cost the mapper the whole solid.
            if (!plane.is_valid()) {
                continue;
            }

            CompileBrushSide side;
            side.plane = plane;
            side.texture = face.texture;
            side.surface_flags = BrushGeometry::surface_flags_from_texture(face.texture.name);
            out.sides.push_back(side);

            out.contents |= BrushGeometry::contents_from_texture(face.texture.name);
        }

        if (out.sides.size() < 4) {
            return false;
        }

        // A brush made of water or acting as a trigger is not something to walk into, even
        // though its sides each read as solid by default. Any non-solid classification on
        // the brush wins over the default.
        if ((out.contents & (CONTENTS_LIQUID_MASK | CONTENTS_TRIGGER)) != 0u) {
            out.contents &= ~static_cast<unsigned int>(CONTENTS_SOLID);
        }

        // Cut each side back with every other side. A plane that survives with area to
        // spare is part of the brush's surface; one that does not is still a bounding plane,
        // it just contributes no polygon.
        for (size_t index = 0; index < out.sides.size(); ++index) {
            Winding winding = Winding::from_plane(out.sides[index].plane);

            for (size_t other = 0; other < out.sides.size(); ++other) {
                if (other == index || winding.points.size() < 3) {
                    continue;
                }

                winding = winding.clipped(out.sides[other].plane, PlaneSide::BACK);
            }

            winding.simplify();

            if (winding.is_valid()) {
                out.sides[index].winding = winding;
            } else {
                out.sides[index].winding.points.clear();
                out.sides[index].visible = false;
            }
        }

        out.bounds = Bounds::empty();
        int solid_sides = 0;

        for (const CompileBrushSide& side : out.sides) {
            if (side.winding.points.empty()) {
                continue;
            }

            ++solid_sides;
            out.bounds = Bounds::merge(out.bounds, side.winding.bounds());
        }

        // Fewer than four surfaces means the planes never closed off a volume.
        if (solid_sides < 4) {
            return false;
        }

        BrushGeometry::add_bevels(out);

        return true;
    }

    void BrushGeometry::add_bevels(CompileBrush& brush)
    {
        for (int axis = 0; axis < 6; ++axis) {
            Vector3 normal = AXIAL_NORMALS[axis];

            bool already_present = false;
            for (const CompileBrushSide& side : brush.sides) {
                if (Vector3DotProduct(side.plane.normal, normal) > 1.0f - NORMAL_EPSILON) {
                    already_present = true;
                    break;
                }
            }

            if (already_present) {
                continue;
            }

            // The brush's own bound in this direction, which is where a plane facing this
            // way would have to sit to touch it without cutting into it.
            const float* mins[3] = { &brush.bounds.min.x, &brush.bounds.min.y, &brush.bounds.min.z };
            const float* maxs[3] = { &brush.bounds.max.x, &brush.bounds.max.y, &brush.bounds.max.z };

            int component = axis / 2;
            bool positive = axis % 2 == 0;

            CompileBrushSide bevel;
            bevel.plane.normal = normal;
            bevel.plane.dist = positive ? *maxs[component] : -*mins[component];
            bevel.is_bevel = true;
            bevel.visible = false;
            bevel.surface_flags = SURFACE_NODRAW;

            brush.sides.push_back(bevel);
        }
    }
}
