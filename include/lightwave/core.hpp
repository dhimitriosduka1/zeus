/**
 * @file core.hpp
 * @brief Contains common macros and classes that are used across the entire renderer.
 */

#pragma once

#define LW_STRINGIFY_2(s) #s
#define LW_STRINGIFY(s) LW_STRINGIFY_2(s)

// CPU
#if defined(__arm__) || defined(_M_ARM)
#define LW_CPU_ARM
#elif defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define LW_CPU_X86
#else
#define LW_CPU_UNKNOWN
#endif

// OS
#if defined(__linux) || defined(linux)
#define LW_OS_LINUX
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__) || defined(__TOS_WIN__)
#define LW_OS_WINDOWS
#if !defined(Win64) && !defined(_WIN64)
#define LW_OS_WINDOWS_32
#else
#define LW_OS_WINDOWS_64
#endif
#elif defined(__APPLE__)
#define LW_OS_APPLE
#else
#warning Your operating system is currently not supported
#endif

// Compiler
#if defined(__CYGWIN__)
#define LW_CC_CYGWIN
#endif

#if defined(__GNUC__)
#define LW_CC_GNU
#endif

#if defined(__clang__)
#define LW_CC_CLANG
#endif

#if defined(__MINGW32__)
#define LW_CC_MINGW32
#endif

#if defined(__INTEL_COMPILER)
#define LW_CC_INTEL
#endif

#if defined(_MSC_VER)
#define LW_CC_MSC
#pragma warning(disable : 4251 4996)
#endif

// Check if C++17
#ifdef LW_CC_MSC
#define LW_CPP_LANG _MSVC_LANG
#else
#define LW_CPP_LANG __cplusplus
#endif

#if LW_CPP_LANG < 202002L
#pragma warning Lightwave requires C++ 20 to compile successfully
#endif

// Headers
#include <memory>
#include <ostream>
#include <chrono>
#include <string>

#ifdef LW_CC_MSC
#pragma warning(push)
#pragma warning(disable : 4127 4100)
#endif

#include <tinyformat.h>

#ifdef LW_CC_MSC
#pragma warning(pop)
#endif

namespace lightwave {

/// @brief A shared pointer for a given type.
template<typename T>
using ref = std::shared_ptr<T>;

/// @brief A shared const pointer for a given type.
template<typename T>
using cref = std::shared_ptr<const T>;

// Forward declarations
class Properties;
class Group;
class Scene;
class Shape;
class Camera;
class Entity;
class Sampler;
class Instance;
struct Intersection;
class Color;
class Image;
class Texture;
class Volume;
class Bsdf;
struct BsdfSample;
struct BsdfEval;
class Light;
struct DirectLightSample;
struct DirectLightEval;
class BackgroundLight;
struct BackgroundLightEval;

/// @brief The base class used by all objects in lightwave.
class Object {
    /// @brief A unique identifier used to refer to the object in logs or filenames.
    std::string m_id;

public:
    /// @brief Returns the unique identifier used to refer to this object in logs or filenames.
    std::string id() const { return m_id; }
    /// @brief Sets the unique identifier used to refer to this object in logs or filenames.
    void setId(const std::string &id) { m_id = id; }

    /// @brief Returns a textual representation of this object, used for debugging.
    virtual std::string toString() const = 0;

    virtual ~Object() {}
};

/**
 * @brief Represents an object that performs a certain action when placed at the root of a scene file.
 * @example An integrator will render the scene when executed, and a test will render an image and compare it to a reference.
 * @see Integrator
 * @see Test
 */
class Executable : public Object {
public:
    /// @brief Performs the action of this object when placed at the root of a scene file.
    virtual void execute() = 0;
};

/**
 * @brief Represents an exception that occured while loading the scene.
 * @warning Please do not use exceptions in functions called during rendering, as their mere presence in code
 * can slow down performance significantly.
 */
class Exception : public std::runtime_error {
    /// @brief A textual description of what has failed.
    std::string m_message;

public:
    Exception(const std::string &message, const char *file, unsigned int line)
    : std::runtime_error(message) {
        m_message = tfm::format("%s:%d : %s", file, line, message);
    }

    ~Exception() throw() {}

    /// @brief Returns a textual description of what has failed. 
    const char *what() const throw() {
        return m_message.c_str();
    }
};

/**
 * @brief Converts a given object to a string, and then indents the string by @c amount levels (two spaces each).
 * @note Used for convenience in many @c toString functions of objects.
 */
template<typename T>
static std::string indent(const T &obj, int amount = 1) {
    std::stringstream ss;
    ss << obj;
    return indent(ss.str(), amount);
}

/// @brief Indents the given string by @c amount levels (two spaces each).
template<>
std::string indent(const std::string &str, int amount) {
    std::string pad = "";
    for (int i = 0; i < amount; i++) pad += "  ";

    std::stringstream iss(str);
    std::stringstream oss;
    std::string line;
    bool firstLine = true;

    while (true) {
        std::getline(iss, line);
        if (!firstLine) oss << pad;
        oss << line;
        firstLine = false;

        if (iss.eof()) break;
        oss << std::endl;
    }
    return oss.str();
}

/// @brief Helper class to time the execution of code fragments.
class Timer {
    std::chrono::high_resolution_clock::time_point m_startTime;

public:
    Timer() {
        m_startTime = std::chrono::high_resolution_clock::now();
    }

    /// @brief Returns the elapsed seconds since the creation of this Timer instance.
    float getElapsedTime() const {
        using namespace std::chrono;
        const auto currentTime = high_resolution_clock::now();
        return duration_cast<milliseconds>(currentTime - m_startTime).count() / 1000.f;
    }
};

/// @brief Prints a given lightwave object to an output stream, with special handling for null pointers.
inline std::ostream &operator<<(std::ostream &os, const lightwave::Object *object) {
    if (object == nullptr) {
        os << "null";
    } else {
        os << object->toString();
    }
    return os;
}

/// @brief Throws an exception, with a description derived from the provided format string.
#define lightwave_throw(...) throw lightwave::Exception(tfm::format(__VA_ARGS__), __FILE__, __LINE__);
/// @brief Throws a nested exception, with a description derived from the provided format string.
#define lightwave_throw_nested(...) std::throw_with_nested(lightwave::Exception(tfm::format(__VA_ARGS__), __FILE__, __LINE__));
/// @brief Denotes that a function has not been implemented, and will abort the render process.
#define NOT_IMPLEMENTED { \
    logger(EError, "%s:%d NOT_IMPLEMENTED", __FILE__, __LINE__); \
    abort(); \
}

}
