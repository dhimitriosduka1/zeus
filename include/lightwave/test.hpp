/**
 * @file test.hpp
 * @brief Contains the Test interface, which are executable objects that check whether the renderer produces the right results.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/color.hpp>
#include <lightwave/math.hpp>
#include <lightwave/properties.hpp>

namespace lightwave {

/// @brief An executable object (typically placed at the root of your scene), that validates the different aspects of your renderer.
class Test : public Executable {
public:
    Test() {}
};

}
