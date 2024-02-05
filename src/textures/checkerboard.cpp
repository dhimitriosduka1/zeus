#include <lightwave.hpp>

namespace lightwave {

class CheckerboardTexture : public Texture {
public:
    CheckerboardTexture(const Properties &properties) {
        m_scale = properties.get<Vector2>("scale", Vector2(1, 1));
        m_color0 = properties.get<Color>("color0", Color(0.f));
        m_color1 = properties.get<Color>("color1", Color(1.f));
    }

    Color evaluate(const Point2 &uv) const override {
        const Vector2i scaled = Vector2i((int) floor(m_scale.x() * uv.x()), (int) floor(m_scale.y() * uv.y()));
        const int parity = (scaled.x() + scaled.y()) % 2;
        return parity == 1 ? m_color1 : m_color0;
    }

    std::string toString() const override {
        NOT_IMPLEMENTED
    }

private:
    Vector2 m_scale;
    Color m_color0;
    Color m_color1;
};

} // namespace lightwave

REGISTER_TEXTURE(CheckerboardTexture, "checkerboard")
