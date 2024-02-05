#pragma once

#include <lightwave/core.hpp>
#include <lightwave/math.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

/// @brief Models a grid of volumetric densities
class Volume : public Object {
public:
    /**
     * @brief Returns the density at a given point
     */
    virtual float evaluate(const Point &pos) const = 0;
    virtual float getMaxDensity() const = 0;
    bool sampleDistance(float itsT, float insideT, float scaleFactor,
        const Ray &localRay, Sampler &rng, float &distance) const
    {
        const int TRACKING_STEPS = 1024;
        const float maxDensity = getMaxDensity();
        for (int i = 0; i < TRACKING_STEPS; i++) {
            const float sigmaT = maxDensity;

            const float e = rng.next();
            // Sample random distance in local space
            const float sampleT = -log(1 - e) / sigmaT * scaleFactor;

            if (sampleT < insideT) {
                // Sampled a point inside the volume
                const float nextT = itsT + sampleT; // Distance to sampled point along localRay 
                const Point localPoint = localRay(nextT);
                const float realProbability = evaluate(localPoint) / maxDensity;

                if (rng.next() < realProbability) {
                    // Convert nextT back to world space
                    distance = nextT / scaleFactor;
                    return true;
                } else {
                    // We hit a null particle, continue tracking
                    itsT = nextT; // itsT is increased by sampleT
                    insideT -= sampleT; // Less inside distance left (we stepped by sampleT)
                    continue;
                }
            } else {
                // Ignore intersection, sampeld outside of volume
                return false;
            }
        }

        // Not sure what to do here, ran out of tracking steps, volume too dense
        distance = itsT / scaleFactor;
        return true;
    }
};

}
