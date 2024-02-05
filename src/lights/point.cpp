#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {
    Color m_color;
    Point m_position;

public:
    PointLight(const Properties &properties) {
        m_color = properties.get<Color>("power", Color(1.f));
        m_position = properties.get<Point>("position");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        auto [length, dir] = Vector(m_position - origin).lengthAndNormalized();
        return { dir, m_color / (sqr(length) * Pi * 4.f), length };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format("PointLight[\n"
                           "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(PointLight, "point")
