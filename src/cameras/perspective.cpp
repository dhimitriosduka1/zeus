#include <lightwave.hpp>

namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 * 
 * In local coordinates (before applying m_transform), the camera looks in positive z direction [0,0,1].
 * Pixels on the left side of the image ( @code normalized.x < 0 @endcode ) are directed in negative x
 * direction ( @code ray.direction.x < 0 ), and pixels at the bottom of the image ( @code normalized.y < 0 @endcode )
 * are directed in negative y direction ( @code ray.direction.y < 0 ).
 */
class Perspective : public Camera {
public:
    Perspective(const Properties &properties) : Camera(properties) {}

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        const Ray local_ray = {
            Point(0.f),
            Vector(normalized.x() * m_plane.x(), normalized.y() * m_plane.y(), 1.f).normalized()
        };

        return CameraSample{
            .ray = m_transform->apply(local_ray).normalized(),
            .weight = Color(1.f)};
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

REGISTER_CAMERA(Perspective, "perspective")
