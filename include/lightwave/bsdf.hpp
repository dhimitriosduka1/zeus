/**
 * @file bsdf.hpp
 * @brief Contains the Bsdf interface and related structures.
 */

#pragma once

#include <lightwave/color.hpp>
#include <lightwave/core.hpp>
#include <lightwave/math.hpp>

namespace lightwave {

/// @brief The result of sampling a material using @ref Bsdf::sample .
struct BsdfSample {
    /// @brief The direction vector, pointing away from the surface.
    Vector wi;
    /// @brief The weight of the sample, given by @code cos(theta) * B(wi, wo) /
    /// p(wi) @endcode
    Color weight;

    /// @brief Return an invalid sample, used to denote that sampling has
    /// failed.
    static BsdfSample invalid() {
        return {
            .wi     = Vector(0),
            .weight = Color(0),
        };
    }

    /// @brief Tests whether the sample is invalid (i.e., sampling has failed).
    bool isInvalid() const { return weight == Color(0); }
};

/// @brief The result of evaluating a material using @ref Bsdf::evaluate .
struct BsdfEval {
    /// @brief The value of the Bsdf, given by @code cos(theta) * B(wi, wo)
    /// @endcode
    Color value;

    /// @brief Indicates the the Bsdf is zero for the given pair of directions.
    static BsdfEval invalid() {
        return {
            .value = Color(0),
        };
    }

    bool isInvalid() const { return value == Color(0); }
};

/// @brief A Bsdf, representing the scattering distribution of a surface.
class Bsdf : public Object {
public:
    /**
     * @brief Evaluates the Bsdf (including the cosine term) for a given pair
     * of directions in local coordinates (i.e., the normal is assumed to be
     * [0,0,1]).
     * @param uv The texture coordinates of the surface.
     * @param wo The outgoing direction light is scattered in, pointing away
     * from the surface, in local coordinates.
     * @param wi The incoming direction light comes from, pointing away
     * from the surface, in local coordinates.
     */
    virtual BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const = 0;
    /**
     * @brief Samples a direction according to the distribution of the Bsdf in
     * local coordinates (i.e., the normal is assumed to be [0,0,1]).
     * @note Can also produce invalid samples in case sampling fails (use @ref
     * BsdfSample::isInvalid() to check for this).
     * @param uv The texture coordinates of the surface.
     * @param wo The outgoing direction light is scattered in, pointing away
     * from the surface, in local coordinates.
     * @param rng A random number generator used to steer the sampling.
     */
    virtual BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const = 0;

    /**
     * @brief Samples the albedo for a particular uv.
     * @param uv The texture coordinates of the surface.
     */
    virtual Color albedo(const Point2 &uv) const = 0;
};

} // namespace lightwave
