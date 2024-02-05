#include <lightwave.hpp>

namespace lightwave {
    class AlbedoIntegrator : public SamplingIntegrator {
    public:
        AlbedoIntegrator(const Properties &properties) : SamplingIntegrator(properties) {}

        Color Li(const Ray &ray, Sampler &rng) override {
            Intersection its = m_scene->intersect(ray, rng);

            // Check if we hit an object and that the object is a bsdf
            if (!its || !its.instance->bsdf()) return Color(0.f);

            // Return albedo evaluated at uv
            return its.instance->bsdf()->albedo(its.uv);
        }

        std::string toString() const override {
            return tfm::format(
                    "AlbedoIntegrator[\n"
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

REGISTER_INTEGRATOR(AlbedoIntegrator, "albedo");