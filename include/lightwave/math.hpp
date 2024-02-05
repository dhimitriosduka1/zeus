/**
 * @file math.hpp
 * @brief Contains all geometrical constructs (Points, Matrices, Rays, etc), as well as commonly used mathematical constants and functions.
 */

#pragma once

#include <lightwave/core.hpp>
#include <lightwave/logger.hpp>

#include <cmath>
#include <algorithm>
#include <array>
#include <optional>

namespace lightwave {

// MARK: - useful constants

/// @brief Pi
static constexpr float Pi     = 3.14159265358979323846f;
/// @brief 1 / Pi
static constexpr float InvPi  = 0.31830988618379067154f;
/// @brief 1 / (2 * Pi)
static constexpr float Inv2Pi = 0.15915494309189533577f;
/// @brief 1 / (4 * Pi)
static constexpr float Inv4Pi = 0.07957747154594766788f;
/// @brief Pi / 2
static constexpr float Pi2    = 1.57079632679489661923f;
/// @brief Pi / 4
static constexpr float Pi4    = 0.78539816339744830961f;
/// @brief sqrt(2)
static constexpr float Sqrt2  = 1.41421356237309504880f;

/// @brief Multiply by this constant to convert degrees into radians.
static constexpr float Deg2Rad = Pi / 180.0f;
/// @brief Multiply by this constant to convert radians into degrees.
static constexpr float Rad2Deg = 180.0f * InvPi;

/**
 * @brief The tolerance threshold for floating point inaccuracies.
 * @example When starting a ray at a surface, it can happen that the surface reports a self-intersection.
 * Mathematically, the intersection distance should be zero, allowing us to discard the intersection, but in reality
 * floating point inaccuracies can report larger distances that would be considered valid intersections.
 * This threshold allows us to solve this (and many other) inaccuracy problem, by treating anything below this threshold
 * as zero.
 */
static constexpr float Epsilon = 1e-4f;

/// @brief Infinity
static constexpr float Infinity = std::numeric_limits<float>::infinity();

// MARK: - utility functions

/// @brief Square root function.
inline float sqrt(float v) { return std::sqrt(v); }
/// @brief Square function, i.e., @code sqr(v) = v * v @endcode.
inline float sqr(float v) { return v * v; }

/// @brief Maximum of two numbers.
inline float max(float a, float b) { return std::max(a, b); }
/// @brief Minimum of two numbers.
inline float min(float a, float b) { return std::min(a, b); }

/// @brief Minimum of three numbers.
inline float min(float a, float b, float c) { return min(a, min(b, c)); }

/// @brief Maximum of three numbers.
inline float max(float a, float b, float c) { return max(a, max(b, c)); }

/// @brief Returns a value of @c mag with the sign of @c sgn .
inline float copysign(float mag, float sgn) { return std::copysign(mag, sgn); }
/// @brief Returns the absolute value of @c v .
inline float abs(float v) { return std::abs(v); }

/// @brief Clamps a value @c v to lie in the interval from @c lo to @c hi .
inline float clamp(float v, float lo, float hi) { return max(lo, min(v, hi)); }
/// @brief Clamps an int value @c v to lie in the interval from @c lo to @c hi .
inline int clampInt(int v, int lo, int hi) { return max(lo, min(v, hi)); }
/// @brief Clamps a value @c v to lie in the unit interval.
inline float saturate(float v) { return clamp(v, 0, 1); }

inline float ffract(float v) {
    return v - std::floor(v);
}

inline float lerp(float a, float b, float t) {
    return a * (1.f - t) + b * t;
}

inline int mod(int a, int b) {
    return (a % b + b) % b;
}

/**
 * @brief Safe square root, which clamps negative input values to zero.
 * @note Use this when floating point errors need to be avoided, e.g., for @code sin = safe_sqrt(1 - sqr(cos)) @endcode.
 */
inline float safe_sqrt(float v) { return v <= 0 ? 0 : std::sqrt(v); }
/**
 * @brief Safe arcus cosine function, which clamps the input to -1 to +1.
 * @note Use this when floating point errors need to be avoided.
 */
inline float safe_acos(float v) { return std::acos(clamp(v, -1, +1)); }

// MARK: - points and vectors

#define BUILD1(expr) \
    result; \
    for (int i = 0; i < result.Dimension; i++) result[i] = expr; \
    return result;

/// @brief A point in @c D dimensions, the components of which are stored with datatype @c T .
template<typename T, int D>
class TPoint {
protected:
    /// @brief The components that constitute the point.
    std::array<T, D> m_data;

public:
    /// @brief The datatype used to store the components of the point.
    typedef T Type;
    /// @brief The dimensionality of the point.
    static constexpr int Dimension = D;

