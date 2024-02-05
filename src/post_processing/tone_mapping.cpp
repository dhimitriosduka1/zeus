#include <lightwave.hpp>

/*
    Template for the tune mapping postprocess
    <postprocess type="tone_mapping">
        <string name="useLogScale" value="false" />
        <ref name="input" id="denoised" />
        <image id="output"/>
    </postprocess>
*/

namespace lightwave {  

    class ToneMapping : public Postprocess {
    public:
        ToneMapping(const Properties &properties) : Postprocess(properties) { 
            m_useLogScale = properties.get<bool>("useLogScale", true);
        }

        void execute() override {
            logger(EDebug, "Applying tone mapping using %s", m_useLogScale ? "Extendend Log Scale" : "Reinhard-Jordie");
            m_output->initialize(m_input->resolution());
        
            m_output->replace(m_useLogScale ? extendedLogScale() : reinhardJodie());

            // Save image after tone mapping
            logger(EDebug, "Saving toned mapped image");

            m_output->save();

            logger(EDebug, "Tone mapped image saved sucessfully");
            
            Streaming stream { *m_output };
            stream.update();
        }

        std::string toString() const override {
            return tfm::format("Tone mapping post-process");
        }
    private:
        bool m_useLogScale;

        float applyExtendedLogScale(float channel, float max) {
            const float a = 1.f / (log10(1.f + max));
            const float numerator = log(1.f + channel);
            const float denominator = log(2.f + 8.f * pow(channel / max, log(0.8f) / log(0.5f)));
            return a * numerator / denominator;
        }

        std::vector<Color> extendedLogScale() {
            std::vector<Color> toned;
            float maxR = 0.f, maxG = 0.f, maxB = 0.f;
            
            for (auto pixel : m_input->bounds()) {
                const Color color = m_input->get(pixel);
                maxR = std::max(maxR, color.r());
                maxG = std::max(maxG, color.g());
                maxB = std::max(maxB, color.b());
            }

            for (auto pixel : m_input->bounds()) {
                const Color current = m_input->get(pixel);
                toned.push_back({
                    applyExtendedLogScale(current.r(), maxR),
                    applyExtendedLogScale(current.g(), maxG),
                    applyExtendedLogScale(current.b(), maxB),
                });
            }
            return toned;
        }

        Color applyReinhardJodie(Color v) {
            const float l = luminance(v);
            const Color tv = v / (Color(1.0f) + v);
            const Color t = v / (1.0f + l);
            const float r = lerp(t.r(), tv.r(), tv.r());
            const float g = lerp(t.g(), tv.g(), tv.g());
            const float b = lerp(t.b(), tv.b(), tv.b());
            return {r, g, b};
        }

        std::vector<Color> reinhardJodie() {
            std::vector<Color> toned;

            for (auto pixel : m_input->bounds()) 
                toned.push_back(applyReinhardJodie(m_input->get(pixel)));
            
            return toned;
        }

        float luminance(Color v) {
            return Vector(v.r(), v.g(), v.b()).dot(Vector(0.2126f, 0.7152f, 0.0722f));
        }
    };
}
REGISTER_POSTPROCESS(ToneMapping, "tone_mapping")