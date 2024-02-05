#include <lightwave.hpp>

#include "accel.hpp"

namespace lightwave {

/**
 * @brief A group is a shape that results from the union of an arbitrary amount of individual shapes.
 * This allows us to avoid manually iterating over all objects in the scene whenever we need to find an intersection,
 * and also provides noticeable speed-up by using an acceleration structure under the hood.
 */
class Group final : public AccelerationStructure {
    std::vector<ref<Shape>> m_children;

protected:
    int numberOfPrimitives() const override {
        return int(m_children.size());
    }

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its, Sampler &rng) const override {
        return m_children[primitiveIndex]->intersect(ray, its, rng);
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        return m_children[primitiveIndex]->getBoundingBox();
    }

    Point getCentroid(int primitiveIndex) const override {
        return m_children[primitiveIndex]->getCentroid();
    }

public:
    Group(const Properties &properties) {
        m_children = properties.getChildren<Shape>();
        buildAccelerationStructure();
    }

    void markAsVisible() override {
        for (auto &child : m_children) child->markAsVisible();
    }

    AreaSample sampleArea(Sampler &rng) const override {
        int childIndex = int(rng.next() * m_children.size());
        childIndex = std::min(childIndex, int(m_children.size()) - 1);

        AreaSample sample = m_children[childIndex]->sampleArea(rng);
        sample.pdf /= m_children.size();
        return sample;
    }

    std::string toString() const override {
        std::stringstream oss;
        oss << "Group[" << std::endl;
        for (auto &entity : m_children) {
            oss << "  " << indent(entity) << "," << std::endl;
        }
        oss << "]";
        return oss.str();
    }
};

}

REGISTER_SHAPE(Group, "group")
