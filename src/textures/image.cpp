#include <lightwave.hpp>

namespace lightwave {

    class ImageTexture : public Texture {
        enum class BorderMode {
            Clamp,
            Repeat,
        };

        enum class FilterMode {
            Nearest,
            Bilinear,
        };

        ref<Image> m_image;
        float m_exposure;
        BorderMode m_border;
        FilterMode m_filter;
        Vector2 m_scale;

    public:
        ImageTexture(const Properties &properties) {
            if (properties.has("filename")) {
                m_image = std::make_shared<Image>(properties);
            } else {
                m_image = properties.getChild<Image>();
            }
            m_exposure = properties.get<float>("exposure", 1);
            m_scale = properties.get<Vector2>("scale", Vector2(1, 1));

            m_border =
                    properties.getEnum<BorderMode>("border", BorderMode::Repeat,
                                                   {
                                                           {"clamp",  BorderMode::Clamp},
                                                           {"repeat", BorderMode::Repeat},
                                                   });

            m_filter = properties.getEnum<FilterMode>(
                    "filter", FilterMode::Bilinear,
                    {
                            {"nearest",  FilterMode::Nearest},
                            {"bilinear", FilterMode::Bilinear},
                    });
        }

        Color evaluate(const Point2 &uv) const override {
            return handleFilter({uv.x() * m_scale.x(), 1 - uv.y() * m_scale.y()});
        }

        Point2i handleBorder(const Point2i &lattice) const {
            if (m_border == BorderMode::Repeat) {
                 return {mod(lattice.x(), m_image->resolution().x()) , mod(lattice.y(), (m_image->resolution().y()))};
            }
            return {
                    std::clamp(lattice.x(), 0, m_image->resolution().x() - 1),
                    std::clamp(lattice.y(), 0, m_image->resolution().y() - 1)
            };
        }

        Color handleFilter(const Point2 &uv) const {
            const Point2i resolution = m_image->resolution();

            if (m_filter == FilterMode::Nearest) {
                // Denormalized coords
                const Point2 dc = {uv.x() * (float) resolution.x(), uv.y() * (float) resolution.y()};

                // Get lattice coordinates
                const Point2i lattice = {(int) std::floor(dc.x()), (int) std::floor(dc.y())};
                return m_exposure * m_image->get(handleBorder(lattice));
            }

            // Denormalized coords
            const Point2 dc = {uv.x() * (float) resolution.x() - .5f, uv.y() * (float) resolution.y() - .5f};

            // Get lattice coordinates
            const Point2i lattice = {(int) std::floor(dc.x()), (int) std::floor(dc.y())};

            // Get the fractional part
            const Point2 fc = {extractFractionalPart(dc.x()), extractFractionalPart(dc.y())};

            const Color n1 = (1.0f - fc.x()) * (1.0f - fc.y()) * getNeighbour(lattice, 0, 0);
            const Color n2 = (1.0f - fc.x()) * fc.y() * getNeighbour(lattice, 0, 1);
            const Color n3 = fc.x() * (1.0f - fc.y()) * getNeighbour(lattice, 1, 0);
            const Color n4 = fc.x() * fc.y() * getNeighbour(lattice, 1, 1);
            return m_exposure * (n1 + n2 + n3 + n4);
        }

        Color getNeighbour(const Point2i dc, const int xOffset, const int yOffset) const {
            return m_image->get(handleBorder({(dc.x() + xOffset), (dc.y() + yOffset)}));
        }

        static float extractFractionalPart(const float val) {
            return val - std::floor(val);
        }

        std::string toString() const override {
            return tfm::format("ImageTexture[\n"
                               "  image = %s,\n"
                               "  exposure = %f,\n"
                               "]",
                               indent(m_image), m_exposure);
        }
    };

} // namespace lightwave

REGISTER_TEXTURE(ImageTexture, "image")
