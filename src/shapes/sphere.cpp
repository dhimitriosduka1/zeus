#include <lightwave.hpp>

namespace lightwave {

/// @brief A sphere with a given radius
class Sphere : public Shape {
    static inline void populate(SurfaceEvent &surf, const Point &position) {
        surf.frame.normal = ((Vector) position).normalized();
        surf.position = surf.frame.normal;
        // Build an arbitrary orthonormal basis for the normal
        buildOrthonormalBasis(surf.frame.normal, surf.frame.tangent, surf.frame.bitangent);

        // What happens if the sampled point is behind the sphere?
        surf.uv = directionToSphereUV(surf.frame.normal);

        surf.pdf = 0.25f * InvPi;
    }

public:
    Sphere(const Properties &properties) {
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        const float EPSILON = 3e-5f;
        const float r = 1.f;
        const Vector oc = (Vector) ray.origin;
        const float a = ray.direction.lengthSquared();
        const float b = 2.f * ray.direction.dot(oc);
        const float c = oc.lengthSquared() - r * r;
        float d = b * b - 4.f * a * c;

        if (d < EPSILON) {
            return false;
        }

        d = sqrt(d);

        const float t1 = (-b - d) / a * 0.5f;
        const float t2 = (-b + d) / a * 0.5f;

        float t;
        if (t1 < EPSILON && t2 < EPSILON)
        {
            // Both negative, did not hit the sphere
            return false;
        } else if (t1 < EPSILON) {
            // Inside sphere, use t2 (positive)
            t = t2;
        } else {
            // Inside sphere, use t1 (positive)
            t = t1;
        }

        if (t > its.t) return false;
        its.t = t;

        Point position = ray(its.t);
        populate(its, position);

        return true;
    }

    Bounds getBoundingBox() const override {
        return {Point{-1, -1, -1}, Point{+1, +1, +1}};
    }

    Point getCentroid() const override {
        return Point(0);
    }

    AreaSample sampleArea(Sampler &rng) const override {
        const Vector random = squareToUniformSphere(rng.next2D());
        const Point position = Point(random);

        AreaSample areaSample;
        populate(areaSample, position);

        return areaSample;
    }

    std::string toString() const override {
        return "Sphere[]";
    }
};

}

REGISTER_SHAPE(Sphere, "sphere")
