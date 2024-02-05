#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting or refracting `wo' is zero, hence we can
        // just ignore that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
        const float ior = m_ior->scalar(uv);
        const float cos = Frame::cosTheta(wo);
        const bool isNegative = std::signbit(cos);
        const float eta = isNegative ? 1 / ior : ior;

        // Not sure about using cos of wo here. Tests are passing though :)
        const float fresnel = fresnelDielectric(cos, eta);
        if (rng.next() < fresnel) {
            // Reflection
            return {reflect(wo, Vector(0, 0, 1)), m_reflectance->evaluate(uv)};
        }
        // Refraction
        const Vector normal = Vector(0, 0, isNegative ? -1 : 1);
        return { refract(wo, normal, eta), m_transmittance->evaluate(uv) / sqr(eta) };
    }

    Color albedo(const Point2 &uv) const override {
        return 0.5f * (m_transmittance->evaluate(uv) + m_reflectance->evaluate(uv));
    }

    std::string toString() const override {
        return tfm::format("Dielectric[\n"
                           "  ior           = %s,\n"
                           "  reflectance   = %s,\n"
                           "  transmittance = %s\n"
                           "]",
                           indent(m_ior), indent(m_reflectance),
                           indent(m_transmittance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Dielectric, "dielectric")