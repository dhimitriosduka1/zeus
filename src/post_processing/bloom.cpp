#include <lightwave.hpp>

/*
    Template for the bloom postprocess
    <postprocess type="bloom" kernelSize="24" iterations="5" threshold="1.3" softThreshold="0.3">
        <ref name="input" id="denoised"/>
        <image id="bloomed"/>
    </postprocess>
*/

namespace lightwave {  
    class Bloom : public Postprocess {
    public:
        Bloom(const Properties &properties) : Postprocess(properties) {
            m_kernelSize = properties.get<int>("kernelSize", 10);
            m_iterations = properties.get<int>("iterations", 3);
            m_threshold = properties.get<float>("threshold", 1.f);
            m_softThreshold = properties.get<float>("softThreshold", 0.5f);
            m_amount = properties.get<float>("amount", 1.0f);
            m_amountSmall = properties.get<float>("amountSmall", 1.0f);
        }

        void execute() override {
            logger(EDebug, "Applying bloom to image");
            m_output->copy(*m_input);
            
            Image result;
            result.copy(*m_input);
            applyThresholdSimple(result, m_threshold);
            result = boxBlur(result, 3);
            result = boxBlur(result, 3);
            result = boxBlur(result, 4);
            blendAdd(*m_output, result, m_amountSmall);

            for (int i = 0; i < m_iterations; i++) {
                result = boxBlur(result, m_kernelSize);
            }

            blendAdd(*m_output, result, m_amount);

            logger(EDebug, "Saving bloomed image");
            
            m_output->save();

            logger(EDebug, "Bloomed image saved sucessfully");

            Streaming stream { *m_output };
            stream.update();
        }

        std::string toString() const override {
            return tfm::format("Bloom post-process");
        }

    private:
        int m_kernelSize;
        float m_iterations;
        float m_threshold;
        float m_softThreshold;
        float m_amount;
        float m_amountSmall;

        Image boxBlur(const Image &image, const int size) {
            const Point2i res = image.resolution();
            const float weight = 1.f / ((size + size + 1) * (size + size + 1));
            Image result(res);
            for (int x = 0; x < res.x(); x++) {
                for (int y = 0; y < res.y(); y++) {
                    Color total;
                    for (int i = -size; i <= size; i++) {
                        for (int j = -size; j <= size; j++) {
                            int xc = clampInt(x + i, 0, res.x() - 1);
                            int yc = clampInt(y + j, 0, res.y() - 1);

                            total += image.get(Point2i(xc, yc));
                        }
                    }
                    result.get(Point2i(x, y)) = total * weight;
                }
            }

            return result;
        }

        void applyThreshold(Image &image, float threshold, float softThreshold) {
            const Point2i res = image.resolution();
            const float knee = threshold * softThreshold;
            for (int x = 0; x < res.x(); x++) {
                for (int y = 0; y < res.y(); y++) {
                    const Color col = image.get(Point2i(x, y));
                    const float brightness = max(col.r(), max(col.g(), col.b()));
                    float soft = brightness - threshold + knee;
                    soft = clamp(soft, 0, 2.f * knee);
                    soft = soft * soft * (4.f * knee + 0.00001f);
                    float contribution = max(soft, brightness - threshold);
                    contribution /= max(brightness, 0.00001f);
                    image.get(Point2i(x, y)) *= contribution;
                }
            }
        }

        void applyThresholdSimple(Image &image, float threshold) {
            const Point2i res = image.resolution();
            for (int x = 0; x < res.x(); x++) {
                for (int y = 0; y < res.y(); y++) {
                    const Color col = image.get(Point2i(x, y));
                    const float brightness = max(col.r(), max(col.g(), col.b()));
                    image.get(Point2i(x, y)) *= brightness > threshold ? 1.f : 0.f;
                }
            }
        }

        void blendAdd(Image &background, const Image &toAdd, const float mult) {
            const Point2i res = background.resolution();
            for (int x = 0; x < res.x(); x++) {
                for (int y = 0; y < res.y(); y++) {
                    background.get(Point2i(x, y)) += toAdd.get(Point2i(x, y)) * mult;
                }
            }
        }
    };
}
REGISTER_POSTPROCESS(Bloom, "bloom")