    /// @brief Constructs a point at the origin.
    TPoint() { std::fill(m_data.begin(), m_data.end(), Type(0)); }
    /// @brief Constructs a point from a given array.
    TPoint(const std::array<Type, Dimension> &data) : m_data(data) {}
    /// @brief Constructs a point from one of dimensionality one less, and a value for the last dimension.
    TPoint(const TPoint<T, D - 1> &other, const T &v) {
        std::copy(other.data().begin(), other.data().end(), m_data.begin());
        m_data.back() = v;
    }
    /// @brief Constructs a point whose components all have the value @c v .
    explicit TPoint(const Type &v) { std::fill(m_data.begin(), m_data.end(), v); }

    /// @brief Constructs a two-dimensional point.
    TPoint(const Type &x, const Type &y) : m_data({ x, y }) {}
    /// @brief Constructs a three-dimensional point.
    TPoint(const Type &x, const Type &y, const Type &z) : m_data({ x, y, z }) {}
    /// @brief Constructs a four-dimensional point (used for homogeneous coordinates).
    TPoint(const Type &x, const Type &y, const Type &z, const Type &w) : m_data({ x, y, z, w }) {}

    /// @brief Returns an array of the components of this point.
    const std::array<Type, Dimension> &data() const { return m_data; }
    /// @brief Returns an array of the components of this point that can be modified.
    std::array<Type, Dimension> &data() { return m_data; }
    
    /// @brief Access a component of this point, with an index ranging from @c 0 to @code Dimension - 1 @endcode .
    const Type &operator[](int i) const { return m_data[i]; }
    /// @brief Access a component of this point that can be modified, with an index ranging from @c 0 to @code Dimension - 1 @endcode .
    Type &operator[](int i) { return m_data[i]; }

    const Type &x() const { static_assert(Dimension >= 1); return m_data[0]; }
    const Type &y() const { static_assert(Dimension >= 2); return m_data[1]; }
    const Type &z() const { static_assert(Dimension >= 3); return m_data[2]; }
    const Type &w() const { static_assert(Dimension >= 4); return m_data[3]; }

    Type &x() { static_assert(Dimension >= 1); return m_data[0]; }
    Type &y() { static_assert(Dimension >= 2); return m_data[1]; }
    Type &z() { static_assert(Dimension >= 3); return m_data[2]; }
    Type &w() { static_assert(Dimension >= 4); return m_data[3]; }

    /// @brief Returns the elementwise minimum of two points.
    friend auto elementwiseMin(const TPoint &a, const TPoint &b) { TPoint BUILD1(std::min(a[i], b[i])) }
    /// @brief Returns the elementwise maximum of two points.
    friend auto elementwiseMax(const TPoint &a, const TPoint &b) { TPoint BUILD1(std::max(a[i], b[i])) }
    friend auto operator-(const TPoint &a) { TPoint BUILD1(-a[i]) }

    /// @brief Converts the point to a different datatype.
    template<typename Type2>
    auto cast() const { TPoint<Type2, Dimension> BUILD1(Type2(m_data[i])) }

    /// @brief Checks whether two points are exactly identical.
    bool operator==(const TPoint &other) const { return m_data == other.m_data; }
    /// @brief Checks whether two points are not exactly identical.
    bool operator!=(const TPoint &other) const { return m_data != other.m_data; }

    /// @brief Returns whether the point lies at the origin, i.e., all components are zero. 
    bool isZero() const {
        return std::all_of(m_data.begin(), m_data.end(), [](const Type &v) { return v == 0; });
    }
};

/// @brief A vector in @c D dimensions, the components of which are stored with datatype @c T .
template<typename Type, int Dimension>
class TVector : public TPoint<Type, Dimension> {
public:
    using TPoint<Type, Dimension>::TPoint;
    using TPoint<Type, Dimension>::m_data;

    explicit TVector(const TPoint<Type, Dimension> &point)
    : TPoint<Type, Dimension>(point.data()) {}

    /// @brief Computes the dot product (aka scalar product) with respect to another vector.
    float dot(const TVector &other) const {
        float result = 0;
        for (int i = 0; i < Dimension; i++) result += m_data[i] * other.m_data[i];
        return result;
    }

    /// @brief Computes the cross product with respect to another vector.
    TVector cross(const TVector &other) const {
        static_assert(Dimension == 3);
        return {
            this->y() * other.z() - this->z() * other.y(),
            this->z() * other.x() - this->x() * other.z(),
            this->x() * other.y() - this->y() * other.x()
        };
    }

