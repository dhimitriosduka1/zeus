#include <lightwave.hpp>
#include "utils.hpp"

namespace lightwave {
    class PathTracerIntegrator : public SamplingIntegrator {
    public:
        PathTracerIntegrator(const Properties &properties) : SamplingIntegrator(properties) {
            m_depth = properties.get<int>("depth", 2);
            m_useNEE = properties.get<bool>("nee", true);
        }

        Color Li(const Ray &ray, Sampler &rng) override {
            Ray currentRay = ray;
            Color weight = Color(1.0f);
            Color Li = Color(0.f);
            bool useLights = m_scene->hasLights();

            for (int i = 0; i < m_depth; i++) {
                Intersection its = m_scene->intersect(currentRay, rng);

                if (!its) {
                    Li += m_scene->evaluateBackground(currentRay.direction).value * weight;
                    return Li;
                }

                Li += its.evaluateEmission() * weight;

                // Exit early when max depth is reached
                if (i == m_depth - 1) {
                    return Li;
                }
                
                if (m_useNEE && useLights) Li += lightwave::integrators::resolveNEEContribution(m_scene, rng, its, weight);

                BsdfSample sample = its.sampleBsdf(rng);
                weight *= sample.weight;
                
                // Exit if sample is invalid (usually just black)
                if (sample.isInvalid()) {
                    return Li;
                }

                currentRay = Ray(its.position, sample.wi.normalized());
            }

            return Li;
        }

        std::string toString() const override {
            return tfm::format(
                "PathTracerIntegrator[\n"
                "  sampler = %s,\n"
                "  image = %s,\n"
                "  depth = %d,\n"
                "]",
                indent(m_sampler),
                indent(m_image),
                indent(m_depth)
            );
        }

    private:
        int m_depth;
        bool m_useNEE;
    };
}

REGISTER_INTEGRATOR(PathTracerIntegrator, "pathtracer");