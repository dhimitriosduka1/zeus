#include <lightwave.hpp>

namespace lightwave {
    class DebugIntegrator : public SamplingIntegrator {
    public:
        DebugIntegrator(const Properties &properties) : SamplingIntegrator(properties) {
            m_remap = properties.get<bool>("remap", true);
            m_mode = properties.getEnum<DisplayMode>("mode", DisplayMode::Normal,
                                           {
                                               { "normal", DisplayMode::Normal },
                                               { "tangent", DisplayMode::Normal },
                                               { "bitangent", DisplayMode::Normal },
                                               { "distance", DisplayMode::Normal },
                                               { "uv", DisplayMode::Uv },
                                               { "color", DisplayMode::Color }
                                           });
        }

        Color Li(const Ray &ray, Sampler &rng) override {
            Intersection its = m_scene->intersect(ray, rng);
            
            Vector value;
            switch (m_mode)
            {
                case DisplayMode::Normal:
                    value = its.frame.normal;
                    break;
                case DisplayMode::Tangent:
                    value = its.frame.tangent;
                    break;
                case DisplayMode::Bitangent:
                    value = its.frame.bitangent;
                    break;
                case DisplayMode::Distance:
                    value = Vector(its.t);
                    break;
                case DisplayMode::Uv:
                    value = Vector(its.uv.x(), its.uv.y(), 0.0f);
                    break;
                case DisplayMode::Color:
                    const BsdfSample sample = its.sampleBsdf(rng);
                    // Color col = its.sampleBsdf(rng).weight;
                    // value = Vector(0.0f);//Vector(col.r(), col.g(), col.b());
                    break;
            }

            return m_remap ? Color((value + Vector(1.f)) * 0.5f) : Color(value);
        }

        std::string toString() const override {
            return tfm::format(
                    "DebugIntegrator[\n"
                    "  sampler = %s,\n"
                    "  image = %s,\n"
                    "]",
                    indent(m_sampler),
                    indent(m_image)
            );
        }

    private:
        enum class DisplayMode {
            Normal,
            Tangent,
            Bitangent,
            Distance,
            Uv,
            Color
        };

        DisplayMode m_mode;
        bool m_remap;
    };
}

REGISTER_INTEGRATOR(DebugIntegrator, "debug");