    /// @brief Projects the vector onto the plane defined by another vector (assuming normalized)
    TVector project(const TVector &other) const {
        return (*this - this->dot(other) * other);
    }

    /// @brief Computes the squared length of this vector. 
    float lengthSquared() const { return dot(*this); }
    /// @brief Computes the length of this vector. 
    float length() const { return std::sqrt(lengthSquared()); }
    /// @brief Returns a normalized copy of this vector.
    auto normalized() const { return *this / length(); }

    /// @brief Returns the length of this vector along with a normalized copy.
    auto lengthAndNormalized() const {
        const float length = this->length();
        return std::make_pair(length, *this / length);
    }

    friend auto operator-(const TVector &a) { TVector BUILD1(-a[i]) }
    friend auto operator*(const Type &a, const TVector &b) { TVector BUILD1(a * b[i]) }
    friend auto operator*(const TVector &a, Type b) { TVector BUILD1(a[i] * b) }
    friend auto operator/(const TVector &a, Type b) { TVector BUILD1(a[i] / b) }
    friend auto operator+(const TVector &a, const TVector &b) { TVector BUILD1(a[i] + b[i]) }
    friend auto operator-(const TVector &a, const TVector &b) { TVector BUILD1(a[i] - b[i]) }
    friend auto operator*(const TVector &a, const TVector &b) { TVector BUILD1(a[i] * b[i]) }
    friend auto operator/(const TVector &a, const TVector &b) { TVector BUILD1(a[i] / b[i]) }
    
    /// @brief Returns the elementwise minimum of two vectors.
    friend auto elementwiseMin(const TVector &a, const TVector &b) { TVector BUILD1(std::min(a[i], b[i])) }
    /// @brief Returns the elementwise maximum of two vectors.
    friend auto elementwiseMax(const TVector &a, const TVector &b) { TVector BUILD1(std::max(a[i], b[i])) }

    /// @brief Returns the lowest component of the vector.
    auto minComponent() const { return *std::min_element(m_data.begin(), m_data.end()); }
    /// @brief Returns the highest component of the vector.
    auto maxComponent() const { return *std::max_element(m_data.begin(), m_data.end()); }

    /// @brief Returns the index of the lowest component of the vector.
    auto minComponentIndex() const { return (int)std::distance(m_data.begin(), std::min_element(m_data.begin(), m_data.end())); }
    /// @brief Returns the index of the highest component of the vector.
    auto maxComponentIndex() const { return (int)std::distance(m_data.begin(), std::max_element(m_data.begin(), m_data.end())); }

    /// @brief Returns a sum of all components of the vector.
    auto sum() const { Type value { 0 }; for (int i = 0; i < Dimension; i++) value += m_data[i]; return value; }
    /// @brief Returns a product of all components of the vector.
    auto product() const { Type value { 1 }; for (int i = 0; i < Dimension; i++) value *= m_data[i]; return value; }

    auto operator*=(const Type &other) { return *this = *this * other; }
    auto operator/=(const Type &other) { return *this = *this / other; }
    auto operator+=(const TVector &other) { return *this = *this + other; }
    auto operator-=(const TVector &other) { return *this = *this - other; }
    auto operator*=(const TVector &other) { return *this = *this * other; }
    auto operator/=(const TVector &other) { return *this = *this / other; }

    /// @brief Converts the vector to a different datatype.
    template<typename Type2>
    auto cast() const { TVector<Type2, Dimension> BUILD1(Type2(m_data[i])) }
};

#undef BUILD_VECTOR

template<typename Type, int Dimension>
auto operator+(const TPoint<Type, Dimension> &a, const TVector<Type, Dimension> &b) {
    TPoint<Type, Dimension> BUILD1(a[i] + b[i])
}

template<typename Type, int Dimension>
auto operator+=(TPoint<Type, Dimension> &a, const TVector<Type, Dimension> &b) {
    a = a + b;
}

template<typename Type, int Dimension>
auto operator-(const TPoint<Type, Dimension> &a, const TVector<Type, Dimension> &b) {
    TPoint<Type, Dimension> BUILD1(a[i] - b[i])
}

template<typename Type, int Dimension>
auto operator-=(TPoint<Type, Dimension> &a, const TVector<Type, Dimension> &b) {
    a = a - b;
}

template<typename Type, int Dimension>
auto operator-(const TPoint<Type, Dimension> &a, const TPoint<Type, Dimension> &b) {
    TVector<Type, Dimension> BUILD1(a[i] - b[i])
}

#define BUILD2(expr) \
    result; \
    for (int row = 0; row < result.Rows; row++) { \
        for (int column = 0; column < result.Columns; column++) { \
            result(row, column) = expr; \
        } \
    } \
    return result;

// MARK: - matrix

/// @brief A matrix with @c R rows and @c C columns, the elements of which are stored with datatype @c T .
template<typename T, int R, int C>
class TMatrix {
    /// @brief The elements that constitute the matrix, in row-major format.
    std::array<std::array<T, C>, R> m_data;

public:
    /// @brief The datatype used to store the elements of the point.
    typedef T Type;
    /// @brief The number of rows in the matrix.
    static constexpr int Rows = R;
    /// @brief The number of columns in the matrix.
    static constexpr int Columns = C;
    
