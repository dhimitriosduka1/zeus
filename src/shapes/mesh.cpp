#include <lightwave.hpp>
#include <tuple>

#include "../core/plyparser.hpp"
#include "accel.hpp"

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of triangles, which share an index and vertex buffer.
 * Since individual triangles are rarely needed (and would pose an excessive amount of overhead), collections of
 * triangles are combined in a single shape.
 */
class TriangleMesh : public AccelerationStructure {
    /**
     * @brief The index buffer of the triangles.
     * The n-th element corresponds to the n-th triangle, and each component of the element corresponds to one
     * vertex index (into @c m_vertices ) of the triangle.
     * This list will always contain as many elements as there are triangles.
     */
    std::vector<Vector3i> m_triangles;
    /**
     * @brief The vertex buffer of the triangles, indexed by m_triangles.
     * Note that multiple triangles can share vertices, hence there can also be fewer than @code 3 * numTriangles @endcode
     * vertices.
     */
    std::vector<Vertex> m_vertices;
    /// @brief The file this mesh was loaded from, for logging and debugging purposes.
    std::filesystem::path m_originalPath;
    /// @brief Whether to interpolate the normals from m_vertices, or report the geometric normal instead.
    bool m_smoothNormals;

    static inline void populate(SurfaceEvent &surf) {
        buildOrthonormalBasis(surf.frame.normal, surf.frame.tangent, surf.frame.bitangent);
        surf.pdf = 0.0f;
    }

protected:
    int numberOfPrimitives() const override {
        return int(m_triangles.size());
    }

    // Paper reference https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its, Sampler &rng) const override {
        auto [v0, v1, v2] = getTriangle(primitiveIndex);

        const Vector e1_vec = v1.position - v0.position;
        const Vector e2_vec = v2.position - v0.position;

        const Vector p_vec = ray.direction.cross(e2_vec);
        const float determinant = p_vec.dot(e1_vec);

        if (abs(determinant) < 1e-8) return false;

        const float inverse_determinant = 1.0f / determinant;
        const Vector t_vec =  ray.origin - v0.position;

        const float bary_u = p_vec.dot(t_vec) * inverse_determinant;

        // Testing one of the barycentric coordinates
        if (bary_u < 0.0f || bary_u > 1.0) return false;

        const Vector q_vec = t_vec.cross(e1_vec);

        const float bary_v = q_vec.dot(ray.direction) * inverse_determinant;

        if (bary_v < 0.0 || bary_u + bary_v > 1.0) return false;

        const float t = q_vec.dot(e2_vec) * inverse_determinant;

        if (t < Epsilon) return false;

        if (t > its.t) return false;
        its.t = t;

        its.uv = interpolateBarycentric(
                Vector2(bary_u, bary_v),
                v0.texcoords,
                v1.texcoords,
                v2.texcoords
        );

        if (m_smoothNormals) {
            its.frame.normal = interpolateBarycentric(
                    Vector2(bary_u, bary_v),
                    v0.normal,
                    v1.normal,
                    v2.normal
            ).normalized();
        } else {
            its.frame.normal = e1_vec.cross(e2_vec).normalized();
        }

        its.position = ray(its.t);
        populate(its);
        return true;
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        auto [a, b, c] = getTriangle(primitiveIndex);

        return {
            elementwiseMin(a.position, elementwiseMin(b.position, c.position)),
            elementwiseMax(a.position, elementwiseMax(b.position, c.position))
        };
    }

    Point getCentroid(int primitiveIndex) const override {
        const Vector3i triangle = m_triangles[primitiveIndex];
        return {
            (
                Vector(m_vertices[triangle.x()].position) +
                Vector(m_vertices[triangle.y()].position) +
                Vector(m_vertices[triangle.z()].position)
            ) / 3.0f
        };
    }

    std::tuple<Vertex, Vertex, Vertex> getTriangle(int primitiveIndex) const
    {
        const Vector3i triangle = m_triangles[primitiveIndex];
        return {
            m_vertices[triangle.x()],
            m_vertices[triangle.y()],
            m_vertices[triangle.z()]
        };
    }

public:
    TriangleMesh(const Properties &properties) {
        m_originalPath = properties.get<std::filesystem::path>("filename");
        m_smoothNormals = properties.get<bool>("smooth", true);
        readPLY(m_originalPath.string(), m_triangles, m_vertices);
        logger(EInfo, "loaded ply with %d triangles, %d vertices",
            m_triangles.size(),
            m_vertices.size()
        );
        buildAccelerationStructure();
    }

    AreaSample sampleArea(Sampler &rng) const override {
        // only implement this if you need triangle mesh area light sampling for your rendering competition
        NOT_IMPLEMENTED
    }

    std::string toString() const override {
        return tfm::format(
            "Mesh[\n"
            "  vertices = %d,\n"
            "  triangles = %d,\n"
            "  filename = \"%s\"\n"
            "]",
            m_vertices.size(),
            m_triangles.size(),
            m_originalPath.generic_string()
        );
    }
};

}

REGISTER_SHAPE(TriangleMesh, "mesh")
