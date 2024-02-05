#include <lightwave/camera.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

CameraSample Camera::sample(const Point2i &pixel, Sampler &rng) const {
    // begin by sampling a random position within the pixel
    const auto pixelPlusRandomOffset = Vector2(pixel.cast<float>()) + Vector2(rng.next2D());
    // normalize by image resolution to end up with value in range [-1,-1] to [+1,+1]
    const auto normalized = 2 * pixelPlusRandomOffset / m_resolution.cast<float>() - Vector2(1);
    // generate the sample using the normalized sample function
    const auto s = sample(normalized, rng);
    assert_normalized(s.ray.direction, {
        logger(EError, "your Camera::sample implementation returned a non-normalized direction");
    });
    return s;
}

}