    /// @brief Creates a matrix filled with zeros.
    TMatrix() {}
 
    TMatrix(std::initializer_list<Type> l) {
        assert(l.size() == (Rows * Columns)); // "cannot initialize Matrix<%d, %d> with %d elements", Rows, Columns, l.size()

        auto it = l.begin();
        for (int row = 0; row < Rows; row++) {
            for (int column = 0; column < Columns; column++) {
                (*this)(row, column) = *it++;
            }
        }
    }

    /// @brief Returns an element of this matrix.
    const Type &operator()(int row, int column) const { return m_data[row][column]; }
    /// @brief Returns a reference to an element of this matrix.
    Type &operator()(int row, int column) { return m_data[row][column]; }

    /// @brief Returns a row of the matrix as vector, with index ranging from @c 0 to @code Rows - 1 @endcode .
    auto row(int rowIndex) const { TVector<Type, Columns> BUILD1((*this)(rowIndex, i)) }
    /// @brief Returns a column of the matrix as vector, with index ranging from @c 0 to @code Columns - 1 @endcode .
    auto column(int columnIndex) const { TVector<Type, Rows> BUILD1((*this)(i, columnIndex)) }

    /// @brief Sets a row of the matrix to a given vector, with index ranging from @c 0 to @code Rows - 1 @endcode .
    auto setRow(int rowIndex, const TVector<Type, Columns> &vector) {
        for (int c = 0; c < Columns; c++) (*this)(rowIndex, c) = vector[c];
    }
    /// @brief Sets a column of the matrix to a given vector, with index ranging from @c 0 to @code Columns - 1 @endcode .
    auto setColumn(int columnIndex, const TVector<Type, Rows> &vector) {
        for (int r = 0; r < Rows; r++) (*this)(r, columnIndex) = vector[r];
    }

    /// @brief Returns the product of @code matrix * vector @endcode .
    auto operator*(const TVector<Type, Columns> &vector) const {
        TVector<Type, Rows> BUILD1(row(i).dot(vector))
    }
    /// @brief Returns the product of @code matrix * otherMatrix @endcode .
    template<int Columns2>
    auto operator*(const TMatrix<Type, Columns, Columns2> &other) const {
        TMatrix<Type, Rows, Columns2> BUILD2((*this).row(row).dot(other.column(column)))
    }

    /// @brief Returns the transpose of this matrix. 
    auto transpose() const {
        TMatrix<Type, Columns, Rows> BUILD2((*this)(column, row))
    }

    /// @brief Returns a slice of this matrix of size @c Rows2 x @c Columns2 , offset by @c startRow and @c startColumn .
    template<int Rows2, int Columns2>
    auto submatrix(int startRow, int startColumn) const {
        assert(startRow >= 0 && startRow + Rows2 <= Rows);
        assert(startColumn >= 0 && startColumn + Columns2 <= Columns);
        TMatrix<Type, Rows2, Columns2> BUILD2((*this)(startRow + row, startColumn + column))
    }

    /// @brief Returns the determinant of this matrix. 
    auto determinant() const {
        static_assert(Rows == Columns);
        static_assert(Rows >= 1);
        static_assert(Rows <= 3);

        if constexpr (Rows == 3 && Columns == 3) {
            float v = 0;
            for (int i = 0; i < 3; i++) {
                float a = 1, b = 1;
                for (int j = 0; j < 3; j++) {
                    a *= (*this)((i + j) % 3, j);
                    b *= (*this)((i + j) % 3, 2 - j);
                }
                v += a - b;
            }
            return v;
        } else if constexpr (Rows == 2 && Columns == 2) {
            return (*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0);
        } else if constexpr (Rows == 1 && Columns == 1) {
            return (*this)(0, 0);
        }
    }

    friend auto operator-(const TMatrix &a) { TMatrix BUILD2(-a(row, column)) }
    friend auto operator*(const Type &a, const TMatrix &b) { TMatrix BUILD2(a * b(row, column)) }
    friend auto operator*(const TMatrix &a, Type b) { TMatrix BUILD2(a(row, column) * b) }
    friend auto operator+(const TMatrix &a, const TMatrix &b) { TMatrix BUILD2(a(row, column) + b(row, column)) }
    friend auto operator-(const TMatrix &a, const TMatrix &b) { TMatrix BUILD2(a(row, column) - b(row, column)) }

