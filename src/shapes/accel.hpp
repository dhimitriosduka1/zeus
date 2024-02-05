#pragma once

#include <lightwave/core.hpp>
#include <lightwave/math.hpp>
#include <lightwave/shape.hpp>

#include <numeric>

namespace lightwave {

/**
 * @brief Parent class for shapes that combine many individual shapes (e.g.,
 * triangle meshes), and hence benefit from building an acceleration structure
 * over their children.
 *
 * To use this class, you will need to implement the following methods:
 * - numberOfPrimitives()           -- report the number of individual children
 * that the shape has
 * - intersect(primitiveIndex, ...) -- intersect a single child (identified by
 * the given index) for the given ray
 * - getBoundingBox(primitiveIndex) -- return the bounding box of a single child
 * (used for building the BVH)
 * - getCentroid(primitiveIndex)    -- return the centroid of a single child
 * (used for building the BVH)
 *
 * @example For a simple example of how to use this class, look at @ref
 * shapes/group.cpp
 * @see Group
 * @see TriangleMesh
 */
class AccelerationStructure : public Shape {
    /// @brief The datatype used to index BVH nodes and the primitive index
    /// remapping.
    typedef int32_t NodeIndex;

    /// @brief A node in our binary BVH tree.
    struct Node {
        /// @brief The axis aligned bounding box of this node.
        Bounds aabb;
        /**
         * @brief Either the index of the left child node in m_nodes (for
         * internal nodes), or the first primitive in m_primitiveIndices (for
         * leaf nodes).
         * @note For efficiency, we store the BVH nodes so that the right child
         * always directly follows the left child, i.e., the index of the right
         * child is always @code leftFirst + 1 @endcode .
         * @note For efficiency, we store primitives so that children of a leaf
         * node are always contigous in m_primitiveIndices.
         */
        NodeIndex leftFirst;
        /// @brief The number of primitives in a leaf node, or 0 to indicate
        /// that this node is not a leaf node.
        NodeIndex primitiveCount;

        /// @brief Whether this BVH node is a leaf node.
        bool isLeaf() const { return primitiveCount != 0; }

        /// @brief For internal nodes: The index of the left child node in
        /// m_nodes.
        NodeIndex leftChildIndex() const { return leftFirst; }
        /// @brief For internal nodes: The index of the right child node in
        /// m_nodes.
        NodeIndex rightChildIndex() const { return leftFirst + 1; }

        /// @brief For leaf nodes: The first index in m_primitiveIndices.
        NodeIndex firstPrimitiveIndex() const { return leftFirst; }
        /// @brief For leaf nodes: The last index in m_primitiveIndices (still
        /// included).
        NodeIndex lastPrimitiveIndex() const {
            return leftFirst + primitiveCount - 1;
        }
    };

    struct Bin {
        Bounds bounds;
        int count = 0;
    };

    /// @brief A list of all BVH nodes.
    std::vector<Node> m_nodes;
    /**
     * @brief Mapping from internal @c NodeIndex to @c primitiveIndex as used by
     * all interface methods. For efficient storage, we assume that children of
     * BVH leaf nodes have contiguous indices, which would require re-ordering
     * the primitives. For simplicity, we instead perform this re-ordering on a
     * list of indices (which starts of as @code 0, 1, 2, ..., primitiveCount -
     * 1 @endcode ), which allows us to translate from re-ordered (contiguous)
     * indices to the indices the user of this class expects.
     */
    std::vector<int> m_primitiveIndices;

    /// @brief Returns the root BVH node.
    const Node &rootNode() const {
        // by convention, this is always the first element of m_nodes
        return m_nodes.front();
    }

