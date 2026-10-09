#include <IronHull/map/compile/BspBuilder.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>
#include <IronHull/map/CompiledMap.hpp>

namespace IronHull
{
    namespace
    {
        // A plane rounded to the tolerances Plane::snap() works to, used only to collapse
        // the many duplicate candidate planes a cell full of neighbouring brushes offers.
        // Two planes that differ by less than this were going to be snapped together anyway.
        using PlaneKey = std::array<int, 4>;

        PlaneKey plane_key(const Plane& plane)
        {
            return PlaneKey{
                static_cast<int>(std::lround(plane.normal.x * 4096.0f)),
                static_cast<int>(std::lround(plane.normal.y * 4096.0f)),
                static_cast<int>(std::lround(plane.normal.z * 4096.0f)),
                static_cast<int>(std::lround(plane.dist * 16.0f)),
            };
        }

        // Where a brush sits relative to a plane. The brush's winding points are its hull
        // corners, so testing them is an exact answer for the whole convex solid.
        PlaneSide classify_brush(const CompileBrush& brush, const Plane& plane)
        {
            bool has_front = false;
            bool has_back = false;

            for (const CompileBrushSide& side : brush.sides) {
                for (Vector3 point : side.winding.points) {
                    PlaneSide where = plane.classify_point(point);

                    if (where == PlaneSide::FRONT) {
                        has_front = true;
                    } else if (where == PlaneSide::BACK) {
                        has_back = true;
                    }

                    if (has_front && has_back) {
                        return PlaneSide::CROSS;
                    }
                }
            }

            if (has_front) {
                return PlaneSide::FRONT;
            }

            if (has_back) {
                return PlaneSide::BACK;
            }

            return PlaneSide::ON;
        }

        class Builder
        {
            private:
                const std::vector<CompileBrush>& brushes;
                PlaneSet& planes;
                const BspBuilder::Options& options;
                BspBuilder::Stats& stats;

            public:
                BspTree tree;

            public:
                Builder(const std::vector<CompileBrush>& brush_list, PlaneSet& plane_set,
                    const BspBuilder::Options& build_options, BspBuilder::Stats& build_stats)
                    : brushes(brush_list), planes(plane_set), options(build_options), stats(build_stats)
                {
                }

            public:
                int build(const ConvexVolume& volume, const std::vector<int>& brush_list, int depth)
                {
                    this->stats.max_depth = std::max(this->stats.max_depth, depth);

                    std::vector<Vector3> corners = volume.corners();

                    // A cell with no corners encloses nothing - a cut sliced away what an
                    // earlier cut had already taken. Nothing can occupy it, and calling it
                    // solid is what stops a sweep from slipping through the gap.
                    if (corners.size() < 4) {
                        return this->make_solid_leaf();
                    }

                    Plane split;
                    if (depth >= this->options.max_depth || !this->choose_split(corners, brush_list, split)) {
                        return this->make_leaf(volume, brush_list, corners);
                    }

                    // The node has to be reserved before recursing so it keeps its index,
                    // but it cannot be filled in until the children are known - and the
                    // vector may have grown and moved by then, so the reference is taken
                    // afterwards rather than held across the calls.
                    int node_index = static_cast<int>(this->tree.nodes.size());
                    this->tree.nodes.push_back(BspTree::Node{});
                    ++this->stats.nodes;

                    ConvexVolume front_volume = volume.split(split, PlaneSide::FRONT).simplified();
                    ConvexVolume back_volume = volume.split(split, PlaneSide::BACK).simplified();

                    std::vector<int> front_brushes;
                    std::vector<int> back_brushes;
                    this->partition_brushes(brush_list, split, front_volume, back_volume, front_brushes, back_brushes);

                    int front_child = this->build(front_volume, front_brushes, depth + 1);
                    int back_child = this->build(back_volume, back_brushes, depth + 1);

                    BspTree::Node& node = this->tree.nodes[static_cast<size_t>(node_index)];
                    node.plane = split;
                    node.plane_index = this->planes.add(split);
                    node.children[0] = front_child;
                    node.children[1] = back_child;
                    node.bounds = Bounds::from_points(corners);

                    return node_index;
                }

            private:
                void partition_brushes(const std::vector<int>& brush_list, const Plane& split,
                    const ConvexVolume& front_volume, const ConvexVolume& back_volume,
                    std::vector<int>& front_brushes, std::vector<int>& back_brushes)
                {
                    BoundingBox front_bounds = front_volume.bounds();
                    BoundingBox back_bounds = back_volume.bounds();

                    for (int brush_index : brush_list) {
                        const CompileBrush& brush = this->brushes[static_cast<size_t>(brush_index)];
                        PlaneSide where = classify_brush(brush, split);

                        // A brush is behind its own sides, so the half-space in front of one
                        // of them is outside that brush - which is exactly how a brush stops
                        // being carried into cells it does not occupy.
                        bool goes_front = where == PlaneSide::FRONT || where == PlaneSide::CROSS;
                        bool goes_back = where == PlaneSide::BACK || where == PlaneSide::CROSS || where == PlaneSide::ON;

                        // Reaching into a half-space is not the same as reaching into the
                        // cell carved out of it, so the cell's own extent gets a say too.
                        // Dropping brushes here keeps the lists from growing stale as they
                        // descend, which is most of what makes the recursion cheap.
                        if (goes_front && Bounds::overlaps(brush.bounds, front_bounds, ON_EPSILON)) {
                            front_brushes.push_back(brush_index);
                        }

                        if (goes_back && Bounds::overlaps(brush.bounds, back_bounds, ON_EPSILON)) {
                            back_brushes.push_back(brush_index);
                        }
                    }
                }

