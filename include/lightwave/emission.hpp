/**
 * @file emission.hpp
 * @brief Contains the Emission interface and related structures.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/color.hpp>
#include <lightwave/math.hpp>

namespace lightwave {

/// @brief The result of evaluating an @ref Emission .
struct EmissionEval {
    /// @brief The color of the emission, not including any cosine term.
    Color value;

    static EmissionEval invalid() {
        return {
            .value = Color(0),
        };
    }
};

/// @brief An Emission, representing the light distribution of emissive surfaces.
class Emission : public Object {
public:
    Emission() {}

    /**
     * @brief Evaluates the emission for a given direction in local coordinates (i.e., the normal is assumed to be [0,0,1]).
     * 
     * @param uv The texture coordinates of the surface.
     * @param wo The outgoing direction light is emitted in, pointing away from the surface, in local coordinates.
     */
    virtual EmissionEval evaluate(const Point2 &uv, const Vector &wo) const = 0;
};

}
