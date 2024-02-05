#include <lightwave/core.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/integrator.hpp>
#include <lightwave/shape.hpp>
#include <lightwave/camera.hpp>
#include <lightwave/light.hpp>

namespace lightwave {

Scene::Scene(const Properties &properties) {
    m_camera = properties.getChild<Camera>();
    m_background = properties.getOptionalChild<BackgroundLight>();
    m_lights = properties.getChildren<Light>();
    
    const std::vector<ref<Shape>> entities = properties.getChildren<Shape>();
    if (entities.size() == 1) {
        m_shape = entities[0];
    } else {
        m_shape = std::static_pointer_cast<Shape>(Registry::create("shape", "group", properties));
    }

    m_shape->markAsVisible();
}

std::string Scene::toString() const {
    return tfm::format(
        "Scene[\n"
        "  camera = %s,\n"
        "  shape = %s,\n"
        "]",
        indent(m_camera),
        indent(m_shape)
    );
}

Intersection Scene::intersect(const Ray &ray, Sampler &rng) const {
    Intersection its(-ray.direction);
    m_shape->intersect(ray, its, rng);
    return its;
}

bool Scene::intersect(const Ray &ray, float tMax, Sampler &rng) const {
    Intersection its(-ray.direction, tMax * (1 - Epsilon));
    return m_shape->intersect(ray, its, rng);
}

BackgroundLightEval Scene::evaluateBackground(const Vector &direction) const {
    if (!m_background) return {
        .value = Color(0),
    };
    return m_background->evaluate(direction);
}

LightSample Scene::sampleLight(Sampler &rng) const {
    int lightIndex = int(rng.next() * m_lights.size());
    lightIndex = std::min(lightIndex, int(m_lights.size()) - 1);
    return {
        .light = m_lights[lightIndex].get(),
        .probability = float(1) / m_lights.size(),
    };
}

float Scene::lightSelectionProbability(const Light *light) const {
    return float(1) / m_lights.size();
}

Bounds Scene::getBoundingBox() const {
    return m_shape->getBoundingBox();
}

}

REGISTER_CLASS(Scene, "scene", "default")
