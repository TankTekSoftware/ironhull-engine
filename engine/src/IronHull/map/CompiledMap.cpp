#include <IronHull/map/CompiledMap.hpp>

#include <algorithm>
#include <cmath>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>

namespace IronHull
{
    namespace
    {
        // How far short of a surface a sweep is stopped. Landing exactly on the plane leaves
        // the moving body ambiguously on the boundary, and the next frame's trace is then as
        // likely to start inside the wall as outside it.
        constexpr float TRACE_EPSILON = 0.03125f;

        // Walks the BSP tree clipping a line segment, narrowing in on the first surface the
        // caller cares about.
        //
        // This is the classic recursive hull check: at each node the segment is split at the
        // plane, the near half is followed first, and the moment the near half reaches a leaf
        // the caller cares about, the recursion unwinds with that hit - so the first surface
        // found along the line is always the nearest one.
        class RayTracer
        {
            private:
                const CompiledMap& map;
                unsigned int mask;

            public:
                TraceResult result;

            public:
                RayTracer(const CompiledMap& compiled_map, unsigned int content_mask)
                    : map(compiled_map), mask(content_mask)
                {
                    this->result.all_solid = true;
                }

            public:
                unsigned int contents_at(int child, Vector3 point) const
                {
                    while (!CompiledMap::child_is_leaf(child)) {
                        const BspNode& node = this->map.nodes[static_cast<size_t>(child)];
                        const Plane& plane = this->map.planes[static_cast<size_t>(node.plane)];

                        child = plane.distance_to(point) >= 0.0f ? node.children[0] : node.children[1];
                    }

                    int leaf = CompiledMap::child_to_leaf(child);
                    if (leaf < 0 || leaf >= static_cast<int>(this->map.leaves.size())) {
                        return CONTENTS_SOLID;
                    }

                    return this->map.leaves[static_cast<size_t>(leaf)].contents;
                }

                bool blocks(unsigned int contents) const
                {
                    return (contents & this->mask) != 0u;
                }

                // Returns false once the trace has found its hit, which unwinds the whole
                // recursion without examining anything further along the line.
                bool recurse(int child, float near_fraction, float far_fraction, Vector3 near_point, Vector3 far_point)
                {
                    if (CompiledMap::child_is_leaf(child)) {
                        int leaf_index = CompiledMap::child_to_leaf(child);

                        unsigned int contents = CONTENTS_SOLID;
                        if (leaf_index >= 0 && leaf_index < static_cast<int>(this->map.leaves.size())) {
                            contents = this->map.leaves[static_cast<size_t>(leaf_index)].contents;
                        }

                        if (this->blocks(contents)) {
                            this->result.started_solid = true;
                        } else {
                            this->result.all_solid = false;
                        }

                        return true;
                    }

                    const BspNode& node = this->map.nodes[static_cast<size_t>(child)];
                    const Plane& plane = this->map.planes[static_cast<size_t>(node.plane)];

                    float near_distance = plane.distance_to(near_point);
                    float far_distance = plane.distance_to(far_point);

                    if (near_distance >= 0.0f && far_distance >= 0.0f) {
                        return this->recurse(node.children[0], near_fraction, far_fraction, near_point, far_point);
                    }

                    if (near_distance < 0.0f && far_distance < 0.0f) {
                        return this->recurse(node.children[1], near_fraction, far_fraction, near_point, far_point);
                    }

                    // Bias the crossing point back towards the near side so the midpoint
                    // lands just short of the plane rather than on it.
                    float fraction = near_distance < 0.0f
                        ? (near_distance + TRACE_EPSILON) / (near_distance - far_distance)
                        : (near_distance - TRACE_EPSILON) / (near_distance - far_distance);
                    fraction = std::clamp(fraction, 0.0f, 1.0f);

                    float middle_fraction = near_fraction + (far_fraction - near_fraction) * fraction;
                    Vector3 middle = Vector3Lerp(near_point, far_point, fraction);

                    int near_side = near_distance < 0.0f ? 1 : 0;
                    int far_side = near_side ^ 1;

                    if (!this->recurse(node.children[near_side], near_fraction, middle_fraction, near_point, middle)) {
                        return false;
                    }

                    // Only cross the plane if whatever is on the other side is passable.
                    if (!this->blocks(this->contents_at(node.children[far_side], middle))) {
                        return this->recurse(node.children[far_side], middle_fraction, far_fraction, middle, far_point);
                    }

                    if (this->result.all_solid) {
                        return false;
                    }

                    // The far side is blocked, so the plane just crossed is the surface that
                    // was struck. Its normal is flipped when approaching from behind, so that
                    // it always points back at where the trace came from.
                    this->result.normal = near_side == 0 ? plane.normal : Vector3Negate(plane.normal);
                    this->result.contents = this->contents_at(node.children[far_side], middle);
                    this->result.fraction = middle_fraction;
                    this->result.position = middle;
                    this->result.hit = true;

                    return false;
                }
        };
    }

