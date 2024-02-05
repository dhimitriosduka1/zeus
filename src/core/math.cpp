#include <lightwave/math.hpp>
#include <lightwave/color.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/bsdf.hpp>
#include <lightwave/emission.hpp>
#include <lightwave/light.hpp>

namespace lightwave {

// based on the MESA implementation of the GLU library
std::optional<Matrix4x4> invert(const Matrix4x4 &m) {
    Matrix4x4 inv;
    inv(0, 0) = m(1, 1) * m(2, 2) * m(3, 3) - 
               m(1, 1) * m(2, 3) * m(3, 2) - 
               m(2, 1) * m(1, 2) * m(3, 3) + 
               m(2, 1) * m(1, 3) * m(3, 2) +
               m(3, 1) * m(1, 2) * m(2, 3) - 
               m(3, 1) * m(1, 3) * m(2, 2);

    inv(1, 0) = -m(1, 0) * m(2, 2) * m(3, 3) + 
                m(1, 0) * m(2, 3) * m(3, 2) + 
                m(2, 0) * m(1, 2) * m(3, 3) - 
                m(2, 0) * m(1, 3) * m(3, 2) - 
                m(3, 0) * m(1, 2) * m(2, 3) + 
                m(3, 0) * m(1, 3) * m(2, 2);

    inv(2, 0) = m(1, 0) * m(2, 1) * m(3, 3) - 
               m(1, 0) * m(2, 3) * m(3, 1) - 
               m(2, 0) * m(1, 1) * m(3, 3) + 
               m(2, 0) * m(1, 3) * m(3, 1) + 
               m(3, 0) * m(1, 1) * m(2, 3) - 
               m(3, 0) * m(1, 3) * m(2, 1);

    inv(3, 0) = -m(1, 0) * m(2, 1) * m(3, 2) + 
                 m(1, 0) * m(2, 2) * m(3, 1) +
                 m(2, 0) * m(1, 1) * m(3, 2) - 
                 m(2, 0) * m(1, 2) * m(3, 1) - 
                 m(3, 0) * m(1, 1) * m(2, 2) + 
                 m(3, 0) * m(1, 2) * m(2, 1);

    const float det = m(0, 0) * inv(0, 0) + m(0, 1) * inv(1, 0) + m(0, 2) * inv(2, 0) + m(0, 3) * inv(3, 0);
    if (det == 0)
        return {};

    inv(0, 1) = -m(0, 1) * m(2, 2) * m(3, 3) + 
                m(0, 1) * m(2, 3) * m(3, 2) + 
                m(2, 1) * m(0, 2) * m(3, 3) - 
                m(2, 1) * m(0, 3) * m(3, 2) - 
                m(3, 1) * m(0, 2) * m(2, 3) + 
                m(3, 1) * m(0, 3) * m(2, 2);

    inv(1, 1) = m(0, 0) * m(2, 2) * m(3, 3) - 
               m(0, 0) * m(2, 3) * m(3, 2) - 
               m(2, 0) * m(0, 2) * m(3, 3) + 
               m(2, 0) * m(0, 3) * m(3, 2) + 
               m(3, 0) * m(0, 2) * m(2, 3) - 
               m(3, 0) * m(0, 3) * m(2, 2);

    inv(2, 1) = -m(0, 0) * m(2, 1) * m(3, 3) + 
                m(0, 0) * m(2, 3) * m(3, 1) + 
                m(2, 0) * m(0, 1) * m(3, 3) - 
                m(2, 0) * m(0, 3) * m(3, 1) - 
                m(3, 0) * m(0, 1) * m(2, 3) + 
                m(3, 0) * m(0, 3) * m(2, 1);

    inv(3, 1) = m(0, 0) * m(2, 1) * m(3, 2) - 
                m(0, 0) * m(2, 2) * m(3, 1) - 
                m(2, 0) * m(0, 1) * m(3, 2) + 
                m(2, 0) * m(0, 2) * m(3, 1) + 
                m(3, 0) * m(0, 1) * m(2, 2) - 
                m(3, 0) * m(0, 2) * m(2, 1);

    inv(0, 2) = m(0, 1) * m(1, 2) * m(3, 3) - 
               m(0, 1) * m(1, 3) * m(3, 2) - 
               m(1, 1) * m(0, 2) * m(3, 3) + 
               m(1, 1) * m(0, 3) * m(3, 2) + 
               m(3, 1) * m(0, 2) * m(1, 3) - 
               m(3, 1) * m(0, 3) * m(1, 2);

    inv(1, 2) = -m(0, 0) * m(1, 2) * m(3, 3) + 
                m(0, 0) * m(1, 3) * m(3, 2) + 
                m(1, 0) * m(0, 2) * m(3, 3) - 
                m(1, 0) * m(0, 3) * m(3, 2) - 
                m(3, 0) * m(0, 2) * m(1, 3) + 
                m(3, 0) * m(0, 3) * m(1, 2);

    inv(2, 2) = m(0, 0) * m(1, 1) * m(3, 3) - 
                m(0, 0) * m(1, 3) * m(3, 1) - 
                m(1, 0) * m(0, 1) * m(3, 3) + 
                m(1, 0) * m(0, 3) * m(3, 1) + 
                m(3, 0) * m(0, 1) * m(1, 3) - 
                m(3, 0) * m(0, 3) * m(1, 1);

    inv(3, 2) = -m(0, 0) * m(1, 1) * m(3, 2) + 
                 m(0, 0) * m(1, 2) * m(3, 1) + 
                 m(1, 0) * m(0, 1) * m(3, 2) - 
                 m(1, 0) * m(0, 2) * m(3, 1) - 
                 m(3, 0) * m(0, 1) * m(1, 2) + 
                 m(3, 0) * m(0, 2) * m(1, 1);

    inv(0, 3) = -m(0, 1) * m(1, 2) * m(2, 3) + 
                m(0, 1) * m(1, 3) * m(2, 2) + 
                m(1, 1) * m(0, 2) * m(2, 3) - 
                m(1, 1) * m(0, 3) * m(2, 2) - 
                m(2, 1) * m(0, 2) * m(1, 3) + 
                m(2, 1) * m(0, 3) * m(1, 2);

    inv(1, 3) = m(0, 0) * m(1, 2) * m(2, 3) - 
               m(0, 0) * m(1, 3) * m(2, 2) - 
               m(1, 0) * m(0, 2) * m(2, 3) + 
               m(1, 0) * m(0, 3) * m(2, 2) + 
               m(2, 0) * m(0, 2) * m(1, 3) - 
               m(2, 0) * m(0, 3) * m(1, 2);

    inv(2, 3) = -m(0, 0) * m(1, 1) * m(2, 3) + 
                 m(0, 0) * m(1, 3) * m(2, 1) + 
                 m(1, 0) * m(0, 1) * m(2, 3) - 
                 m(1, 0) * m(0, 3) * m(2, 1) - 
                 m(2, 0) * m(0, 1) * m(1, 3) + 
                 m(2, 0) * m(0, 3) * m(1, 1);

    inv(3, 3) = m(0, 0) * m(1, 1) * m(2, 2) - 
                m(0, 0) * m(1, 2) * m(2, 1) - 
                m(1, 0) * m(0, 1) * m(2, 2) + 
                m(1, 0) * m(0, 2) * m(2, 1) + 
                m(2, 0) * m(0, 1) * m(1, 2) - 
                m(2, 0) * m(0, 2) * m(1, 1);

    return inv * (1.f / det);
}

void buildOrthonormalBasis(const Vector &a, Vector &b, Vector &c) {
    if (abs(a.x()) > abs(a.y())) {
        auto invLen = 1 / sqrt(a.x() * a.x() + a.z() * a.z());
        c = { a.z() * invLen, 0, -a.x() * invLen };
    } else {
        auto invLen = 1 / sqrt(a.y() * a.y() + a.z() * a.z());
        c = { 0, a.z() * invLen, -a.y() * invLen };
    }
    b = c.cross(a);
}

Vector2 directionToSphereUV(const Vector& dir)
{
    return {
        .5f - (float) atan2(dir.z(), dir.x()) / (2 * Pi),
        .5f - (float) asin(dir.y()) * InvPi
    };
}

Color Intersection::evaluateEmission() const {
    if (!instance->emission()) return Color::black();
    return instance->emission()->evaluate(uv, frame.toLocal(wo)).value;
}

BsdfSample Intersection::sampleBsdf(Sampler &rng) const {
    if (!instance->bsdf()) return BsdfSample::invalid();
    assert_normalized(wo, {});
    auto bsdfSample = instance->bsdf()->sample(uv, frame.toLocal(wo), rng);
    if (bsdfSample.isInvalid()) return bsdfSample;
    assert_normalized(bsdfSample.wi, {
        logger(EError, "offending BSDF: %s", instance->bsdf()->toString());
        logger(EError, "  input was: %s with length %f", wo, wo.length());
    });
    bsdfSample.wi = frame.toWorld(bsdfSample.wi);
    assert_normalized(bsdfSample.wi, {
        logger(EError, "tangent frame: %s / %s / %s", frame.tangent, frame.bitangent, frame.normal);
    });
    return bsdfSample;
}

BsdfEval Intersection::evaluateBsdf(const Vector &wi) const {
    if (!instance->bsdf())
        return BsdfEval::invalid();
    return instance->bsdf()->evaluate(uv, frame.toLocal(wo), frame.toLocal(wi));
}

}
