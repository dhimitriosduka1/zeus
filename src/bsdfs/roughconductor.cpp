#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughConductor : public Bsdf {
    ref<Texture> m_reflectance;
    ref<Texture> m_roughness;

public:
    RoughConductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
        m_roughness   = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
        // Using the squared roughness parameter results in a more gradual
        // transition from specular to rough. For numerical stability, we avoid
        // extremely specular distributions (alpha values below 10^-3)
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const Vector wh = (wi + wo).normalized();

        const float pdf_GGX = microfacet::evaluateGGX(alpha, wh);
        const float pdf_G_i = microfacet::smithG1(alpha, wh, wi);
        const float pdf_G_o = microfacet::smithG1(alpha, wh, wo);

        const float factor = (pdf_GGX * pdf_G_i * pdf_G_o) / (4 * Frame::cosTheta(wo));

        return { m_reflectance->evaluate(uv) * factor };
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        Vector wh = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        Vector reflected = reflect(wo, wh);

        return { reflected, m_reflectance->evaluate(uv) * microfacet::smithG1(alpha, wh, reflected) };
    }

    Color albedo(const Point2 &uv) const override {
        return 0.5f * (m_reflectance->evaluate(uv) + m_roughness->evaluate(uv));
    }

    std::string toString() const override {
        return tfm::format("RoughConductor[\n"
                           "  reflectance = %s,\n"
                           "  roughness = %s\n"
                           "]",
                           indent(m_reflectance), indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughConductor, "roughconductor")