    /**
     * @brief Intersects a BVH node, recursing into children (for internal
     * nodes), or intersecting all primitives (for leaf nodes).
     */
    bool intersectNode(const Node &node, const Ray &ray, Intersection &its,
                       Sampler &rng) const {
        // update the statistic tracking how many BVH nodes have been tested for
        // intersection
        its.stats.bvhCounter++;

        bool wasIntersected = false;
        if (node.isLeaf()) {
            for (NodeIndex i = 0; i < node.primitiveCount; i++) {
                // update the statistic tracking how many children have been
                // tested for intersection
                its.stats.primCounter++;
                // test the child for intersection
                wasIntersected |= intersect(
                    m_primitiveIndices[node.leftFirst + i], ray, its, rng);
            }
        } else { // internal node
            // test which bounding box is intersected first by the ray.
            // this allows us to traverse the children in the order they are
            // intersected in, which can help prune a lot of unnecessary
            // intersection tests.
            const auto leftT =
                intersectAABB(m_nodes[node.leftChildIndex()].aabb, ray);
            const auto rightT =
                intersectAABB(m_nodes[node.rightChildIndex()].aabb, ray);
            if (leftT < rightT) { // left child is hit first; test left child
                                  // first, then right child
                if (leftT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.leftChildIndex()], ray, its, rng);
                if (rightT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.rightChildIndex()], ray, its, rng);
            } else { // right child is hit first; test right child first, then
                     // left child
                if (rightT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.rightChildIndex()], ray, its, rng);
                if (leftT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.leftChildIndex()], ray, its, rng);
            }
        }
        return wasIntersected;
    }

    /// @brief Performs a slab test to intersect a bounding box with a ray,
    /// returning Infinity in case the ray misses.
    float intersectAABB(const Bounds &bounds, const Ray &ray) const {
        // but this only saves us ~1%, so let's not do it. intersect all axes at
        // once with the minimum slabs of the bounding box
        const auto t1 = (bounds.min() - ray.origin) / ray.direction;
        // intersect all axes at once with the maximum slabs of the bounding box
        const auto t2 = (bounds.max() - ray.origin) / ray.direction;

        // the elementwiseMin picks the near slab for each axis, of which we
        // then take the maximum
        const auto tNear = elementwiseMin(t1, t2).maxComponent();
        // the elementwiseMax picks the far slab for each axis, of which we then
        // take the minimum
        const auto tFar = elementwiseMax(t1, t2).minComponent();

        if (tFar < tNear)
            return Infinity; // the ray does not intersect the bounding box
        if (tFar < Epsilon)
            return Infinity; // the bounding box lies behind the ray origin

        return tNear; // return the first intersection with the bounding box
                      // (may also be negative!)
    }

    /// @brief Computes the axis aligned bounding box for a leaf BVH node
    void computeAABB(Node &node) {
        node.aabb = Bounds::empty();
        for (NodeIndex i = 0; i < node.primitiveCount; i++) {
            const Bounds childAABB =
                getBoundingBox(m_primitiveIndices[node.leftFirst + i]);
            node.aabb.extend(childAABB);
        }
    }

    /// @brief Computes the surface area of a bounding box.
    float surfaceArea(const Bounds &bounds) const {
        const auto size = bounds.diagonal();
        return 2 * (size.x() * size.y() + size.x() * size.z() +
                    size.y() * size.z());
    }

    std::tuple<float, float> calculateMinAndMaxBounds(Node &node, int splitAxis) const {
        float boundsMin = Infinity;
        float boundsMax = -Infinity;

        for (int i = 0; i < node.primitiveCount; i++) {
            const Point centroid = getCentroid(m_primitiveIndices[node.leftFirst + i]);
            boundsMin = std::min(boundsMin, centroid[splitAxis]);
            boundsMax = std::max(boundsMax, centroid[splitAxis]);
        }
        return {boundsMin, boundsMax};
    }

    std::vector<Bin> populateBins(Node &node, int splitAxis, float boundsMin, float scale, int BINS) const {
        std::vector<Bin> bins(BINS);
        for (int i = 0; i < node.primitiveCount; i++) {
            const int primitiveIndex = node.leftFirst + i;
            const Point centroid = getCentroid(m_primitiveIndices[primitiveIndex]);
            const Bounds bounds = getBoundingBox(m_primitiveIndices[primitiveIndex]);
            const int binIndex = std::min((int)((centroid[splitAxis] - boundsMin) * scale), BINS - 1);
            bins[binIndex].count++;
            bins[binIndex].bounds.extend(bounds);
        }
        return bins;
    }

    std::tuple<float, float> findBestSplitPlane(Node &node, int splitAxis) {
        float bestCost = Infinity;
        float bestPosition = 0;

        const auto [boundsMin, boundsMax] = calculateMinAndMaxBounds(node, splitAxis);

        // We return Infinity and 0 here
        if (boundsMin == boundsMax) return {bestCost, bestPosition};

        const int BINS = 16;
        float scale = BINS / (boundsMax - boundsMin);

        std::vector<Bin> bins = populateBins(node, splitAxis, boundsMin, scale, BINS);

        float leftArea[BINS - 1], rightArea[BINS - 1];

        int leftCount[BINS - 1], rightCount[BINS - 1];

        Bounds leftBox, rightBox;

        int leftSum = 0, rightSum = 0;
        for (int i = 0; i < BINS - 1; i++) {
            leftSum += bins[i].count;
            leftCount[i] = leftSum;
            leftBox.extend(bins[i].bounds);
            leftArea[i] = surfaceArea(leftBox);

            rightSum += bins[BINS - i - 1].count;
            rightCount[BINS - 2 - i] = rightSum;
            rightBox.extend(bins[BINS - i - 1].bounds);
            rightArea[BINS - i - 2] = surfaceArea(rightBox);
        }

        scale = (boundsMax - boundsMin) / BINS;
        for (int i = 0; i < BINS - 1; i++) {
            const float planeCost = (float) leftCount[i] * leftArea[i] + (float) rightCount[i] * rightArea[i];
            if (planeCost < bestCost) {
                bestCost = planeCost;
                bestPosition = boundsMin + scale * ((float) i + 1);
            }
        }
        return {bestCost, bestPosition};
    }

    float calculateNodeCost(Node &node) {
        return (float) node.primitiveCount * surfaceArea(node.aabb);
    }

    NodeIndex binning(Node &node, int splitAxis) {
        const auto [splitCost, splitPosition] = findBestSplitPlane(node, splitAxis);

        if (splitCost == Infinity) return -1;

        const float noSplitCost = calculateNodeCost(node);

        if (noSplitCost < splitCost) return -1;

        return partition(node, splitAxis, splitPosition);
    }

    NodeIndex partition(Node &node, int splitAxis, float splitPos) {
        NodeIndex firstRightIndex = node.firstPrimitiveIndex();
        NodeIndex lastLeftIndex = node.lastPrimitiveIndex();
        while (firstRightIndex <= lastLeftIndex) {
            if (getCentroid(m_primitiveIndices[firstRightIndex])[splitAxis] < splitPos) {
                firstRightIndex++;
            } else {
                std::swap(m_primitiveIndices[firstRightIndex], m_primitiveIndices[lastLeftIndex--]);
            }
        }
        return firstRightIndex;
    }

    /// @brief Attempts to subdivide a given BVH node.
    void subdivide(Node &parent) {
        // only subdivide if enough children are available.
        if (parent.primitiveCount <= 2) {
            return;
        }

        // pick the axis with the highest bounding box length as split axis.
        const int splitAxis = parent.aabb.diagonal().maxComponentIndex();
        const NodeIndex firstPrimitive = parent.firstPrimitiveIndex();

        // set to true when implementing binning
        static constexpr bool UseSAH = true;

        // the point at which to split (note that primitives must be re-ordered
        // so that all children of the left node will have a smaller index than
        // firstRightIndex, and nodes on the right will have an index larger or
        // equal to firstRightIndex)
        NodeIndex firstRightIndex;
        if (UseSAH) {
            firstRightIndex = binning(parent, splitAxis);

            // If we do not gain anything from splitting the node, we don't split it
            if (firstRightIndex == -1) return;
        } else {
            // split in the middle
            // pick center of bounding box as split pos
            const float splitPos = parent.aabb.center()[splitAxis];
            firstRightIndex = partition(parent, splitAxis, splitPos);
        }

        const NodeIndex leftCount =
            firstRightIndex - parent.firstPrimitiveIndex();
        const NodeIndex rightCount = parent.primitiveCount - leftCount;

        if (leftCount == 0 || rightCount == 0) {
            // if either child gets no primitives, we abort subdividing
            return;
        }

        // the two children will always be contiguous in our m_nodes list
        const auto leftChildIndex  = (NodeIndex) (m_nodes.size() + 0);
        const auto rightChildIndex = (NodeIndex) (m_nodes.size() + 1);
        parent.primitiveCount = 0; // mark the parent node as internal node
        parent.leftFirst      = leftChildIndex;

        // `parent' breaks
        m_nodes.emplace_back();
        m_nodes[leftChildIndex].leftFirst      = firstPrimitive;
        m_nodes[leftChildIndex].primitiveCount = leftCount;

        m_nodes.emplace_back();
        m_nodes[rightChildIndex].leftFirst      = firstRightIndex;
        m_nodes[rightChildIndex].primitiveCount = rightCount;

        // first, process the left child node (and all of its children)
        computeAABB(m_nodes[leftChildIndex]);
        subdivide(m_nodes[leftChildIndex]);
        // then, process the right child node (and all of its children)
        computeAABB(m_nodes[rightChildIndex]);
        subdivide(m_nodes[rightChildIndex]);
    }

