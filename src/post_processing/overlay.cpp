#include <lightwave.hpp>

/*
    Template for the overlay postprocess
    <postprocess type="overlay" amount="0.55">
        <ref name="input" id="bloomed"/>
        <image name="overlay" filename="./textures/lensflare.png"/>
        <image id="final_flared"/>
    </postprocess>
*/

namespace lightwave {  
    class Overlay : public Postprocess {
    public:
        Overlay(const Properties &properties) : Postprocess(properties) {
            m_overlay = properties.get<Image>("overlay");
            m_amount = properties.get<float>("amount", 1.0f);
            m_mode = properties.getEnum<OverlayMode>("mode", OverlayMode::Add,
                                            {
                                                    {"add",  OverlayMode::Add},
                                                    {"multiply", OverlayMode::Multiply},
                                            });
        }

        void execute() override {
            logger(EDebug, "Applying Overlay to image");
            m_output->copy(*m_input);

            if (m_mode == OverlayMode::Add) {
                blendAdd(*m_output, m_overlay, m_amount);
            } else if (m_mode == OverlayMode::Multiply) {
                blendMult(*m_output, m_overlay, m_amount);
            }

            logger(EDebug, "Saving Overlayed image");
            
            m_output->save();

            logger(EDebug, "Overlayed image saved sucessfully");
        }

        std::string toString() const override {
            return tfm::format("Overlay post-process");
        }

    private:
        enum class OverlayMode {
            Add,
            Multiply
        };

        ref<Image> m_overlay;
        float m_amount;
        OverlayMode m_mode;

        void blendAdd(Image &background, ref<Image> toAdd, const float mult) {
            const Point2i res = background.resolution();
            for (int x = 0; x < res.x(); x++) {
                for (int y = 0; y < res.y(); y++) {
                    background.get(Point2i(x, y)) += toAdd->get(Point2i(x, y)) * mult;
                }
            }
        }

        void blendMult(Image &background, ref<Image> toAdd, const float mult) {
            const Point2i res = background.resolution();
            for (int x = 0; x < res.x(); x++) {
                for (int y = 0; y < res.y(); y++) {
                    background.get(Point2i(x, y)) *= toAdd->get(Point2i(x, y)) * mult;
                }
            }
        }
    };
}
REGISTER_POSTPROCESS(Overlay, "overlay")