                bool choose_split(const std::vector<Vector3>& corners, const std::vector<int>& brush_list, Plane& out)
                {
                    // Gather the distinct planes on offer. Neighbouring brushes share a
                    // great many of them, and evaluating the same plane twenty times over
                    // is the difference between a compile that takes a moment and one that
                    // does not finish.
                    std::vector<std::pair<PlaneKey, Plane>> candidates;

                    for (int brush_index : brush_list) {
                        for (const CompileBrushSide& side : this->brushes[static_cast<size_t>(brush_index)].sides) {
                            // Bevels are generated from a brush's bounds purely so box
                            // sweeps behave; they describe no surface and splitting space
                            // along them would only deepen the tree.
                            if (side.is_bevel) {
                                continue;
                            }

                            candidates.push_back({ plane_key(side.plane), side.plane });
                        }
                    }

                    std::sort(candidates.begin(), candidates.end(),
                        [](const std::pair<PlaneKey, Plane>& a, const std::pair<PlaneKey, Plane>& b) {
                            return a.first < b.first;
                        });

                    candidates.erase(std::unique(candidates.begin(), candidates.end(),
                        [](const std::pair<PlaneKey, Plane>& a, const std::pair<PlaneKey, Plane>& b) {
                            return a.first == b.first;
                        }), candidates.end());

                    int best_score = std::numeric_limits<int>::max();
                    bool found = false;

                    for (const std::pair<PlaneKey, Plane>& candidate : candidates) {
                        const Plane& plane = candidate.second;

                        // A plane the cell lies wholly on one side of divides nothing, and
                        // choosing it would recurse forever on an unchanged cell.
                        bool has_front = false;
                        bool has_back = false;

                        for (Vector3 corner : corners) {
                            PlaneSide where = plane.classify_point(corner);

                            if (where == PlaneSide::FRONT) {
                                has_front = true;
                            } else if (where == PlaneSide::BACK) {
                                has_back = true;
                            }
                        }

                        if (!has_front || !has_back) {
                            continue;
                        }

                        int front = 0;
                        int back = 0;
                        int straddling = 0;

                        for (int brush_index : brush_list) {
                            switch (classify_brush(this->brushes[static_cast<size_t>(brush_index)], plane)) {
                                case PlaneSide::CROSS: ++straddling; break;
                                case PlaneSide::FRONT: ++front; break;
                                default: ++back; break;
                            }
                        }

                        // Cutting a brush in two costs work twice over - more nodes now and
                        // more faces later - so it is weighted heavily against, while an
                        // even split is worth a little. Axial planes are preferred because
                        // they make for cheap tests and tidy cells.
                        int score = std::abs(front - back) + straddling * 5;

                        if (plane.type() != PlaneType::NON_AXIAL) {
                            score -= 5;
                        }

                        if (score < best_score) {
                            best_score = score;
                            out = plane;
                            found = true;
                        }
                    }

                    return found;
                }

                int make_solid_leaf()
                {
                    BspTree::Leaf leaf;
                    leaf.contents = CONTENTS_SOLID;
                    leaf.bounds = Bounds::empty();

                    this->tree.leaves.push_back(std::move(leaf));
                    ++this->stats.leaves;
                    ++this->stats.solid_leaves;

                    return CompiledMap::leaf_to_child(static_cast<int>(this->tree.leaves.size()) - 1);
                }

                int make_leaf(const ConvexVolume& volume, const std::vector<int>& brush_list,
                    const std::vector<Vector3>& corners)
                {
                    BspTree::Leaf leaf;
                    leaf.volume = volume;
                    leaf.bounds = Bounds::from_points(corners);
                    leaf.brushes = brush_list;

                    // No brush plane divides this cell any more, so every brush either fills
                    // it completely or misses it completely - which is what makes one point
                    // enough to settle what the whole cell is made of.
                    Vector3 interior = volume.interior_point();

                    for (int brush_index : brush_list) {
                        if (this->brushes[static_cast<size_t>(brush_index)].contains(interior, -ON_EPSILON)) {
                            leaf.contents |= this->brushes[static_cast<size_t>(brush_index)].contents;
                        }
                    }

                    bool solid = (leaf.contents & CONTENTS_SOLID_MASK) != 0u;

                    this->tree.leaves.push_back(std::move(leaf));
                    ++this->stats.leaves;

                    if (solid) {
                        ++this->stats.solid_leaves;
                    }

                    return CompiledMap::leaf_to_child(static_cast<int>(this->tree.leaves.size()) - 1);
                }
        };
    }

    BspTree BspBuilder::build(const std::vector<CompileBrush>& brushes,
        const std::vector<int>& brush_indices, PlaneSet& planes,
        const Options& options, Stats& stats)
    {
        BoundingBox world = Bounds::empty();

        for (int brush_index : brush_indices) {
            world = Bounds::merge(world, brushes[static_cast<size_t>(brush_index)].bounds);
        }

        if (Bounds::is_empty(world)) {
            world = BoundingBox{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
        }

        // The root cell is padded out past the brushwork so that the outermost brush faces
        // have open space in front of them to face into. Without the margin the world's
        // outer skin would sit flush against the root cell's own boundary, and the flood
        // fill would have nowhere to recognise as outside.
        world = Bounds::expanded(world, 64.0f);

        Builder builder(brushes, planes, options, stats);
        builder.tree.bounds = world;
        builder.tree.root = builder.build(ConvexVolume::from_bounds(world), brush_indices, 0);

        return std::move(builder.tree);
    }
}
