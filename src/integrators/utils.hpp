#pragma once

#include <lightwave.hpp>

namespace lightwave::integrators {

    inline Color resolveNEEContribution(const ref<Scene> &scene, Sampler &rng, const Intersection &its, const Color &weight) {
        auto [ light, probability ] = scene->sampleLight(rng);
        DirectLightSample lightSample = light->sampleDirect(its.position, rng);
        if (light->canBeIntersected()) {
            // Ignore intersectable lights for now
            return Color(0.f);
        } 
        
        const Ray lightRay = Ray(its.position, lightSample.wi);
        // Check if we can reach the light
        if (!scene->intersect(lightRay, lightSample.distance, rng)) {
            return weight * its.evaluateBsdf(lightSample.wi).value * lightSample.weight / probability;
        }
        return Color(0.f);
    }
}