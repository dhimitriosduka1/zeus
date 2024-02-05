/**
 * @brief Functions for dealing with microfacet distributions.
 * @file microfacet.hpp
 */

#pragma once

#include <lightwave/math.hpp>

namespace lightwave::microfacet {

/**
 * Change in density for @code wi = reflect(wo, normal) @endcode , i.e.,
 * @code p(wi) = p(normal) * detReflection(normal, wo) @endcode .
 * Needed for rough reflection (e.g., @c RoughConductor and
 * @c RoughDielectric ).
 * @see https://www.graphics.cornell.edu/~bjw/microfacetbsdf.pdf (Figure 6)
 */
inline float detReflection(const Vector &normal, const Vector &wo) {
    return 1 / abs(4 * normal.dot(wo));
}

/**
 * Change in density for @code wi = refract(wo, normal, eta) @endcode , i.e.,
 * @code p(wi) = p(normal) * detRefraction(normal, wi, wo, eta) @endcode .
 * Needed for rough refraction (e.g., if you decide to implement the optional
 * @c RoughDielectric ).
 * @see https://www.graphics.cornell.edu/~bjw/microfacetbsdf.pdf (Figure 7)
 */
inline float detRefraction(const Vector &normal, const Vector &wi,
                           const Vector &wo, float eta) {
    return sqr(eta) *
           abs(normal.dot(wi) / sqr(normal.dot(wi) * eta + normal.dot(wo)));
}

/**
 * Smith shadowing/masking function for the GGX microfacet distribution.
 * @param alpha The roughness of the distribution
 * @param wh The sampled normal (also known as half vector)
 * @param w The direction to evaluate for (either @c wo or @c wi )
 */
inline float smithG1(float alpha, const Vector &wh, const Vector &w) {
    /// Ensure correct orientation by projecting both @c w and @c wh into the
    /// upper hemisphere and checking that the angle they form is less than 90°
    if (w.dot(wh) * Frame::cosTheta(w) * Frame::cosTheta(wh) <= 0)
        return 0;

    /// Special case: if @c cosTheta of @c w is large, we know that the tangens
    /// will be @c 0 and hence our result is @c 1
    if (abs(Frame::cosTheta(w)) >= 1)
        return 1;

    const float a2tanTheta2 = sqr(alpha) * Frame::tanTheta2(w);
    return 2 / (1 + sqrt(1 + a2tanTheta2));
}

/**
 * Evaluates the GGX normal distribution function.
 * @param alpha The roughness of the distribution
 * @param wh The normal to be evaluated (also known as half vector)
 * @see "Microfacet Models for Refraction through Rough Surfaces" [Walter et al.
 * 2007]
 */
inline float evaluateGGX(float alpha, const Vector &wh) {
    float nDotH = Frame::cosTheta(wh);
    float a     = Frame::cosPhiSinTheta(wh) / alpha;
    float b     = Frame::sinPhiSinTheta(wh) / alpha;
    float c     = sqr(a) + sqr(b) + sqr(nDotH);
    return 1 / (Pi * sqr(alpha * c));
}

/**
 * Sampling of the visible normal distribution function (VNDF) of the GGX
 * microfacet distribution with Smith shadowing function by [Heitz 2018].
 * @param alpha The roughness of the distribution
 * @param wo The outgoing direction (determines which normals are visible)
 * @param rnd A random point to steer sampling (typically @c prng.next2D() )
 * @returns A normal sampled from the distribution of visible normals
 * @note The PDF of this sampling function is given by @c pdfGGXVNDF
 * @see For details on how and why this works, check out Eric Heitz' great JCGT
 * paper "Sampling the GGX Distribution of Visible Normals".
 */
inline Vector sampleGGXVNDF(float alpha, const Vector &wo, const Point2 &rnd) {
    // Addition: flip sign of incident vector for transmission
    float sgn = copysign(1, Frame::cosTheta(wo));
    // Section 3.2: transforming the view direction to the hemisphere
    // configuration
    Vector Vh =
        sgn * Vector(alpha * wo.x(), alpha * wo.y(), wo.z()).normalized();
    // Section 4.1: orthonormal basis (with special case if cross product is
    // zero)
    float lensq = Vh.x() * Vh.x() + Vh.y() * Vh.y();
    Vector T1 =
        lensq > 0 ? Vector(-Vh.y(), Vh.x(), 0) / sqrt(lensq) : Vector(1, 0, 0);
    Vector T2 = Vh.cross(T1);
    // Section 4.2: parameterization of the projected area
    float r   = sqrt(rnd.x());
    float phi = 2 * Pi * rnd.y();
    float t1  = r * cos(phi);
    float t2  = r * sin(phi);
    float s   = 0.5f * (1 + Vh.z());
    t2        = (1 - s) * sqrt(1 - sqr(t1)) + s * t2;
    // Section 4.3: reprojection onto hemisphere
    Vector Nh = t1 * T1 + t2 * T2 + safe_sqrt(1 - sqr(t1) - sqr(t2)) * Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    Vector Ne =
        Vector(alpha * Nh.x(), alpha * Nh.y(), max(0, Nh.z())).normalized();
    return sgn * Ne;
}

/**
 * The probability density of sampling a given normal using @c sampleGGXVNDF .
 * @param alpha The roughness of the distribution
 * @param wh The normal to compute the PDF for (also known as half vector)
 * @param wo The outgoing direction (determines which normals are visible)
 * @returns The probability density of @c sampleGGXVNDF sampling @c wh .
 */
inline float pdfGGXVNDF(float alpha, const Vector &wh, const Vector &wo) {
    // clang-format off
    return microfacet::evaluateGGX(alpha, wh) *
           microfacet::smithG1(alpha, wh, wo) *
           abs(wh.dot(wo)) / Frame::absCosTheta(wo);
    // clang-format on
}

//
//
// MARK: - advanced anisotropic versions below (not needed for the core
// assignments) feel free to use these if you want to implement anisotropic
// reflections as bonus feature
//
//

/**
 * Anisotropic Smith shadowing/masking function for the GGX microfacet
 * distribution.
 * @param ax The roughness of the distribution in x-direction
 * @param ay The roughness of the distribution in y-direction
 * @param wh The sampled normal (also known as half vector)
 * @param w The direction to evaluate for (either @c wo or @c wi )
 */
inline float anisotropicSmithG1(float ax, float ay, const Vector &wh,
                                const Vector &w) {
    /// Ensure correct orientation by projecting both @c w and @c wh into the
    /// upper hemisphere and checking that the angle they form is less than 90°
    if (w.dot(wh) * Frame::cosTheta(w) * Frame::cosTheta(wh) <= 0)
        return 0;

    /// Special case: if @c cosTheta of @c w is large, we know that the tangent
    /// will be @c 0 and hence our result is @c 1
    if (abs(Frame::cosTheta(w)) >= 1)
        return 1;

    const float a2tanTheta2 = (sqr(ax * Frame::cosPhiSinTheta(w)) +
                               sqr(ay * Frame::sinPhiSinTheta(w))) /
                              Frame::cosTheta2(w);
    return 2 / (1 + sqrt(1 + a2tanTheta2));
}

/**
 * Evaluates the anisotropic GGX normal distribution function.
 * @param ax The roughness of the distribution in x-direction
 * @param ay The roughness of the distribution in y-direction
 * @param wh The normal to be evaluated (also known as half vector)
 * @see "Microfacet Models for Refraction through Rough Surfaces" [Walter et al.
 * 2007]
 */
inline float evaluateAnisotropicGGX(float ax, float ay, const Vector &wh) {
    float nDotH = Frame::cosTheta(wh);
    float a     = Frame::cosPhiSinTheta(wh) / ax;
    float b     = Frame::sinPhiSinTheta(wh) / ay;
    float c     = sqr(a) + sqr(b) + sqr(nDotH);
    return 1 / (Pi * ax * ay * sqr(c));
}

/**
 * Sampling of the visible normal distribution function (VNDF) of the GGX
 * microfacet distribution with Smith shadowing function by [Heitz 2018].
 * @param ax The roughness of the distribution in x-direction
 * @param ay The roughness of the distribution in y-direction
 * @param wo The outgoing direction (determines which normals are visible)
 * @param rnd A random point to steer sampling (typically @c prng.next2D() )
 * @returns A normal sampled from the distribution of visible normals
 * @note The PDF of this sampling function is given by @c pdfGGXVNDF
 * @see For details on how and why this works, check out Eric Heitz' great JCGT
 * paper "Sampling the GGX Distribution of Visible Normals".
 */
inline Vector sampleAnisotropicGGXVNDF(float ax, float ay, const Vector &wo,
                                       const Point2 &rnd) {
    // Addition: flip sign of incident vector for transmission
    float sgn = copysign(1, Frame::cosTheta(wo));
    // Section 3.2: transforming the view direction to the hemisphere
    // configuration
    Vector Vh = sgn * Vector(ax * wo.x(), ay * wo.y(), wo.z()).normalized();
    // Section 4.1: orthonormal basis (with special case if cross product is
    // zero)
    float lensq = Vh.x() * Vh.x() + Vh.y() * Vh.y();
    Vector T1 =
        lensq > 0 ? Vector(-Vh.y(), Vh.x(), 0) / sqrt(lensq) : Vector(1, 0, 0);
    Vector T2 = Vh.cross(T1);
    // Section 4.2: parameterization of the projected area
    float r   = sqrt(rnd.x());
    float phi = 2 * Pi * rnd.y();
    float t1  = r * cos(phi);
    float t2  = r * sin(phi);
    float s   = 0.5f * (1 + Vh.z());
    t2        = (1 - s) * sqrt(1 - sqr(t1)) + s * t2;
    // Section 4.3: reprojection onto hemisphere
    Vector Nh = t1 * T1 + t2 * T2 + safe_sqrt(1 - sqr(t1) - sqr(t2)) * Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    Vector Ne = Vector(ax * Nh.x(), ay * Nh.y(), max(0, Nh.z())).normalized();
    return sgn * Ne;
}

/**
 * The probability density of sampling a given normal using @c
 * sampleAnisotropicGGXVNDF .
 * @param ax The roughness of the distribution in x-direction
 * @param ay The roughness of the distribution in y-direction
 * @param wh The normal to compute the PDF for (also known as half vector)
 * @param wo The outgoing direction (determines which normals are visible)
 * @returns The probability density of @c sampleGGXVNDF sampling @c wh .
 */
inline float pdfAnisotropicGGXVNDF(float ax, float ay, const Vector &normal,
                                   const Vector &wo) {
    return microfacet::evaluateAnisotropicGGX(ax, ay, normal) *
           microfacet::anisotropicSmithG1(ax, ay, normal, wo) *
           abs(normal.dot(wo)) / Frame::absCosTheta(wo);
}

//
//
// MARK: - other microfacet distributions below (not needed for the core
// assignments) feel free to use these if you want to implement clearcoat layers
// in your principled bsdf
//
//

// Note: The GTR1 functions are often paired with the smith G1 shadowing
// function. It's not a physically correct match, but artists apparently like
// the look.

/**
 * Evaluates the isotropic GTR1 normal distribution function.
 * @param alpha The roughness of the distribution
 * @param wh The normal to be evaluated (also known as half vector)
 * @see "Diffuse Reflection of Light from a Matt Surface" [Berry 1923]
 * @see "Physically Based Shading at Disney" [Burley 2012]
 */
inline float evaluateGTR1(float alpha, const Vector &wh) {
    float nDotH = Frame::cosTheta(wh);
    float a2    = sqr(alpha);
    float t     = 1 + (a2 - 1) * sqr(nDotH);
    return (a2 - 1) / (Pi * log(a2) * t);
}

/**
 * Samples the isotropic GTR1 normal distribution function.
 * @param alpha The roughness of the distribution
 * @param rnd A random point to steer sampling (typically @c prng.next2D() )
 * @return A microfacet normal that will always lie in the upper hemisphere.
 * @note The PDF of @c wh is given by:
 *   @code cosTheta(wh) * D(wh) @endcode
 */
inline Vector sampleGTR1(float alpha, const Point2 &rnd) {
    float a2 = sqr(alpha);

    float cosTheta = safe_sqrt((1 - pow(a2, 1 - rnd.x())) / (1 - a2));
    float sinTheta = safe_sqrt(1 - (cosTheta * cosTheta));
    float phi      = 2 * Pi * rnd.y();
    float sinPhi   = sin(phi);
    float cosPhi   = cos(phi);

    return { sinTheta * cosPhi, sinTheta * sinPhi, cosTheta };
}

} // namespace lightwave::microfacet
