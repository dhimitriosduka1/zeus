/**
 * @file image.hpp
 * @brief Contains the Image class, used to represent, load, store, and work
 * with images.
 */

#pragma once

#include <lightwave/color.hpp>
#include <lightwave/core.hpp>
#include <lightwave/logger.hpp>
#include <lightwave/math.hpp>
#include <lightwave/properties.hpp>

namespace lightwave {

/// @brief An image.
class Image final : public Object {
    /// @brief The resolution of this image in pixels.
    Point2i m_resolution;

    /// @brief A sequence of the pixel colors of this image.
    std::vector<Color> m_data;

    /// @brief The folder the image was loaded from or should be stored to.
    std::filesystem::path m_basePath;

    /**
     * @brief Converts a normalized position from [0,0]..[+1,+1] to a pixel
     * index [0,0]..[resolution.x-1, resolution.y-1]. Input positions outside
     * the unit square will be clamped to the edges.
     */
    Point2i pixelFromNormalized(const Point2 &normalized) const {
        return { std::clamp(int(normalized.x() * m_resolution.x()), 0,
                            m_resolution.x() - 1),
                 std::clamp(int(normalized.y() * m_resolution.y()), 0,
                            m_resolution.y() - 1) };
    }

public:
    Image() {}

    /// @brief Loads an image from a file, optionally performing an inverse sRGB
    /// transform when @c isLinearSpace is set to false.
    Image(const std::filesystem::path &path, bool isLinearSpace = false) {
        loadImage(path, isLinearSpace);
    }

    /// @brief Creates a black image with the given resolution.
    Image(const Point2i &resolution) { initialize(resolution); }

    Image(const Properties &properties) {
        if (properties.has("filename")) {
            auto path = properties.get<std::filesystem::path>("filename");
            loadImage(path, properties.get<bool>("linear", false));
            m_basePath = path.parent_path();
        } else {
            m_basePath = properties.basePath();
        }
    }

    /// @brief Sets the folder the image will be stored in if no explicit path
    /// is given.
    void setBasePath(const std::filesystem::path &basePath) {
        m_basePath = basePath;
    }

    /// @brief Copies the data and resolution from another image, but leaves all
    /// other attributes the same.
    void copy(const Image &image) {
        m_resolution = image.m_resolution;
        m_data       = image.m_data;
    }

    /**
     * @brief Loads the data and resolution from a file with a given path,
     * optionally performing an inverse sRGB transform when @c isLinearSpace is
     * set to false.
     */
    void loadImage(const std::filesystem::path &path,
                   bool isLinearSpace = false);

    /// @brief Changes the resolution and sets all pixels to black.
    void initialize(const Point2i &resolution) {
        m_resolution = resolution;
        m_data.resize(resolution.x() * resolution.y());
        std::fill(m_data.begin(), m_data.end(), Color());
    }

    /// @brief Saves the image as an EXR file at a given path.
    void saveAt(const std::filesystem::path &path) const;

    /// @brief Saves the image at its default path, given by the @ref basePath
    /// of this image and its @ref id .
    void save() const { saveAt(m_basePath / (id() + ".exr")); }

    /// @brief Multiplies the color of all pixels component-wise by a given
    /// scalar.
    void operator*=(float v) {
        for (auto &pixel : m_data)
            pixel *= v;
    }

    /**
     * @brief Returns the color at a given pixel coordinate in the range [0,0]
     * to [resolution.x - 1, resolution.y - 1].
     * @warning Pixel coordinates outside the specified range will result in
     * undefined behavior!
     */
    const Color &operator()(const Point2i &pixel) const {
        return m_data[pixel.y() * m_resolution.x() + pixel.x()];
    }
    /**
     * @brief Returns a reference to the color at a given pixel coordinate in
     * the range [0,0] to [resolution.x - 1, resolution.y - 1].
     * @warning Pixel coordinates outside the specified range will result in
     * undefined behavior!
     */
    Color &operator()(const Point2i &pixel) {
        return m_data[pixel.y() * m_resolution.x() + pixel.x()];
    }

    /**
     * @brief Returns the color at a given normalized coordinate in the range
     * [0,0] to [1,1], clamping the input coordinates to edges if they are
     * outside this interval.
     */
    const Color &operator()(const Point2 &normalized) const {
        return (*this)(pixelFromNormalized(normalized));
    }
    /**
     * @brief Returns a reference to the color at a given normalized coordinate
     * in the range [0,0] to [1,1], clamping the input coordinates to edges if
     * they are outside this interval.
     */
    Color &operator()(const Point2 &normalized) {
        return (*this)(pixelFromNormalized(normalized));
    }

    /**
     * @brief Returns the color at a given pixel coordinate in the range [0,0]
     * to [resolution.x - 1, resolution.y - 1].
     * @note This function is equivalent to the call operator, but is more
     * readable when operating on pointers to images.
     * @warning Pixel coordinates outside the specified range will result in
     * undefined behavior!
     */
    const Color &get(const Point2i &pixel) const { return (*this)(pixel); }
    /**
     * @brief Returns a reference to the color at a given pixel coordinate in
     * the range [0,0] to [resolution.x - 1, resolution.y - 1].
     * @note This function is equivalent to the call operator, but is more
     * readable when operating on pointers to images.
     * @warning Pixel coordinates outside the specified range will result in
     * undefined behavior!
     */
    Color &get(const Point2i &pixel) { return (*this)(pixel); }

    /// @brief Returns the resolution of this image in pixels.
    const Point2i &resolution() const { return m_resolution; }
    /// @brief Returns the bounding box of this image, ranging from [0,0] to
    /// [m_resolution.x, m_resolution.y].
    Bounds2i bounds() const { return { {}, Vector2i(m_resolution) }; }

    std::string toString() const override {
        if (m_basePath.empty()) {
            return tfm::format("Image[\n"
                               "  id = \"%s\",\n"
                               "]",
                               id());
        } else {
            return tfm::format("Image[\n"
                               "  id = \"%s\",\n"
                               "  filename = \"%s\",\n"
                               "]",
                               id(), m_basePath.generic_string());
        }
    }

    /// @brief Returns the number of bytes used to store a single pixel.
    int getBytesPerPixel() const { return sizeof(Color); }

    /// @brief Returns a pointer to the sequence of pixels constituting this
    /// image.
    const Color *data() const { return m_data.data(); }
    /// @brief Returns a modifiable pointer to the sequence of pixels
    /// constituting this image.
    Color *data() { return m_data.data(); }
    /// @brief Replaces m_data with data param.
    void replace(const std::vector<Color> &data) {
        m_data = data;
    }
};

} // namespace lightwave
