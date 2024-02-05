#include <lightwave.hpp>

namespace lightwave {

class BVHPerformance : public SamplingIntegrator {
    float m_unit;

public:
    BVHPerformance(const Properties &properties)
        : SamplingIntegrator(properties) {
        m_unit = properties.get<float>("unit", 1);
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        return Color(its.stats.bvhCounter / m_unit,
                     its.stats.primCounter / m_unit, 0);
    }

    std::string toString() const override {
        return tfm::format("BVHPerformance[\n"
                           "  sampler = %s,\n"
                           "  image = %s,\n"
                           "]",
                           indent(m_sampler), indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(BVHPerformance, "bvh")
