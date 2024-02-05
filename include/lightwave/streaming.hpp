/**
 * @file streaming.hpp
 * @brief Contains the Streaming class, which is used to preview live rendering progress by sending it to the "tev" image viewer.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/math.hpp>

#include <vector>
#include <string>

namespace lightwave {

/// @brief A connection to the "tev" image viewer than can be used to send updates to images in real-time.
class Streaming {
    class Stream;
    struct UpdateThread;

    const std::vector<std::string> m_channels = { "r", "g", "b" };
    const Image &m_image;
    std::mutex m_mutex;
    float m_normalization = 1;

    std::unique_ptr<Stream> m_stream;
    std::unique_ptr<UpdateThread> m_updater;

public:
    Streaming(const Image &image);

    /// @brief Sends a given block of image data (e.g., when a tile has finished rendering).
    void updateBlock(const Bounds2i &block);
    /// @brief Sends the entire image at once.
    void update();

    /// @brief Starts regular updating of the image as background task (e.g., when using a progressive rendering algorithm).
    void startRegularUpdates();
    /// @brief Stops regular updating of the image.
    void stopRegularUpdates();
    /// @brief Requests that the image should be multiplied by a given scalar value before being sent.
    void normalize(float v) { m_normalization = v; }

    ~Streaming();
};

}