    std::string BspEntity::get(const std::string& key, const std::string& fallback) const
    {
        for (const MapKeyValue& pair : this->keyvalues) {
            if (pair.key == key) {
                return pair.value;
            }
        }

        return fallback;
    }

    std::string BspEntity::classname() const
    {
        return this->get("classname");
    }

    int CompiledMap::leaf_to_child(int leaf_index)
    {
        return -(leaf_index + 1);
    }

    int CompiledMap::child_to_leaf(int child)
    {
        return -child - 1;
    }

    bool CompiledMap::child_is_leaf(int child)
    {
        return child < 0;
    }

    int CompiledMap::find_leaf(Vector3 point, int model) const
    {
        if (model < 0 || model >= static_cast<int>(this->models.size()) || this->leaves.empty()) {
            return -1;
        }

        int child = this->models[static_cast<size_t>(model)].root_node;

        while (!CompiledMap::child_is_leaf(child)) {
            if (child >= static_cast<int>(this->nodes.size())) {
                return -1;
            }

            const BspNode& node = this->nodes[static_cast<size_t>(child)];
            const Plane& plane = this->planes[static_cast<size_t>(node.plane)];

            child = plane.distance_to(point) >= 0.0f ? node.children[0] : node.children[1];
        }

        int leaf = CompiledMap::child_to_leaf(child);
        return leaf < static_cast<int>(this->leaves.size()) ? leaf : -1;
    }

    unsigned int CompiledMap::point_contents(Vector3 point, int model) const
    {
        int leaf = this->find_leaf(point, model);
        if (leaf < 0) {
            return CONTENTS_EMPTY;
        }

        return this->leaves[static_cast<size_t>(leaf)].contents;
    }

    TraceResult CompiledMap::trace_ray(Vector3 start, Vector3 end, unsigned int mask, int model) const
    {
        TraceResult result;
        result.fraction = 1.0f;
        result.position = end;

        if (model < 0 || model >= static_cast<int>(this->models.size()) || this->leaves.empty()) {
            return result;
        }

        RayTracer tracer(*this, mask);
        tracer.recurse(this->models[static_cast<size_t>(model)].root_node, 0.0f, 1.0f, start, end);

        result = tracer.result;

        if (!result.hit) {
            result.fraction = 1.0f;
            result.position = end;
        }

        if (result.all_solid) {
            result.started_solid = true;
            result.fraction = 0.0f;
            result.position = start;
            result.hit = true;
        }

        return result;
    }

    void CompiledMap::collect_leaf_brushes(int child, const BoundingBox& box, unsigned int mask,
        std::vector<int>& out_brushes) const
    {
        if (CompiledMap::child_is_leaf(child)) {
            int leaf_index = CompiledMap::child_to_leaf(child);
            if (leaf_index < 0 || leaf_index >= static_cast<int>(this->leaves.size())) {
                return;
            }

            const BspLeaf& leaf = this->leaves[static_cast<size_t>(leaf_index)];

            for (int offset = 0; offset < leaf.leaf_brush_count; ++offset) {
                size_t slot = static_cast<size_t>(leaf.first_leaf_brush + offset);
                if (slot >= this->leaf_brushes.size()) {
                    break;
                }

                int brush_index = this->leaf_brushes[slot];
                if (brush_index < 0 || brush_index >= static_cast<int>(this->brushes.size())) {
                    continue;
                }

                const BspBrush& brush = this->brushes[static_cast<size_t>(brush_index)];
                if ((brush.contents & mask) == 0u || !Bounds::overlaps(brush.bounds, box)) {
                    continue;
                }

                out_brushes.push_back(brush_index);
            }

            return;
        }

        if (child >= static_cast<int>(this->nodes.size())) {
            return;
        }

        const BspNode& node = this->nodes[static_cast<size_t>(child)];
        if (!Bounds::overlaps(node.bounds, box)) {
            return;
        }

        this->collect_leaf_brushes(node.children[0], box, mask, out_brushes);
        this->collect_leaf_brushes(node.children[1], box, mask, out_brushes);
    }

