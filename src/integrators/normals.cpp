#include <lightwave.hpp>

namespace lightwave {
    class NormalsIntegrator : public SamplingIntegrator {
    public:
        NormalsIntegrator(const Properties &properties) : SamplingIntegrator(properties) {
            m_remap = properties.get<bool>("remap", true);
        }

        Color Li(const Ray &ray, Sampler &rng) override {
            Intersection its = m_scene->intersect(ray, rng);
            Vector normal = its ? its.frame.normal : Vector(0.f);
            return m_remap ? Color((normal + Vector(1.f)) * 0.5f) : Color(normal);
        }

        std::string toString() const override {
            return tfm::format(
                    "NormalsIntegrator[\n"
                    "  sampler = %s,\n"
                    "  image = %s,\n"
                    "]",
                    indent(m_sampler),
                    indent(m_image)
            );
        }

    private:
        bool m_remap;
    };
}

REGISTER_INTEGRATOR(NormalsIntegrator, "normals");