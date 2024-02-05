#include <lightwave.hpp>

namespace lightwave {

    class PrincipledVolume : public Bsdf {
        float m_absorption;
        float m_phase;
        Color m_color;

    public:
        PrincipledVolume(const Properties &properties) {
            m_color = properties.get<Color>("color", Color(1));
            m_absorption = properties.get<float>("absorption");
            m_phase = properties.get<float>("phase", 0);
        }

        BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
            const float phasePdf = phase(m_phase, (-wo).dot(wi));
            return { m_color * phasePdf * m_absorption };
        }

        BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
            const Vector up = Vector(0, 0, 1); // because normal is random
            const float phasePdf = phase(m_phase, (-wo).dot(up));
            return BsdfSample(up, m_color * phasePdf * m_absorption);
        }

        Color albedo(const Point2 &uv) const override {
            return m_color;
        }

        float phase(float g, float cos_theta) const
        {
            float denom = 1 + g * g - 2 * g * cos_theta;
            return 1 / (4 * Pi) * (1 - g * g) / (denom * sqrtf(denom));
        }

        std::string toString() const override {
            return tfm::format(
                    "PrincipledVolume[\n"
                    "  absorption = %s,\n"
                    "]",
                    indent(m_absorption)
            );
        }
    };

}

REGISTER_BSDF(PrincipledVolume, "volume")
