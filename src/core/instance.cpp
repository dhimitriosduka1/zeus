#include <lightwave.hpp>
#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

void Instance::transformFrame(SurfaceEvent &surf) const {
    surf.position = m_transform->apply(surf.position);

    // Applying T transformation to tangent and bitangent
    surf.frame.tangent = m_transform->apply(surf.frame.tangent);
    surf.frame.bitangent = m_transform->apply(surf.frame.bitangent);

    // Account for the change in surface after applying the transformation
    surf.pdf /= surf.frame.tangent.cross(surf.frame.bitangent).length();

    surf.frame.tangent = surf.frame.tangent.normalized();
    surf.frame.bitangent = surf.frame.bitangent.normalized();

    // Re-project frame
    if (m_flipNormal) { surf.frame.bitangent = -surf.frame.bitangent; }
    surf.frame.bitangent = surf.frame.bitangent.project(surf.frame.tangent).normalized();

    // We do not need to normalize the normal since the cross product of two unit vectors yields a unit vector
    surf.frame.normal = surf.frame.tangent.cross(surf.frame.bitangent);
    surf.frame.bitangent = surf.frame.normal.cross(surf.frame.tangent);

    if (m_normal) {
        // Unpack the normal
        const Color normal = 2 * m_normal->evaluate(surf.uv) - Color(1);
        // Rebuild the frame using the new normal
        surf.frame = Frame(
            surf.frame.toWorld(Vector(normal.r(), normal.g(), normal.b()).normalized()
        ));
    }
}

bool Instance::intersect(const Ray &worldRay, Intersection &its, Sampler &rng) const {
    if (!m_transform) {
        // fast path, if no transform is needed
        Ray localRay = worldRay;
        if (m_shape->intersect(localRay, its, rng)) {
            if (isTransparent(its.uv, rng)) {
                return false;
            }
            its.instance = this;
            return true;
        } else {
            return false;
        }
    }

    const Intersection oldIts = its;
    Ray localRay = m_transform->inverse(worldRay);
    // Convert its.t to localspace
    const float scaleFactor = localRay.direction.length();
    its.t *= scaleFactor;
    localRay = localRay.normalized();

    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        if (isTransparent(its.uv, rng)) {
            its = oldIts;
            return false;
        }

        if (m_volume) {
            // Possibly sample the point in volume instead
            // We do all calculations in local space for convenience/efficiency
            const float rayEpsilonLocal = 0.001f * scaleFactor; // Minimum surface thickness (local space)
            float itsT = its.t; // Intersection distance (local space)

            Ray nextLocalRay = localRay;
            nextLocalRay.origin = localRay(itsT + rayEpsilonLocal);
            float insideT = its.t; // Thickness/distance inside the volume (local space)
            Intersection nextIts = oldIts;

            // Shoot another ray slightly further away from the intersection point
            // bool isInside = its.frame.normal.dot(localRay.direction) < 0; // doesn't work
            
            if (m_shape->intersect(nextLocalRay, nextIts, rng)) {
                // If we intersect again, we've "pierced" the surface (original ray was outside)
                // (localRay) ---> | (nextLocalRay) --insideT--> |
                insideT = nextIts.t + rayEpsilonLocal;
            } else {
                // No intersection, which likely means that first ray came from inside already
                // | (localRay) --insideT--> |
                // Then insideT is just equal to our original hit distance
                itsT = 0;
            }

            float distance = 0.f;
            if (m_volume->sampleDistance(itsT, insideT, scaleFactor, localRay, rng, distance) && distance < oldIts.t) {
                its.instance = this;
                its.frame.normal = squareToUniformSphere(rng.next2D());
                buildOrthonormalBasis(its.frame.normal, its.frame.tangent, its.frame.bitangent);
                // Convert nextT back to world space
                its.t = distance;
                its.position = worldRay(its.t);
                return true;
            } else {
                its = oldIts;
                return false;
            }
        }

        its.instance = this;
        // Convert its.t back to worldspace
        its.t /= scaleFactor;
        transformFrame(its);
        return true;
    } else {
        its = oldIts;
        return false;
    }
}

bool Instance::isTransparent(const Point2 &uv, Sampler &rng) const {
    if (!m_alpha) { return false; }

    float alpha = m_alpha->scalar(uv);

    return rng.next() > alpha;
}

Bounds Instance::getBoundingBox() const {
    if (!m_transform) {
        // fast path
        return m_shape->getBoundingBox();
    }

    const Bounds untransformedAABB = m_shape->getBoundingBox();
    if (untransformedAABB.isUnbounded()) {
        return Bounds::full();
    }

    Bounds result;
    for (int point = 0; point < 8; point++) {
        Point p = untransformedAABB.min();
        for (int dim = 0; dim < p.Dimension; dim++) {
            if ((point >> dim) & 1) {
                p[dim] = untransformedAABB.max()[dim];
            }
        }
        p = m_transform->apply(p);
        result.extend(p);
    }
    return result;
}

Point Instance::getCentroid() const {
    if (!m_transform) {
        // fast path
        return m_shape->getCentroid();
    }

    return m_transform->apply(m_shape->getCentroid());
}

AreaSample Instance::sampleArea(Sampler &rng) const {
    AreaSample sample = m_shape->sampleArea(rng);
    transformFrame(sample);
    return sample;
}

}

REGISTER_CLASS(Instance, "instance", "default")
