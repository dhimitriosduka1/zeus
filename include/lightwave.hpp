/**
 * @file lightwave.hpp
 * @brief Include this umbrella header if you do not feel like having dozens of
 * includes in each file.
 * @warning This comes at the price of slightly increased compile times.
 */

// MARK: - core
#include <lightwave/color.hpp>
#include <lightwave/core.hpp>
#include <lightwave/logger.hpp>
#include <lightwave/math.hpp>
#include <lightwave/properties.hpp>
#include <lightwave/registry.hpp>

// MARK: - utilities
#include <lightwave/iterators.hpp>
#include <lightwave/parallel.hpp>
#include <lightwave/streaming.hpp>
#include <lightwave/warp.hpp>

// MARK: - objects
#include <lightwave/bsdf.hpp>
#include <lightwave/camera.hpp>
#include <lightwave/emission.hpp>
#include <lightwave/image.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/integrator.hpp>
#include <lightwave/light.hpp>
#include <lightwave/postprocess.hpp>
#include <lightwave/sampler.hpp>
#include <lightwave/shape.hpp>
#include <lightwave/test.hpp>
#include <lightwave/texture.hpp>
#include <lightwave/volume.hpp>

// MARK: - scene
#include <lightwave/scene.hpp>
