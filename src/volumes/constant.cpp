#include <lightwave.hpp>

namespace lightwave {

/// @brief Volume with a constant density
class ConstantVolume : public Volume {
    float m_value;

public:
    ConstantVolume(const Properties &properties) {
        m_value = properties.get<float>("value");
    }

    float evaluate(const Point &pos) const override { return m_value; }

    float getMaxDensity() const override { return m_value; }

    std::string toString() const override {
        return tfm::format("ConstantVolume[\n"
                           "  value = %s\n"
                           "]",
                           indent(m_value));
    }
};

}

REGISTER_VOLUME(ConstantVolume, "constant")
