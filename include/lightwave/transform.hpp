/**
 * @file transform.hpp
 * @brief Contains the Transform interface.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/color.hpp>
#include <lightwave/math.hpp>

namespace lightwave {

/**
 * @brief Transfers points or vectors from one coordinate system to another.
 * @note This is an interface to allow time-dependent transforms (e.g., motion blur), or non-linear transforms (be creative!)
 */
class Transform : public Object {
protected:
    Matrix4x4 m_transform = Matrix4x4::identity();
    Matrix4x4 m_inverse = Matrix4x4::identity();

public:
    Transform() {}
    Transform(const Properties &) {}

    /// @brief Transforms the given point.
    Point apply(const Point &point) const {
        const Vector4 result = m_transform * Vector4(Vector(point), 1);
        return Vector(result.x(), result.y(), result.z()) / result.w();
    }

    /// @brief Transforms the given vector.
    Vector apply(const Vector &vector) const {
        const Vector4 result = m_transform * Vector4(vector, 0);
        return Vector(result.x(), result.y(), result.z());
    }

    /**
     * @brief Transforms the given ray.
     * @warning The ray direction will not be normalized, as its transformed length
     * is typically useful for other tasks (e.g., instancing).
     */
    Ray apply(const Ray &ray) const {
        Ray result(ray);
        result.origin = apply(ray.origin);
        result.direction = apply(ray.direction);
        return result;
    }

    /// @brief Applies the inverse transform to the given point.
    Point inverse(const Point &point) const {
        const Vector4 result = m_inverse * Vector4(Vector(point), 1);
        return Vector(result.x(), result.y(), result.z()) / result.w();
    }

    /// @brief Applies the inverse transform to the given vector.
    Vector inverse(const Vector &vector) const {
        const Vector4 result = m_inverse * Vector4(vector, 0);
        return Vector(result.x(), result.y(), result.z());
    }

    /**
     * @brief Transforms the given ray.
     * @warning The ray direction will not be normalized.
     */
    Ray inverse(const Ray &ray) const {
        Ray result(ray);
        result.origin = inverse(ray.origin);
        result.direction = inverse(ray.direction);
        return result;
    }


    /// @brief Transforms the given normal vector returning a new normalized vector
    Vector inverseNormal(const Vector &vector) const {
        const Vector4 result = m_inverse.transpose() * Vector4(vector, 0.0f);
        return Vector(result.x(), result.y(), result.z()).normalized();
    }

    /// @brief Appends a matrix in homogeneous coordinates to this transform.
    void matrix(const Matrix4x4 &value) {
        m_transform = value * m_transform;
        if (auto inv = invert(value)) {
            m_inverse = m_inverse * *inv;
        } else {
            lightwave_throw("transform is not invertible");
        }
    }

    /// @brief Appends a translation to this transform.
    void translate(const Vector &translation) {
        m_transform = Matrix4x4 {
            1, 0, 0, translation.x(),
            0, 1, 0, translation.y(),
            0, 0, 1, translation.z(),
            0, 0, 0, 1
        } * m_transform;
        m_inverse = m_inverse * Matrix4x4 {
            1, 0, 0, -translation.x(),
            0, 1, 0, -translation.y(),
            0, 0, 1, -translation.z(),
            0, 0, 0, 1
        };
    }

    /// @brief Appends a (potentially non-uniform) scaling to this transform.
    void scale(const Vector &scaling) {
        if (scaling.product() == 0) {
            lightwave_throw("scaling is not invertible");
        }

        m_transform = Matrix4x4 {
            scaling.x(), 0, 0, 0,
            0, scaling.y(), 0, 0,
            0, 0, scaling.z(), 0,
            0, 0, 0, 1
        } * m_transform;
        m_inverse = m_inverse * Matrix4x4 {
            1 / scaling.x(), 0, 0, 0,
            0, 1 / scaling.y(), 0, 0,
            0, 0, 1 / scaling.z(), 0,
            0, 0, 0, 1
        };
    }

    /// @brief Appends a rotation around the given axis to this transform.
    void rotate(const Vector &axis, float angle) {
        const auto u = axis.normalized();
        const float cos = std::cos(angle);
        const float sin = std::sin(angle);

        auto rotation = Matrix4x4::identity();
        for (int row = 0; row < axis.Dimension; row++) {
            for (int column = 0; column < axis.Dimension; column++) {
                rotation(row, column) = (1 - cos) * u[row] * u[column] + (
                    row == column ?
                        cos :
                        (row == (column + 1) % axis.Dimension ? +1 : -1) * sin * u[((axis.Dimension - row) + (axis.Dimension - column)) % axis.Dimension]
                );
            }
        }

        m_transform = rotation * m_transform;
        m_inverse = m_inverse * rotation.transpose();
    }

    /**
     * @brief Appends a "lookat" operation to this transform, which is useful to aim cameras or light sources at other objects.
     * The z-axis will be re-oriented to be aligned with @code target - origin @endcode , and the y-axis will be in the plane
     * that the @c up vector lies in.
     */
    void lookat(const Vector &origin, const Vector &target, const Vector &up) {
        const auto direction = (target - origin).normalized();
        if (up.cross(direction).isZero()) {
            lightwave_throw("lookat: direction (%s) and up vector (%s) must not be colinear", direction, up);
        }
        const auto left = up.cross(direction).normalized();
        const auto orthogonalUp = direction.cross(left).normalized();

        Matrix4x4 matrix;
        matrix.setColumn(0, Vector4(left, 0));
        matrix.setColumn(1, Vector4(orthogonalUp, 0));
        matrix.setColumn(2, Vector4(direction, 0));
        matrix.setColumn(3, Vector4(origin, 1));

        m_transform = matrix * m_transform;

        matrix.setColumn(3, Vector4(0, 0, 0, 1));
        matrix = matrix.transpose();
        matrix.setColumn(3, Vector4(-origin, 1));

        m_inverse = m_inverse * matrix;
    }

    /// @brief Returns the determinant of this transformation.
    float determinant() const {
        return m_transform.submatrix<3, 3>(0, 0).determinant();
    }

    std::string toString() const override {
        return tfm::format(
            "Transform[\n"
            "  matrix = %s,\n"
            "  inverse = %s,\n"
            "]",
            indent(m_transform),
            indent(m_inverse)
        );
    }
};

}
