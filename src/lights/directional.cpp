#include <lightwave.hpp>

namespace lightwave {

class DirectionalLight final : public Light {
public:
    DirectionalLight(const Properties &properties) {
        m_direction = properties.get<Vector>("direction");
        m_intensity = properties.get<Color>("intensity");
    }

    DirectLightSample sampleDirect(const Point &origin, Sampler &rng) const override {
        DirectLightSample dls;
        dls.wi = m_direction.normalized();
        dls.distance = Infinity;
        dls.weight = m_intensity;
        return dls;
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format("DirectionalLight[]");
    }
private:
    Vector m_direction;
    Color  m_intensity;
};

} // namespace lightwave

REGISTER_LIGHT(DirectionalLight, "directional")
