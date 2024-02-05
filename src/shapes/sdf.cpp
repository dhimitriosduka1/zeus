#include <lightwave.hpp>

namespace lightwave {

/// @brief A raymarched shape defined by the SDF (getDistance)
class SDF : public Shape {
    static inline void populate(SurfaceEvent &surf, const Vector& normal, const Point& position) {
        surf.frame.normal = normal;
        surf.position = position;
        // This basis guarantees we get the normals correct, but the tangent space is less usable
        buildOrthonormalBasis(surf.frame.normal, surf.frame.tangent, surf.frame.bitangent);

        surf.pdf = 0.0f;
    }

public:
    SDF(const Properties &properties) {
        const std::string shape = properties.get<std::string>("shape");
        m_round = properties.get<float>("round", 0.0f);

        if (shape == "sphere") {
            m_getDistance = &SDF::getDistanceSphere;
        } else if (shape == "box") {
            m_getDistance = &SDF::getDistanceBox;
        } else if (shape == "mandelbulb") {
            m_getDistance = &SDF::getDistanceMandelbulb;
        } else {
            lightwave_throw("invalid SDF shape %s!", shape);
        }
    }

    bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
        const int MAX_STEPS = 500;
        const float MAX_DISTANCE = 100;

        float totalDist = 0;
        for (int i = 0; i < MAX_STEPS; i++)
        {
            Point pt = ray(totalDist);
            float dist = (this->*m_getDistance) (Vector(pt));

            if (abs(dist) < Epsilon) {
                its.t = totalDist + dist;
                Point p = ray(its.t);
                populate(its, getNormal(Vector(p)), p);
                return true;
            }

            totalDist += dist;

            if (totalDist > MAX_DISTANCE) {
                return false;
            }
        }

        return false;
    }

    Bounds getBoundingBox() const override {
        return {Point{-1, -1, -1}, Point{+1, +1, +1}};
    }

    Point getCentroid() const override {
        return Point(0);
    }

    AreaSample sampleArea(Sampler &rng) const override {
    }

    std::string toString() const override {
        return "SDF[]";
    }

    /// @brief SDF for a rounded box
    float getDistanceBox(const Vector& point) const {
        const Vector b = Vector(0.5);
        Vector q = Vector(abs(point.x()), abs(point.y()), abs(point.z())) - b;
        return elementwiseMax(q, Vector(0, 0, 0)).length() + min(max(q.x(),max(q.y(),q.z())), 0.0f) - m_round;
    }

    float getDistanceSphere(const Vector& point) const {
        const float r = 1.0f;
        return ((point).length() - r);
    }

    /// @brief Get a distance to a Mandelbulb fractal
    float getDistanceMandelbulb(const Vector& point) const {
        Vector z = point;
        const float Power = 3.0f + m_round;
        const int STEPS = 18;
        float dr = 1.0;
        float r = 0.0;
        for (int i = 0; i < STEPS; i++) {
            r = z.length();

            if (r > 4.0) break;
            
            // Convert to polar coordinates
            float theta = acos(z.z() / r);
            float phi = atan2(z.y(), z.x());
            dr = pow(r, Power - 1.0f) * Power * dr + 1.0f;
            
            // scale and rotate the point
            float zr = pow(r, Power);
            theta = theta * Power;
            phi = phi * Power;
            
            // Convert back to cartesian coordinates
            z = zr * Vector(sin(theta)*cos(phi), sin(phi)*sin(theta), cos(theta));
            z += point;
        }

        return 0.5 * log(r) * r / dr;
    }

    /// @brief Get an approximate normal by sampling the SDF gradient
    Vector getNormal(const Vector& point, const float step = Epsilon) const {
        // Get the normal gradient around point
        return Vector(
            (this->*m_getDistance)(point + Vector(step, 0, 0)) - (this->*m_getDistance)(point - Vector(step, 0, 0)),
            (this->*m_getDistance)(point + Vector(0, step, 0)) - (this->*m_getDistance)(point - Vector(0, step, 0)),
            (this->*m_getDistance)(point + Vector(0, 0, step)) - (this->*m_getDistance)(point - Vector(0, 0, step))
        ).normalized();
    }
protected:
    float (SDF::*m_getDistance) (const Vector&) const;
    float m_round;
};

}

REGISTER_SHAPE(SDF, "sdf")
