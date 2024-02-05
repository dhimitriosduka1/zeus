#include <lightwave.hpp>

namespace lightwave {

class SpotLight final : public Light {
    Color m_intensity;
    Point m_position;
    Vector m_direction;
    float m_angle;
    float m_falloff;

public:
    SpotLight(const Properties &properties) {
        m_position = properties.get<Point>("position");
        m_direction = properties.get<Vector>("direction").normalized();
        m_intensity = properties.get<Color>("intensity");
        m_angle = properties.get<float>("angle") * 0.5f * Deg2Rad;
        m_falloff = properties.get<float>("falloff") * 0.5f * Deg2Rad;
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        DirectLightSample sample;
        auto [length, dir] = Vector(origin - m_position).lengthAndNormalized();
        float angle = safe_acos(m_direction.dot(dir));

        if (angle > m_angle + m_falloff) {
            sample.weight = Color(0);
        } else if (angle > m_angle) {
            sample.weight = m_intensity * (1.f - (angle - m_angle) / m_falloff);
        } else {
            sample.weight = m_intensity;
        }

        sample.wi = -dir;
        sample.distance = length;
        return sample;
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format("SpotLight[\n"
                           "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(SpotLight, "spot")