    /// @brief Constructs an identity matrix for the given dimensions.
    static auto identity() { TMatrix BUILD2(row == column ? 1.0f : 0.0f) }
};

/**
 * @brief Bounds describe the range from one point to another (e.g., an axis-aligned bounding box).
 * In one dimension, this corresponds to an interval from one value to another.
 * In two dimensions, this corresponds to a rectangle.
 */
template<typename T, int D>
class TBounds {
public:
    using Point = TPoint<T, D>;
    using Vector = TVector<T, D>;

private:
    /// @brief The lower corner of the bounding box.
    Point m_min;
    /// @brief The upper corner of the bounding box.
    Point m_max;

public:
    /// @brief The datatype used to store the components of the point.
    typedef T Type;
    /// @brief The dimensionality of the point.
    static constexpr int Dimension = D;

    /// @brief Constructs an empty bounding box.
    static TBounds empty() { return TBounds(); }
    /// @brief Constructs a bounding box spanning all of space. 
    static TBounds full() {
        TBounds result;
        result.m_min = Point(-std::numeric_limits<T>::infinity());
        result.m_max = Point(+std::numeric_limits<T>::infinity());
        return result;
    }

    /// @brief Constructs an empty bounding box.
    TBounds()
    : m_min(+std::numeric_limits<T>::infinity()), m_max(-std::numeric_limits<T>::infinity()) {}

    TBounds(const Point &min, const Point &max)
    : m_min(min), m_max(max) {}

    /// @brief Extends this bounding box to also cover the region of another bounding box.
    void extend(const TBounds &other) {
        m_min = elementwiseMin(m_min, other.m_min);
        m_max = elementwiseMax(m_max, other.m_max);
    }

    /// @brief Extends this bounding box to contain a given point.
    void extend(const Point &point) {
        m_min = elementwiseMin(m_min, point);
        m_max = elementwiseMax(m_max, point);
    }

