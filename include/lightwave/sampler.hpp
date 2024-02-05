/**
 * @file sampler.hpp
 * @brief Contains the Sampler interface.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/color.hpp>
#include <lightwave/math.hpp>
#include <lightwave/properties.hpp>

namespace lightwave {

/**
 * @brief Samplers are pseudo-random number generators that steer sampling decisions.
 * While all samplers are expected to produce uniformly distributed random numbers, they can still differ
 * in how they correlate samples, e.g., some samplers try to cover the space of random numbers more evenly by
 * by avoiding placing samples close to previous samples.
 */
class Sampler : public Object {
protected:
    /// @brief The number of samples that should be taken per pixel.
    int m_samplesPerPixel;

public:
    Sampler() : m_samplesPerPixel(0) {}
    Sampler(const Properties &properties) {
        m_samplesPerPixel = properties.get<int>("count", 1);
    }

    /// @brief Generates a single random number in the interval [0,1). 
    virtual float next() = 0;
    /// @brief Generates a random point in the unit square [0,1)^2. 
    virtual Point2 next2D() {
        return { next(), next() };
    }

    /**
     * @brief Initiates a random number sequence characterized by the given number.
     * @note When identical samplers are given the same seed, they are expected to produce the same sequence
     * of random numbers. For different seeds, they are expected to give different random sequences.
     */
    virtual void seed(int index) = 0;
    /**
     * @brief Initiates a random number sequence characterized for the given pixel and sample per pixel index.
     * @note When identical samplers are given the same seed, they are expected to produce the same sequence
     * of random numbers. For different seeds, they are expected to give different random sequences.
     */
    virtual void seed(const Point2i &pixel, int sampleIndex) = 0;
    /// @brief Returns an identical copy of the sampler, e.g., for use in different threads. 
    virtual ref<Sampler> clone() const = 0;

    /// @brief Returns the number of samples that should be taken per pixel. 
    int samplesPerPixel() const { return m_samplesPerPixel; }
};

}
