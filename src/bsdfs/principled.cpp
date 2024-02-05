#include <lightwave.hpp>

#include "fresnel.hpp"
#include "microfacet.hpp"

namespace lightwave {

struct DiffuseLobe {
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        return { color * max(0, Frame::cosTheta(wi)) * InvPi };

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        const Vector sample = squareToCosineHemisphere(rng.next2D());
        return { sample.normalized(), color };
    }
};

struct MetallicLobe {
    float alpha;
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        if (!Frame::sameHemisphere(wi, wo)) {
            return BsdfEval::invalid();
        }
        const Vector wh = (wi + wo).normalized();

        const float pdf_GGX = microfacet::evaluateGGX(alpha, wh);
        const float pdf_G_i = microfacet::smithG1(alpha, wh, wi);
        const float pdf_G_o = microfacet::smithG1(alpha, wh, wo);

        const float factor = (pdf_GGX * pdf_G_i * pdf_G_o) / (4 * Frame::cosTheta(wo));

        return { color * factor };
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        Vector wh = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        Vector reflected = reflect(wo, wh);

        return { reflected, color * microfacet::smithG1(alpha, wh, reflected) };
    }
};

class Principled : public Bsdf {
    ref<Texture> m_baseColor;
    ref<Texture> m_roughness;
    ref<Texture> m_metallic;
    ref<Texture> m_specular;
    ref<Texture> m_transparency;

    struct Combination {
        float diffuseSelectionProb;
        DiffuseLobe diffuse;
        MetallicLobe metallic;
    };

    Combination combine(const Point2 &uv, const Vector &wo) const {
        const auto baseColor = m_baseColor->evaluate(uv);
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const auto specular = m_specular->scalar(uv);
        const auto metallic = m_metallic->scalar(uv);
        const auto F =
            specular * schlick((1 - metallic) * 0.08f, Frame::cosTheta(wo));

        const DiffuseLobe diffuseLobe = {
            .color = (1 - F) * (1 - metallic) * baseColor,
        };
        const MetallicLobe metallicLobe = {
            .alpha = alpha,
            .color = F * Color(1) + (1 - F) * metallic * baseColor,
        };

        const auto diffuseAlbedo = diffuseLobe.color.mean();
        const auto totalAlbedo =
            diffuseLobe.color.mean() + metallicLobe.color.mean();
        return {
            .diffuseSelectionProb =
                totalAlbedo > 0 ? diffuseAlbedo / totalAlbedo : 1.0f,
            .diffuse  = diffuseLobe,
            .metallic = metallicLobe,
        };
    }

public:
    Principled(const Properties &properties) {
        m_baseColor = properties.get<Texture>("baseColor");
        m_roughness = properties.get<Texture>("roughness");
        m_metallic  = properties.get<Texture>("metallic");
        m_specular  = properties.get<Texture>("specular");
        m_transparency  = properties.get<Texture>("transparency", nullptr);
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
        const auto combination = combine(uv, wo);
        return { combination.diffuse.evaluate(wo, wi).value + combination.metallic.evaluate(wo, wi).value };
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
        if (m_transparency) {
            float chance = m_transparency->scalar(uv);
            if (rng.next() > chance) {
                return { -wo, Color(1.0f) };
            }
        }
        const auto combination = combine(uv, wo);
        const float diffuseSelectionProb = combination.diffuseSelectionProb;

        if (rng.next() < diffuseSelectionProb) {
            // Sample diffuse
            auto sample = combination.diffuse.sample(wo, rng);
            return { sample.wi, sample.weight / diffuseSelectionProb };
        } else {
            // Sample metallic
            auto sample = combination.metallic.sample(wo, rng);
            return { sample.wi, sample.weight / (1.f - diffuseSelectionProb) };
        }
    }

    Color albedo(const Point2 &uv) const override {
        return m_baseColor->evaluate(uv);
    }

    std::string toString() const override {
        return tfm::format("Principled[\n"
                           "  baseColor = %s,\n"
                           "  roughness = %s,\n"
                           "  metallic  = %s,\n"
                           "  specular  = %s,\n"
                           "]",
                           indent(m_baseColor), indent(m_roughness),
                           indent(m_metallic), indent(m_specular));
    }
};

} // namespace lightwave

REGISTER_BSDF(Principled, "principled")
