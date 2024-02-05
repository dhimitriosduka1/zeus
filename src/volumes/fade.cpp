#include <lightwave.hpp>

namespace lightwave {

/// @brief Volume with a constant density
class FadeVolume : public Volume {
    float m_value;

public:
    FadeVolume(const Properties &properties) {
        m_value = properties.get<float>("value");
    }

    float evaluate(const Point &pos) const override {
        Point p(pos.x() * 0.5f + 0.5f, pos.y() * 0.5f + 0.5f, pos.z() * 0.5f + 0.5f);
        return (p.y()*p.y()) * p.z() * m_value;
    }

    float getMaxDensity() const override { return m_value; }

    std::string toString() const override {
        return tfm::format("FadeVolume[\n"
                           "  value = %s\n"
                           "]",
                           indent(m_value));
    }
};

}

REGISTER_VOLUME(FadeVolume, "fade")
