#include <lightwave.hpp>

namespace lightwave {

/*
    <light type="area">
        <instance>
            <shape type="sphere"/>
            <transform>
                <scale value="0.1"/>
                <translate x="4.08" y="1.01" z="5.9"/>
            </transform>
            <emission type="lambertian">
                <texture name="emission" type="constant" value="2.53e+03,2.53e+03,2.53e+03"/>
            </emission>
        </instance>
    </light>
*/

class AreaLight final : public Light {
public:
    AreaLight(const Properties &properties) {
        m_instance = properties.getChild<Instance>();
    }

    DirectLightSample sampleDirect(const Point &origin, Sampler &rng) const override {
        const AreaSample sample = m_instance->sampleArea(rng);
        auto [length, wi] = (sample.position - origin).lengthAndNormalized();

        if (sample.pdf == 0.f || length == 0.f) return DirectLightSample::invalid();

        const Vector localWi = sample.frame.toLocal(wi).normalized();

        const float cosTheta = Frame::absCosTheta(localWi);
        const Color emission = m_instance->emission()->evaluate(sample.uv, -localWi).value;

        return {wi, (emission * cosTheta) / (sqr(length) * sample.pdf), length};
    }

    bool canBeIntersected() const override { return m_instance->isVisible(); }

    std::string toString() const override {
        return tfm::format("AreaLight[]");
    }
private:
    ref<Instance> m_instance;
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "area")
 