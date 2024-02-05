#include <lightwave.hpp>

#ifdef LW_WITH_OIDN

#include <OpenImageDenoise/oidn.h>
#include <OpenImageDenoise/oidn.hpp>
#include <OpenImageDenoise/config.h>

/*
    General sceleton of the denoising postprocess
    <postprocess type="denoising">
        <ref name="input" id="noisy"/>
        <ref name="normals" id="normals"/>
        <ref name="albedo" id="albedo"/>
        <image id="denoised"/>
    </postprocess>
*/

namespace lightwave {  
    class Denoising : public Postprocess {
    public:
        Denoising(const Properties &properties) : Postprocess(properties) {
            m_normals = properties.get<Image>("normals");
            m_albedo = properties.get<Image>("albedo");

            m_type = oidn::DeviceType::CPU; // This can be replaced with GPU

            initDevice();            
        }

        void execute() override {
            logger(EDebug, "Applying denoising to image");
            // Init output image
            m_output->initialize(m_input->resolution());
            
            const Point2i res = m_input->resolution();
            const int width = res.x();
            const int height = res.y();

            const oidn::Format format = oidn::Format::Float3;

            oidn::FilterRef filter = m_device.newFilter("RT");
            
            filter.setImage("color", m_input->data(), format, width, height);
            filter.setImage("normal", m_normals->data(), format, width, height);
            filter.setImage("albedo", m_albedo->data(), format, width, height);
            filter.setImage("output", m_output->data(), format, width, height);

            filter.set("hdr", true);
            
            filter.setProgressMonitorFunction([](void * userPtr, double n) -> bool {
                logger(EDebug, "Denoising image...\n");
                return true;
            });

            filter.commit();    
            filter.execute();
            
            logger(EDebug, "Saving denoised image");

            m_output->save();

            logger(EDebug, "Denoised image saved sucessfully");

            Streaming stream { *m_output };
            stream.update();
        }

        std::string toString() const override {
            return tfm::format("Denoising post-process");
        }

    private:
        ref<Image> m_normals;
        ref<Image> m_albedo;

        oidn::DeviceType m_type;
        oidn::PhysicalDeviceRef m_physicalDevice;
        oidn::DeviceRef m_device;

        void initDevice() {
            m_device = oidn::newDevice(m_type);

            const char* errorMessage;
            if (m_device.getError(errorMessage) != oidn::Error::None) {
                lightwave_throw(errorMessage);
            }  

            m_device.setErrorFunction([](void* userPtr, oidn::Error error, const char* message) {
                lightwave_throw("OIDN Error: %s", message);
            });
            m_device.commit();
        }
    };
}
REGISTER_POSTPROCESS(Denoising, "denoising")

#endif // LW_WITH_OIDN