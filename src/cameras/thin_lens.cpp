#include <lightwave.hpp>

namespace lightwave {
    class ThinLens : public Camera {
    protected:
        /// @brief Radius of the aperture
        float m_aperture;
        /// @brief Focal distance of the camera
        float m_focalDistance;
    public:
        ThinLens(const Properties &properties) : Camera(properties) {
            m_focalDistance = properties.get<float>("focalDistance", 1.f);
            m_aperture = properties.get<float>("aperture", 0.f);
        }

        CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
            // Generate random sample in the circle with radius m_aperture
            const Point2 sampleInUnitDisk = squareToUniformDiskConcentric(rng.next2D());
            const Point sampleInDisk = {m_aperture * sampleInUnitDisk.x(), m_aperture * sampleInUnitDisk.y(), 0.f};

            // Generate the localRay
            const Ray localRay = Ray(
                    Point(0.f),
                    Vector(normalized.x() * m_plane.x(), normalized.y() * m_plane.y(), 1.f).normalized()
            );

            // Find point of focus using localRay
            const float ft = m_focalDistance / localRay.direction.z();
            const Point pointOfFocus = localRay(ft);

            // Shift the origin using the sample point in disk
            const Point shiftedOrigin = Point(sampleInDisk.x(), sampleInDisk.y(), 0.f);

            return CameraSample(
                    m_transform->apply(Ray(shiftedOrigin, (pointOfFocus - shiftedOrigin).normalized())),
                    Color(1.f));
        }

        std::string toString() const override {
            return tfm::format(
                    "Perspective[\n"
                    "  width = %d,\n"
                    "  height = %d,\n"
                    "  transform = %s,\n"
                    "]",
                    m_resolution.x(),
                    m_resolution.y(),
                    indent(m_transform)
            );
        }
    };

}

REGISTER_CAMERA(ThinLens, "thin_lens")

