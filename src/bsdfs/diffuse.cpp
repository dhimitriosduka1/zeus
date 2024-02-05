#include <lightwave.hpp>

namespace lightwave {

    class Diffuse : public Bsdf {
        ref<Texture> m_albedo;

    public:
        Diffuse(const Properties &properties) {
            m_albedo = properties.get<Texture>("albedo");
        }

        BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
            return { m_albedo->evaluate(uv) * max(0, Frame::cosTheta(wi)) * InvPi };
        }

        BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
            const Vector sample = squareToCosineHemisphere(rng.next2D());
            return BsdfSample(sample.normalized(), m_albedo->evaluate(uv));
        }

        Color albedo(const Point2 &uv) const override {
            return m_albedo->evaluate(uv);
        }

        std::string toString() const override {
            return tfm::format(
                    "Diffuse[\n"
                    "  albedo = %s,\n"
                    "]",
                    indent(m_albedo)
            );
        }
    };

}

REGISTER_BSDF(Diffuse, "diffuse")
