#include <lightwave.hpp>

namespace lightwave {

/**
 * @brief Tests whether the output of an integrator matches a given reference image.
 * 
 * Paired with different integrators, this can be used to test nearly all aspects of your renderer:
 * For example, with a normal integrator (which returns the normal vector at the first intersection),
 * this test can verify that your intersection functions report the correct normal.
 * 
 * @note Internally, this computes the mean absolute error (MAE) of the image and compares it against
 * a specified threshold.
 */
class CompareImage : public Test {
    /// @brief The integrator to execute and compare against a reference image.
    ref<SamplingIntegrator> m_integrator;
    /// @brief The directory the resulting image should be stored to.
    std::filesystem::path m_basePath;
    /// @brief The threshold to compare the MAE (mean absolute error) against.
    float m_thresholdMAE;
    /// @brief The threshold to compare the ME (mean error) against.
    float m_thresholdME;
    /// @brief Whether to report an error if any color channels in the output image are negative.
    bool m_allowNegative;

public:
    CompareImage(const Properties &properties) {
        m_integrator = properties.getChild<SamplingIntegrator>();
        m_thresholdMAE = properties.get<float>("mae", 1e-1);
        m_thresholdME = properties.get<float>("me", 2e-4);
        m_basePath = properties.basePath(); // we store the test image in the same folder as the scene file
        m_allowNegative = properties.get<bool>("allowNegative", true);
    }

    void execute() override {
        std::filesystem::path referencePath = m_basePath / (id() + "_ref.exr");

        ref<Image> image = std::make_shared<Image>();
        image->setBasePath(m_basePath);
        image->setId(id() + "_test");
        m_integrator->setImage(image);
        m_integrator->execute();

        if (std::getenv("reference")) {
            image->saveAt(referencePath);
        } else {
            ref<Image> reference = std::make_shared<Image>(referencePath);
            reference->setId(id() + "_ref");

            Streaming stream { *reference };
            stream.update();
            
            compare(*image, *reference);
            logger(EInfo, "test passed!");
        }
    }

    std::string toString() const override {
        return "CompareImage[]";
    }

private:
    void compare(const Image &image, const Image &reference) const {
        if (image.resolution() != reference.resolution()) {
            lightwave_throw("resolution does not match reference image");
        }

        double error = 0;
        double absError = 0;
        for (auto pixel : image.bounds()) {
            Color i = image.get(pixel);
            Color r = reference.get(pixel);
            for (int channel = 0; channel < r.NumComponents; channel++) {
                if (std::isnan(i[channel])) lightwave_throw("nan encountered at pixel %d,%d", pixel.x(), pixel.y());
                if (std::isinf(i[channel])) lightwave_throw("infinity encountered at pixel %d,%d", pixel.x(), pixel.y());
                if (!m_allowNegative && i[channel] < 0) lightwave_throw("negative value encountered at pixel %d,%d", pixel.x(), pixel.y());
            }
            for (int chan = 0; chan < i.NumComponents; chan++) {
                error += i[chan] - r[chan];
                absError += abs(i[chan] - r[chan]);
            }
        }

        // normalize error values
        error /= Color::NumComponents * image.bounds().diagonal().product();
        absError /= Color::NumComponents * image.bounds().diagonal().product();

        if (absError > m_thresholdMAE) lightwave_throw("absolute error threshold exceeded (%.3g > %.3g)", absError, m_thresholdMAE);
        if (abs(error) > m_thresholdME) lightwave_throw("error threshold exceeded (%.3g > %.3g)", abs(error), m_thresholdME);
    }
};

}

REGISTER_TEST(CompareImage, "image");
