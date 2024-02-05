#include <lightwave.hpp>
#include "utils.hpp"

namespace lightwave {
    class DirectIntegrator : public SamplingIntegrator {
    public:
        DirectIntegrator(const Properties &properties) : SamplingIntegrator(properties) {
        }

        Color Li(const Ray &ray, Sampler &rng) override {
            const int DEPTH = 2;

            Ray currentRay = ray;
            Color weight = Color(1.0f);
            Color Li = Color(0.f);
            bool useLights = m_scene->hasLights();

            for (int i = 0; i < DEPTH; i++) {
                const Intersection its = m_scene->intersect(currentRay, rng);

                // Hit nothing, early exit
                if (!its) {
                    Li += m_scene->evaluateBackground(currentRay.direction).value * weight;
                    return Li;
                }

                Li += its.evaluateEmission() * weight;
                // Return early for direct integrator
                if (i == DEPTH - 1) return Li;

                if (useLights) Li += lightwave::integrators::resolveNEEContribution(m_scene, rng, its, weight);
                    
                BsdfSample sample = its.sampleBsdf(rng);
                weight *= sample.weight;

                currentRay = Ray(its.position, sample.wi.normalized());
            }

            return Li;
        }

        std::string toString() const override {
            return tfm::format(
                "DirectIntegrator[\n"
                "  sampler = %s,\n"
                "  image = %s,\n"
                "]",
                indent(m_sampler),
                indent(m_image)
            );
        }
    };
}

REGISTER_INTEGRATOR(DirectIntegrator, "direct");