protected:
    /// @brief Returns the number of children (individual shapes) that are part
    /// of this acceleration structure.
    virtual int numberOfPrimitives() const = 0;
    /// @brief Intersect a single child (identified by the index) with the given
    /// ray.
    virtual bool intersect(int primitiveIndex, const Ray &ray,
                           Intersection &its, Sampler &rng) const = 0;
    /// @brief Returns the axis aligned bounding box of the given child.
    virtual Bounds getBoundingBox(int primitiveIndex) const = 0;
    /// @brief Returns the centroid of the given child.
    virtual Point getCentroid(int primitiveIndex) const = 0;

    /// @brief Builds the acceleration structure.
    void buildAccelerationStructure() {
        Timer buildTimer;

        // fill primitive indices with 0 to primitiveCount - 1
        m_primitiveIndices.resize(numberOfPrimitives());
        std::iota(m_primitiveIndices.begin(), m_primitiveIndices.end(), 0);

        // create root node
        auto &root          = m_nodes.emplace_back();
        root.leftFirst      = 0;
        root.primitiveCount = numberOfPrimitives();
        computeAABB(root);
        subdivide(root);

        logger(EInfo, "built BVH with %ld nodes for %ld primitives in %.1f ms",
               m_nodes.size(), numberOfPrimitives(),
               buildTimer.getElapsedTime() * 1000);
    }

public:
    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        if (m_primitiveIndices.empty())
            return false; // exit early if no children exist
        if (intersectAABB(rootNode().aabb, ray) <
            its.t) // test root bounding box for potential hit
            return intersectNode(rootNode(), ray, its, rng);
        return false;
    }

    Bounds getBoundingBox() const override { return rootNode().aabb; }

    Point getCentroid() const override { return rootNode().aabb.center(); }
};

} // namespace lightwave
