/**
 * @file instance.hpp
 * @brief Contains the Instance class, which binds shapes to your scene.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/shape.hpp>

namespace lightwave {

/**
 * @brief An instance represents an instantiation of a @ref Shape in your scene, and binds materials and a transformation to it.
 * Wrapping a shape like this might seem cumbersome (why not add materials or transforms to the Shape class itself instead?), but allows
 * for more flexibility and efficiency when building scenes. Most notably, you could load a complex triangle mesh (e.g., a tree) once,
 * and render an entire forest by instantiating it with different transformations, without increasing loading time or memory consumption.
 * 
 * @note Instances can be used anywhere shapes can be used, and only differ in that they augment the intersect method of the wrapped shape
 * (namely by applying an optional transform before intersecting the wrapped object, and by populating the instance field).
 */
class Instance : public Shape {
    /// @brief When an instance is wrapped within an area light object, this will reference it.
    Light *m_light;
    /// @brief The shape wrapped by the instance.
    ref<Shape> m_shape;
    /// @brief The material that the shape should be rendered with (can be null for non-reflecting objects).
    ref<Bsdf> m_bsdf;
    /// @brief The distribution of light the shape should emit (can be null for non-emissive objects).
    ref<Emission> m_emission;
    /// @brief The transformation applied to the shape, leading from object coordinates to world coordinates.
    ref<Transform> m_transform;
    /// @brief The normal texture (optional)
    ref<Texture> m_normal;
    /// @brief Flip the normal direction, used to correct for the change of handedness in case the transformation mirrors the object.
    bool m_flipNormal;
    /// @brief The alpha texture (optional)
    ref<Texture> m_alpha;
    /// @brief Tracks whether this instance has been added to the scene, i.e., could be hit by ray tracing.
    bool m_visible;
    /// @brief Volume inside the instance (optional)
    ref<Volume> m_volume;
    
    /// @brief Transforms the frame from object coordinates to world coordinates.
    inline void transformFrame(SurfaceEvent &surf) const;

public:
    Instance(const Properties &properties) 
        : m_light(nullptr) {
        m_shape = properties.getChild<Shape>();
        m_bsdf = properties.getOptionalChild<Bsdf>();
        m_emission = properties.getOptionalChild<Emission>();
        m_transform = properties.getOptionalChild<Transform>();
        m_normal = properties.get<Texture>("normal", nullptr);
        m_alpha = properties.get<Texture>("alpha", nullptr);
        m_volume = properties.getOptionalChild<Volume>();

        m_visible = false;
        
        m_flipNormal = false;
        if (m_transform && m_transform->determinant() < 0) {
            m_flipNormal = !m_flipNormal;
        }
    }

    /// @brief Returns the material that the shape should be rendered with (can be null for non-reflecting objects).
    Bsdf *bsdf() const { return m_bsdf.get(); }
    /// @brief Returns the distribution of light the shape should emit (can be null for non-emissive objects).
    Emission *emission() const { return m_emission.get(); }
    /// @brief Returns the light object that contains this instance (or null if this instance is not part of any area light).
    Light *light() const { return m_light; }

    /// @brief Returns whether this instance has been added to the scene, i.e., could be hit by ray tracing.
    bool isVisible() const { return m_visible; }
    /// @brief Sets the visible flag of this instance to true.
    void markAsVisible() override {
        m_visible = true;
    }

    /// @brief Sets the parent light object that contains this instance.
    void setLight(Light *light) {
        if (m_light) {
            lightwave_throw("instances can only have one light associated with them, %s has multiple!", indent(this));
        }
        m_light = light;
    }

    /**
     * @brief Intersects the instance with a given ray in world coordinates.
     * @param ray The ray to intersect the shape with in world coordinates.
     * @param its Contains the intersection if one occured, otherwise left unchanged.
     * @param rng A random number generator used to steer sampling decisions (e.g., alpha masking or volume intersections).
     * @return @c true if an intersection was found.
     */
    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override;
    /// @brief Returns the bounding box of the instance in world coordinates. 
    Bounds getBoundingBox() const override;
    /// @brief Returns the centroid of the instance in world coordinates. 
    Point getCentroid() const override;
    /**
     * @brief Samples a point in world coordinates on the surface of this instance.
     * @param rng A random number generator used to steer sampling decisions.
     */
    AreaSample sampleArea(Sampler &rng) const override;

    /// @brief Returns a textual representation of this image.
    std::string toString() const override {
        return tfm::format(
            "Instance[\n"
            "  shape = %s,\n"
            "  bsdf = %s,\n"
            "  emission = %s,\n"
            "  transform = %s,\n"
            "]",
            indent(m_shape),
            indent(m_bsdf),
            indent(m_emission),
            indent(m_transform)
        );
    }
private:
    bool isTransparent(const Point2 &uv, Sampler &rng) const;
};

}
