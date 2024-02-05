/**
 * @file registry.hpp
 * @brief Contains interfaces to register and query classes that can be referenced in scene description files.
 */

#pragma once

#include <lightwave/core.hpp>

#include <map>
#include <string>

namespace lightwave {

/// @brief Keeps track of all classes that can be referenced in scene description files.
class Registry {
public:
    using Constructor = ref<Object> (*)(const Properties &);
    
    template<typename C>
    struct Registrar {
        Registrar(const std::string &category, const std::string &name, Constructor constructor) {
            Registry::constructors()[category][name] = constructor;
        }
    };

    /// @brief Checks whether a given category (e.g., "shape") of classes exists.
    static bool exists(const std::string &category);
    /// @brief Checks whether a given class (e.g., "sphere") within a given category of classes (e.g., "shape") exists.
    static bool exists(const std::string &category, const std::string &name);
    /// @brief Prints all available categories and classes to the provided output stream.
    static void listAvailable(std::ostream &os);
    /// @brief Creates an object of the given category and class by calling its constructor with the provided properties.
    static ref<Object> create(const std::string &category, const std::string &name, const Properties &properties);

private:
    using ConstructorMap = std::map<std::string, std::map<std::string, Constructor>>;
    static ConstructorMap &constructors();
};

}

#define REGISTER_CLASS(Class, Category, Name) namespace lightwave { \
    ref<Object> Create##Class(const Properties &properties) { \
        try { \
            /* do not use std::make_shared so error messages make more sense */ \
            return ref<Class>(new Class(properties)); \
        } catch (...) { \
            lightwave_throw_nested("while creating " LW_STRINGIFY(Class) " object"); \
        } \
    } \
    Registry::Registrar<Class> r_##Class(Category, Name, Create##Class); \
}

#define REGISTER_BSDF(       Class, Name) REGISTER_CLASS(Class, "bsdf"             , Name)
#define REGISTER_TEXTURE(    Class, Name) REGISTER_CLASS(Class, "texture"          , Name)
#define REGISTER_VOLUME(     Class, Name) REGISTER_CLASS(Class, "volume"           , Name)
#define REGISTER_CAMERA(     Class, Name) REGISTER_CLASS(Class, "camera"           , Name)
#define REGISTER_SHAPE(      Class, Name) REGISTER_CLASS(Class, "shape"            , Name)
#define REGISTER_EMISSION(   Class, Name) REGISTER_CLASS(Class, "emission"         , Name)
#define REGISTER_SAMPLER(    Class, Name) REGISTER_CLASS(Class, "sampler"          , Name)
#define REGISTER_TRANSFORM(  Class, Name) REGISTER_CLASS(Class, "transform"        , Name)
#define REGISTER_INTEGRATOR( Class, Name) REGISTER_CLASS(Class, "integrator"       , Name)
#define REGISTER_GROUP(      Class, Name) REGISTER_CLASS(Class, "group"            , Name)
#define REGISTER_LIGHT(      Class, Name) REGISTER_CLASS(Class, "light"            , Name)
#define REGISTER_TEST(       Class, Name) REGISTER_CLASS(Class, "test"             , Name)
#define REGISTER_POSTPROCESS(Class, Name) REGISTER_CLASS(Class, "postprocess"      , Name)