    /// @brief Returns whether the bounding box is empty.
    bool isEmpty() const {
        for (int dim = 0; dim < Dimension; dim++) {
            if (m_min[dim] >= m_max[dim]) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Returns whether the bounding box covers an infinite amount of space.
     * @note It suffices if a single axis of the bounding box covers an infinite range.
     */
    bool isUnbounded() const {
        for (int dim = 0; dim < Dimension; dim++) {
            if (m_min[dim] < 0 && std::isinf(m_min[dim])) return true;
            if (m_max[dim] > 0 && std::isinf(m_max[dim])) return true;
        }
        return false;
    }

    /// @brief Clamps the components of a point so that it lies within this bounding box.
    Point clip(const Point &point) const {
        return elementwiseMax(m_min, elementwiseMin(point, m_max));
    }

    /// @brief Clamps the components of another bounding box so that it lies within this bounding box.
    TBounds clip(const TBounds &other) const {
        TBounds result;
        result.m_min = clip(other.m_min);
        result.m_max = clip(other.m_max);
        return result;
    }

    /// @brief Returns a shifted version of this bounding box by some given vector.
    TBounds operator+(const Vector &shift) const {
        return { m_min + shift, m_max + shift };
    }

    /// @brief Helper to iterate over the elements of a bounding box.
    struct iterator {
        iterator(const TBounds &bounds, const Point &value)
        : m_value(value), m_start(bounds.min()), m_end(bounds.max()) {}

        Point operator*() const { return m_value; }
        bool operator!=(const iterator &other) const { return m_value != other.m_value; }
        iterator &operator++() {
            for (int i = 0; i < Dimension; i++) {
                if (++m_value[i] < m_end[i]) break;
                if (i == Dimension - 1) break;
                m_value[i] = m_start[i];
            }
            return *this;
        }
    
    private:
        Point m_value;
        Point m_start, m_end;
    };

    /// @brief Returns the lower corner of this bounding box. 
    const Point &min() const { return m_min; }
    /// @brief Returns the upper corner of this bounding box. 
    const Point &max() const { return m_max; }
    /// @brief Returns a reference to the lower corner of this bounding box. 
    Point &min() { return m_min; }
    /// @brief Returns a reference to the upper corner of this bounding box. 
    Point &max() { return m_max; }

    /// @brief Returns the diagonal of the bounding box, i.e., @code max - min @endcode .
    Vector diagonal() const { return m_max - m_min; }
    /// @brief Returns the point that lies in the center of the bounding box. 
    Point center() const { return m_min + diagonal() / 2; }

    iterator begin() const { return { *this, m_min }; }
    iterator end() const {
        Point end = m_min;
        end[Dimension - 1] = m_max[Dimension - 1];
        return { *this, end };
    }

    /// @brief Tests whether the given point lies within this bounding box.
    bool includes(const Point &point) const {
        for (int dim = 0; dim < D; dim++) {
            if (point[dim] < m_min[dim]) return false;
            if (point[dim] > m_max[dim]) return false;
        }
        return true;
    }
};

/// @brief A two-dimensional point with floating point components.
using Point2 = TPoint<float, 2>;
/// @brief A two-dimensional point with integer components.
using Point2i = TPoint<int, 2>;
/// @brief A three-dimensional point with floating point components.
using Point = TPoint<float, 3>;

/// @brief A two-dimensional vector with floating point components.
using Vector2 = TVector<float, 2>;
/// @brief A two-dimensional vector with integer components.
using Vector2i = TVector<int, 2>;
/// @brief A three-dimensional vector with floating point components.
using Vector = TVector<float, 3>;
/// @brief A three-dimensional vector with integer components.
using Vector3i = TVector<int, 3>;
/// @brief A four-dimensional vector with floating point components (used for homogeneous coordinates).
using Vector4 = TVector<float, 4>;

/// @brief An integer rectangle (e.g., to describe the blocks of an image).
using Bounds2i = TBounds<int, 2>;
/// @brief A three-dimensional axis-aligned bounding box with floating point components.
using Bounds = TBounds<float, 3>;

/// @brief A 3x3 matrix with floating point components.
using Matrix3x3 = TMatrix<float, 3, 3>;
/// @brief A 4x4 matrix with floating point components (used for homogeneous coordinates).
using Matrix4x4 = TMatrix<float, 4, 4>;

/// @brief Computes the component-wise modulo operation of two points. 
template<int Dimension>
auto operator%(const TPoint<int, Dimension> &a, const TPoint<int, Dimension> &b) { TPoint<int, Dimension> BUILD1(a[i] % b[i]) }

/// @brief Computes the component-wise modulo operation of two points. 
template<int Dimension>
auto operator%(const TPoint<float, Dimension> &a, const TPoint<float, Dimension> &b) { TPoint<float, Dimension> BUILD1(fmodf(a[i], b[i])) }

#undef BUILD1
#undef BUILD2

/// @brief Computes the inverse of a 4x4 matrix. 
std::optional<Matrix4x4> invert(const Matrix4x4 &matrix);

/// @brief Builds an orthonormal basis from the given vectors. 
void buildOrthonormalBasis(const Vector &normal, Vector &tangent, Vector &bitangent);

/// @brief Converts a direction (normalized) to sphere UV coordinates
Vector2 directionToSphereUV(const Vector &dir);

// MARK: - rays and related structures

/**
 * @brief Reflects a vector @c w at a surface with normal @c n .
 * @note All vectors are assumed to point away from the surface.
 */
inline Vector reflect(const Vector &w, const Vector &n) {
    return 2 * n.dot(w) * n - w;
}

/**
 * @brief Refracts a vector @c w at a surface with normal @c n and relative index of refraction @c eta .
 * @note All vectors are assumed to point away from the surface.
 * @return Refracted vector, or zero vector in case of total internal reflection.
*/
inline Vector refract(const Vector &w, const Vector &n, float eta) {
    const float invEta = 1 / eta;
    const float k = 1 - sqr(invEta) * (1 - sqr(n.dot(w)));
    if (k < 0) {
        // total internal reflection
        return Vector();
    }
    return (invEta * n.dot(w) - sqrt(k)) * n - invEta * w;
}

/// @brief Describes a ray that propagates through space.
struct Ray {
    /// @brief The origin whether the ray starts (t = 0).
    Point origin;
    /// @brief The direction of the ray, which must always be normalized.
    Vector direction;
    /// @brief The number of bounces encountered by the ray, for use in integrators.
    int depth = 0;

    Ray() {}
    Ray(Point origin, Vector direction, int depth = 0)
    : origin(origin), direction(direction), depth(depth) {}

    /// @brief Computes a point on the ray for a given distance @c t .
    Point operator()(float t) const {
        return origin + t * direction;
    }

    /// @brief Returns a copy of the ray with normalized direction vector (useful after applying transforms). 
    Ray normalized() const {
        return Ray(
            origin,
            direction.normalized(),
            depth
        );
    }
};

/**
 * @brief Defines shading frames and common trigonometrical functions used within them.
 * In lightwave, we follow the convention that material functions (sampling and evaluation of Bsdfs and Emissions) happen
 * within a local frame where the normal vector is always [0,0,1], which simplifies much of their implementation.
 * This class defines the orientation of a surface, which allows us to transform world space vectors (e.g., ray directions)
 * into this local shading frame.
 * @warning Make sure that the vectors that constitute your Frame are always orthonormal and form a right-handed coordinate system.
 */
struct Frame {
    /// @brief The normal vector of the surface (implicitly given by the cross product of tangent and bitangent).
    Vector normal;
    /// @brief The tangent vector, which lies in the surface.
    Vector tangent;
    /// @brief The bitangent vector, which lies in the surface and is orthogonal to the tangent vector.
    Vector bitangent;

    Frame() {}

    /// @brief Constructs a frame with arbitrary tangent and bitangent vector from a given normal vector. 
    explicit Frame(const Vector &normal) : normal(normal) {
        buildOrthonormalBasis(normal, tangent, bitangent);
    }

    /// @brief Converts a vector from world-space coordinates to local shading coordinates. 
    Vector toLocal(const Vector &world) const {
        return { world.dot(tangent), world.dot(bitangent), world.dot(normal) };
    }
    /// @brief Converts a vector from local shading coordinates to world-space coordinates.
    Vector toWorld(const Vector &local) const {
        return local.x() * tangent + local.y() * bitangent + local.z() * normal;
    }
    
    /// @brief Tests whether two vectors lie within the same hemisphere (i.e., the angle they form is below 180°). 
    static bool sameHemisphere(const Vector &wi, const Vector &wo) {
        return cosTheta(wi) * cosTheta(wo) > 0;
    }

    /// @brief Computes cos(theta) for a vector in shading coordinates. 
    static float cosTheta(const Vector &w) { return w.z(); }
    /// @brief Computes cos^2(theta) for a vector in shading coordinates. 
    static float cosTheta2(const Vector &w) { return sqr(w.z()); }
    /// @brief Computes |cos(theta)| for a vector in shading coordinates. 
    static float absCosTheta(const Vector &w) {  return abs(w.z()); }
    
    /// @brief Computes sin(theta) for a vector in shading coordinates. 
    /// @brief Computes sin^2(theta) for a vector in shading coordinates. 
    static float sinTheta(const Vector &w) { return safe_sqrt(1 - cosTheta2(w)); }
    static float sinTheta2(const Vector &w) { return 1 - cosTheta2(w); }
    
    /// @brief Computes cos(phi)*sin(theta) for a vector in shading coordinates. 
    static float cosPhiSinTheta(const Vector &w) { return w.x(); }
    /// @brief Computes sin(phi)*sin(theta) for a vector in shading coordinates. 
    static float sinPhiSinTheta(const Vector &w) { return w.y(); }

    /// @brief Computes tan(theta) for a vector in shading coordinates. 
    static float tanTheta(const Vector &w) {
        const float cos = cosTheta(w);
        return safe_sqrt(1 - sqr(cos)) / cos;
    }
    
    /// @brief Computes tan^2(theta) for a vector in shading coordinates. 
    static float tanTheta2(const Vector &w) {
        const float cos2 = cosTheta2(w);
        return (1 - cos2) / cos2;
    }
};

/// @brief Barycentric interpolation ([0,0] returns a, [1,0] returns b, and [0,1] returns c).
template<typename T>
static T interpolateBarycentric(const Vector2 &bary, const T &a, const T &b, const T &c) {
    return a * (1 - bary.x() - bary.y()) + b * bary.x() + c * bary.y();
}

/// @brief Barycentric interpolation ([0,0] returns a, [1,0] returns b, and [0,1] returns c).
template<>
Point interpolateBarycentric(const Vector2 &bary, const Point &a, const Point &b, const Point &c) {
    return Point(interpolateBarycentric(bary, Vector(a), Vector(b), Vector(c)));
}

/// @brief A vertex of a triangle mesh.
struct Vertex {
    /// @brief The position of the vertex in object space.
    Point position;
    /// @brief The texture coordinates associated with the vertex.
    Vector2 texcoords;
    /// @brief The normal vector, which will be barycentrically interpolated when smooth normals are used.
    Vector normal;

    /// @brief Barycentric interpolation of vertices ([0,0] returns a, [1,0] returns b, and [0,1] returns c).
    static Vertex interpolate(const Vector2 &bary, const Vertex &a, const Vertex &b, const Vertex &c) {
        return {
            .position  = interpolateBarycentric(bary, a.position , b.position , c.position ),
            .texcoords = interpolateBarycentric(bary, a.texcoords, b.texcoords, c.texcoords),
            .normal    = interpolateBarycentric(bary, a.normal   , b.normal   , c.normal   ),
        };
    }
};

/// @brief A point on a surface along with context about the orientation of the surface.
struct SurfaceEvent {
    /// @brief The position of the surface point.
    Point position;
    /// @brief The texture coordinates of the surface for the given position.
    Point2 uv;
    /// @brief The shading frame of the surface at the given position.
    Frame frame;
    /// @brief The probability of sampling the point when doing area sampling, in area units.
    float pdf;
    /// @brief The instance object associated with the surface.
    const Instance *instance = nullptr;
};

/// @brief Describes an intersection of a ray with a surface.
struct Intersection : public SurfaceEvent {
    /// @brief The direction of the ray that hit the surface, pointing away from the surface.
    Vector wo;
    /// @brief The intersection distance, which can also be used to specify a maximum distance when querying intersections.
    float t;

    /// @brief Statistics recorded while traversing acceleration structures.
    struct {
        /// @brief The number of BVH nodes that have been tested for intersection.
        int bvhCounter = 0;
        /// @brief The number of shapes that have been tested for intersection.
        int primCounter = 0;
    } stats;

    Intersection(const Vector &wo = Vector(), float t = Infinity)
    : wo(wo), t(t) {}

    Intersection(const Intersection &other) = default;
    Intersection &operator=(const Intersection &other) = default;

    /// @brief Reports whether an object has been hit.
    operator bool() const {
        return instance != nullptr;
    }

    /// @brief Evaluates the emission of the underlying instance.
    Color evaluateEmission() const;
    /// @brief Samples the Bsdf of the underlying surface.
    BsdfSample sampleBsdf(Sampler &rng) const;
    /// @brief Evaluates the Bsdf of the underlying surface.
    BsdfEval evaluateBsdf(const Vector &wi) const;
};

/// @brief Print a given point to an output stream.
template<typename Type, int Dimension>
static std::ostream &operator<<(std::ostream &os, const TPoint<Type, Dimension> &point) {
    os << "Point[";
    for (int i = 0; i < Dimension; i++) {
        if (i) os << ", ";
        os << point[i];
    }
    os << "]";
    return os;
}

/// @brief Print a given vector to an output stream.
template<typename Type, int Dimension>
static std::ostream &operator<<(std::ostream &os, const TVector<Type, Dimension> &vector) {
    os << "Vector[";
    for (int i = 0; i < Dimension; i++) {
        if (i) os << ", ";
        os << vector[i];
    }
    os << "]";
    return os;
}

/// @brief Print a given matrix to an output stream.
template<typename Type, int Rows, int Columns>
static std::ostream &operator<<(std::ostream &os, const TMatrix<Type, Rows, Columns> &matrix) {
    os << "Matrix[" << std::endl;
    for (int row = 0; row < Rows; row++) {
        os << "  ";
        for (int column = 0; column < Columns; column++) {
            os << matrix(row, column);
            os << ", ";
        }
        os << std::endl;
    }
    os << "]";
    return os;
}

}

namespace std {

static bool isfinite(const lightwave::Color &c);

/// @brief Tests whether all components of a given point are finite.
template<typename F, int D>
bool isfinite(const lightwave::TPoint<F, D> &p) {
    for (int dim = 0; dim < D; dim++)
        if (!std::isfinite(p[dim]))
            return false;
    return true;
}

}

// MARK: - assertions

namespace lightwave {

//#ifdef LW_DEBUG
// lightwave with assertions enabled is not much slower, so we also keep them enabled in Release builds
#define ENABLE_ASSERTIONS
//#endif

/// @brief Asserts that a given vector/color/value is finite, and aborts if that is note the case.
#define assert_finite(x, ctx) if (!check_finite(x, __FILE__, __LINE__)) { ctx; abort(); }
template <typename T>
inline bool check_finite(const T &n, const char *file, size_t line) {
#ifdef ENABLE_ASSERTIONS
    if (!std::isfinite(n)) {
        logger(EError, "non-finite value in %s:%d (%s)", file, line, n);
        return false;
    }
#endif
    return true;
}

/// @brief Asserts that a given vector is normalized, and aborts if that is note the case.
#define assert_normalized(x, ctx) if (!check_normalized(x, __FILE__, __LINE__)) { ctx; abort(); }
template <typename T>
inline bool check_normalized(const T &n, const char *file, size_t line) {
#ifdef ENABLE_ASSERTIONS
    float lenSqr = n.lengthSquared();
    if (!(abs(lenSqr - 1) < 0.001f)) {
        logger(EError, "vector not normalized in %s:%d (%s, lengthSquared: %f)", file, line, n, lenSqr);
        return false;
    }
#endif
    return true;
}

}
