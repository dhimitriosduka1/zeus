#include <lightwave.hpp>

namespace lightwave {

class ConstantTexture : public Texture {
    Color m_value;

public:
    ConstantTexture(const Properties &properties) {
        m_value = properties.get<Color>("value");
    }

    Color evaluate(const Point2 &uv) const override { return m_value; }

    std::string toString() const override {
        return tfm::format("ConstantTexture[\n"
                           "  value = %s\n"
                           "]",
                           indent(m_value));
    }
};

} // namespace lightwave

REGISTER_TEXTURE(ConstantTexture, "constant")