    void CompiledMap::clip_box_to_brush(const BspBrush& brush, Vector3 start, Vector3 end,
        Vector3 mins, Vector3 maxs, TraceResult& result) const
    {
        if (brush.side_count == 0) {
            return;
        }

        // Sweeping a box through a convex brush is the same problem as sweeping a point
        // through that brush with every one of its planes pushed outwards by the box's
        // extent along that plane's normal. That reduces the whole thing to finding the
        // interval of the line that lies inside all of the shifted half-spaces: the sweep
        // enters at the latest entry and leaves at the earliest exit, and only hits
        // something if it enters before it leaves.
        float enter_fraction = -1.0f;
        float leave_fraction = 1.0f;
        const Plane* clip_plane = nullptr;

        bool starts_outside = false;
        bool ends_outside = false;

        for (int offset = 0; offset < brush.side_count; ++offset) {
            size_t slot = static_cast<size_t>(brush.first_side + offset);
            if (slot >= this->brush_sides.size()) {
                break;
            }

            const Plane& plane = this->planes[static_cast<size_t>(this->brush_sides[slot].plane)];

            // The corner of the box that reaches furthest towards the plane.
            Vector3 support = {
                plane.normal.x < 0.0f ? maxs.x : mins.x,
                plane.normal.y < 0.0f ? maxs.y : mins.y,
                plane.normal.z < 0.0f ? maxs.z : mins.z,
            };

            float distance = plane.dist - Vector3DotProduct(support, plane.normal);

            float start_distance = Vector3DotProduct(start, plane.normal) - distance;
            float end_distance = Vector3DotProduct(end, plane.normal) - distance;

            // Sitting exactly on a surface counts as outside it, not inside. A body that
            // just landed is left standing a hair off the floor by the bias below, and one
            // placed by hand tends to be put at exactly floor level - in both cases it is
            // resting on the surface, not embedded in it, and reporting it as stuck would
            // wedge it in place permanently. The tolerance matches the standoff the sweep
            // leaves, so the two agree about where "touching" is.
            if (start_distance > -TRACE_EPSILON) {
                starts_outside = true;
            }

            if (end_distance > -TRACE_EPSILON) {
                ends_outside = true;
            }

            // Both ends sit outside this one plane, so the whole segment does - a half-space
            // is convex and the brush is the intersection of all of them. Nothing about the
            // other sides can bring the line back inside, so stop here.
            //
            // The same tolerance as above, and for the same reason. A body walking along a
            // floor is exactly level with the top of every floor brush it crosses, so each
            // one's top plane reads zero at both ends. Testing strictly, that is "not
            // outside", and the body would then be found colliding with the vertical side
            // of the next floor brush along - catching on a seam in flat ground, which is
            // the single most familiar bug in a map of this kind.
            if (start_distance > -TRACE_EPSILON && end_distance > -TRACE_EPSILON) {
                return;
            }

            if (start_distance <= 0.0f && end_distance <= 0.0f) {
                continue;
            }

            if (start_distance > end_distance) {
                float fraction = (start_distance - TRACE_EPSILON) / (start_distance - end_distance);
                if (fraction > enter_fraction) {
                    enter_fraction = fraction;
                    clip_plane = &plane;
                }
            } else {
                float fraction = (start_distance + TRACE_EPSILON) / (start_distance - end_distance);
                if (fraction < leave_fraction) {
                    leave_fraction = fraction;
                }
            }
        }

        if (!starts_outside) {
            result.started_solid = true;
            result.contents |= brush.contents;

            if (!ends_outside) {
                result.all_solid = true;
                result.fraction = 0.0f;
                result.position = start;
                result.hit = true;
            }

            return;
        }

        if (enter_fraction >= leave_fraction || clip_plane == nullptr) {
            return;
        }

        if (enter_fraction < result.fraction) {
            result.fraction = std::max(enter_fraction, 0.0f);
            result.normal = clip_plane->normal;
            result.contents = brush.contents;
            result.hit = true;
        }
    }

    TraceResult CompiledMap::trace_box(Vector3 start, Vector3 end, Vector3 mins, Vector3 maxs,
        unsigned int mask, int model) const
    {
        TraceResult result;
        result.fraction = 1.0f;
        result.position = end;

        if (model < 0 || model >= static_cast<int>(this->models.size())) {
            return result;
        }

        // A zero-sized box has no volume for the plane-pushing above to work with, and a
        // point sweep through the tree is both exact and cheaper anyway.
        if (mins.x == 0.0f && mins.y == 0.0f && mins.z == 0.0f
            && maxs.x == 0.0f && maxs.y == 0.0f && maxs.z == 0.0f) {
            return this->trace_ray(start, end, mask, model);
        }

        BoundingBox swept = Bounds::empty();
        swept = Bounds::add_point(swept, Vector3Add(start, mins));
        swept = Bounds::add_point(swept, Vector3Add(start, maxs));
        swept = Bounds::add_point(swept, Vector3Add(end, mins));
        swept = Bounds::add_point(swept, Vector3Add(end, maxs));

        std::vector<int> candidates;
        this->collect_leaf_brushes(this->models[static_cast<size_t>(model)].root_node, swept, mask, candidates);

        // A brush spanning several leaves is reached once per leaf; testing it more than
        // once is wasted work and would double-report a start-solid.
        std::sort(candidates.begin(), candidates.end());
        candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

        for (int brush_index : candidates) {
            this->clip_box_to_brush(this->brushes[static_cast<size_t>(brush_index)], start, end, mins, maxs, result);

            if (result.all_solid || result.fraction == 0.0f) {
                break;
            }
        }

        result.position = Vector3Lerp(start, end, result.fraction);
        return result;
    }

    std::vector<const BspEntity*> CompiledMap::find_entities(const std::string& classname) const
    {
        std::vector<const BspEntity*> found;

        for (const BspEntity& entity : this->entities) {
            if (entity.classname() == classname) {
                found.push_back(&entity);
            }
        }

        return found;
    }

    const BspEntity* CompiledMap::worldspawn() const
    {
        for (const BspEntity& entity : this->entities) {
            if (entity.classname() == "worldspawn") {
                return &entity;
            }
        }

        return nullptr;
    }
}
