#include <IronHull/map/compile/PortalFlood.hpp>

#include <deque>

#include <raymath.h>

#include <IronHull/geometry/Bounds.hpp>
#include <IronHull/map/CompiledMap.hpp>

namespace IronHull
{
    namespace
    {
        bool is_open(const BspTree::Leaf& leaf)
        {
            return (leaf.contents & CONTENTS_SOLID_MASK) == 0u;
        }

        // Whether two open cells share a surface, and so whether the flood can pass between
        // them.
        //
        // Clipping one cell's boundary polygons against the other cell's half-spaces finds
        // the overlap directly. BSP cells never overlap, so the only part of one cell's
        // boundary that can lie inside the other's closure is the surface they share - if
        // anything survives the clip with area to it, that surface is the portal.
        bool shares_portal(const std::vector<Winding>& faces, const ConvexVolume& other)
        {
            for (const Winding& face : faces) {
                if (!face.is_valid()) {
                    continue;
                }

                Winding portal = face;

                for (const Plane& plane : other.planes) {
                    if (portal.points.size() < 3) {
                        break;
                    }

                    portal = portal.clipped(plane, PlaneSide::BACK);
                }

                portal.simplify();

                if (portal.is_valid()) {
                    return true;
                }
            }

            return false;
        }

        // A cell pressed up against the edge of the compiler's world box. The box is padded
        // out well past the brushwork, so the only way the flood reaches one of these is by
        // escaping the map through a hole.
        bool touches_world_edge(const BspTree::Leaf& leaf, const BoundingBox& world)
        {
            constexpr float edge_epsilon = 1.0f;

            return leaf.bounds.min.x <= world.min.x + edge_epsilon
                || leaf.bounds.min.y <= world.min.y + edge_epsilon
                || leaf.bounds.min.z <= world.min.z + edge_epsilon
                || leaf.bounds.max.x >= world.max.x - edge_epsilon
                || leaf.bounds.max.y >= world.max.y - edge_epsilon
                || leaf.bounds.max.z >= world.max.z - edge_epsilon;
        }
    }

    int PortalFlood::find_leaf(const BspTree& tree, Vector3 point)
    {
        int child = tree.root;

        while (!CompiledMap::child_is_leaf(child)) {
            if (child >= static_cast<int>(tree.nodes.size())) {
                return -1;
            }

            const BspTree::Node& node = tree.nodes[static_cast<size_t>(child)];
            child = node.plane.distance_to(point) >= 0.0f ? node.children[0] : node.children[1];
        }

        int leaf = CompiledMap::child_to_leaf(child);
        return leaf < static_cast<int>(tree.leaves.size()) ? leaf : -1;
    }

    std::vector<std::vector<int>> PortalFlood::build_adjacency(const BspTree& tree)
    {
        size_t count = tree.leaves.size();
        std::vector<std::vector<int>> adjacency(count);

        // Each cell's boundary polygons are worked out once up front. Deriving them is the
        // expensive part of the test below, and every cell is compared against many others.
        std::vector<std::vector<Winding>> boundaries(count);

        for (size_t index = 0; index < count; ++index) {
            if (is_open(tree.leaves[index])) {
                boundaries[index] = tree.leaves[index].volume.faces();
            }
        }

        for (size_t a = 0; a < count; ++a) {
            if (!is_open(tree.leaves[a])) {
                continue;
            }

            for (size_t b = a + 1; b < count; ++b) {
                if (!is_open(tree.leaves[b])) {
                    continue;
                }

                // Cells whose boxes do not even touch cannot share a surface, and this
                // throws out almost every pair for the price of six comparisons.
                if (!Bounds::overlaps(tree.leaves[a].bounds, tree.leaves[b].bounds, ON_EPSILON)) {
                    continue;
                }

                if (!shares_portal(boundaries[a], tree.leaves[b].volume)) {
                    continue;
                }

                adjacency[a].push_back(static_cast<int>(b));
                adjacency[b].push_back(static_cast<int>(a));
            }
        }

        return adjacency;
    }

    PortalFlood::Result PortalFlood::run(BspTree& tree, const std::vector<Vector3>& seeds,
        const std::vector<std::string>& seed_names)
    {
        Result result;

        for (BspTree::Leaf& leaf : tree.leaves) {
            leaf.area = -1;
        }

        if (tree.leaves.empty()) {
            return result;
        }

        std::vector<std::vector<int>> adjacency = PortalFlood::build_adjacency(tree);

        for (size_t seed_index = 0; seed_index < seeds.size(); ++seed_index) {
            int start = PortalFlood::find_leaf(tree, seeds[seed_index]);

            if (start < 0 || !is_open(tree.leaves[static_cast<size_t>(start)])) {
                continue;
            }

            // Already reached from an earlier seed, so it adds no new region.
            if (tree.leaves[static_cast<size_t>(start)].area >= 0) {
                continue;
            }

            int area = result.area_count;
            ++result.area_count;

            std::deque<int> pending;
            pending.push_back(start);
            tree.leaves[static_cast<size_t>(start)].area = area;

            while (!pending.empty()) {
                int current = pending.front();
                pending.pop_front();

                if (!result.leaked && touches_world_edge(tree.leaves[static_cast<size_t>(current)], tree.bounds)) {
                    result.leaked = true;
                    result.leak_point = seeds[seed_index];
                    result.leak_entity = seed_index < seed_names.size() ? seed_names[seed_index] : std::string();
                }

                for (int neighbour : adjacency[static_cast<size_t>(current)]) {
                    if (tree.leaves[static_cast<size_t>(neighbour)].area >= 0) {
                        continue;
                    }

                    tree.leaves[static_cast<size_t>(neighbour)].area = area;
                    pending.push_back(neighbour);
                }
            }
        }

        for (const BspTree::Leaf& leaf : tree.leaves) {
            if (!is_open(leaf)) {
                continue;
            }

            if (leaf.area >= 0) {
                ++result.reachable_leaves;
            } else {
                ++result.sealed_leaves;
            }
        }

        return result;
    }

    void PortalFlood::assign_faces(BspTree& tree, std::vector<CompileFace>& faces, bool cull_outside)
    {
        for (BspTree::Leaf& leaf : tree.leaves) {
            leaf.faces.clear();
        }

        for (size_t index = 0; index < faces.size(); ++index) {
            CompileFace& face = faces[index];

            if (!face.winding.is_valid()) {
                face.visible = false;
                continue;
            }

            // A face is seen from the space in front of it, so a point nudged just off its
            // middle along the normal lands in the cell that can see it. The nudge is kept
            // small so it cannot overshoot a narrow gap into the solid on the far side.
            Vector3 sample = Vector3Add(face.winding.centroid(), Vector3Scale(face.plane.normal, ON_EPSILON * 2.0f));

            int leaf_index = PortalFlood::find_leaf(tree, sample);
            if (leaf_index < 0) {
                face.visible = false;
                continue;
            }

            BspTree::Leaf& leaf = tree.leaves[static_cast<size_t>(leaf_index)];

            // Facing into solid means something else is already covering this surface, so
            // it can go whether or not the flood ran.
            if (!is_open(leaf)) {
                face.visible = false;
                continue;
            }

            if (cull_outside && leaf.area < 0) {
                face.visible = false;
                continue;
            }

            leaf.faces.push_back(static_cast<int>(index));
        }
    }
}
