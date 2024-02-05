/**
 * @file properties.hpp
 * @brief Contains the Properties class, which represent data parsed from the
 * scene description files.
 */

#pragma once

#include <lightwave/color.hpp>
#include <lightwave/core.hpp>

#include <filesystem>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <iostream>

namespace lightwave {

std::string demangle(const char *name);

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename To> To parse_string(const std::string &) {
    lightwave_throw("cannot convert string into %s",
                    demangle(typeid(To).name()));
}

template <> float parse_string(const std::string &str);
template <> int parse_string(const std::string &str);
template <> bool parse_string(const std::string &str);
template <> Color parse_string(const std::string &str);
template <> Vector2 parse_string(const std::string &str);
template <> Vector parse_string(const std::string &str);
template <> Point2 parse_string(const std::string &str);
template <> Point parse_string(const std::string &str);
template <> Matrix4x4 parse_string(const std::string &str);

/**
 * @brief A Properties object contains all attributes, children and additional
 * context that has been parsed from a node in the scene description file.
 */
class Properties {
    using Value =
        std::variant<float, int, bool, std::string, Color, Vector, ref<Object>>;

    static std::string toString(const Value &value) {
        return std::visit(
            overloaded{
                [&](auto &&arg) { return tfm::format("%s", arg); },
                [&](const std::string &str) {
                    return tfm::format("\"%s\"", str);
                },
                [&](const ref<Object> &object) { return object->toString(); } },
            value);
    }

    /**
     * @brief The directory of the scene file that the node is part of.
     * @note The basePath of a node within an included file will point to the
     * directory of the included file.
     */
    std::filesystem::path m_basePath;
    /// @brief The attributes associated with the node.
    std::map<std::string, Value> m_attributes;
    /// @brief The children associated with the node.
    std::vector<ref<Object>> m_children;
    /// @brief A set of attributes that have not yet been queried, used to warn
    /// the user about potentially misspelled attributes.
    mutable std::set<std::string> m_unqueriedAttributes;
    /// @brief A set of children that have not yet been queried, used to warn
    /// the user about potentially misplaced nodes.
    mutable std::set<ref<Object>> m_unqueriedChildren;

public:
    Properties() : m_basePath(std::filesystem::current_path()) {}

    Properties(const std::filesystem::path &basePath) : m_basePath(basePath) {}

    std::string toString() const {
        std::stringstream ss;
        ss << "Properties[" << std::endl;
        for (auto &attr : m_attributes) {
            ss << "  " << attr.first << ": ";
            ss << indent(toString(attr.second));
            ss << "," << std::endl;
        }
        for (auto &attr : m_children) {
            ss << "  " << indent(attr.get());
            ss << "," << std::endl;
        }
        ss << "]";
        return ss.str();
    }

    /**
     * @brief Returns the directory of the scene file that the node is part of.
     * @note The basePath of a node within an included file will point to the
     * directory of the included file.
     */
    std::filesystem::path basePath() const { return m_basePath; }

    /**
     * @brief Registers an object as child of the node.
     * @param needsQuery If false, disables the "unqueried" warning for this
     * child (useful when a child is defined for use with references in a
     * different location).
     */
    void addChild(const ref<Object> &object, bool needsQuery = true) {
        m_children.push_back(object);
        if (needsQuery) {
            m_unqueriedChildren.insert(object);
        }
    }

    /// @brief Checks whether a given attribute is present.
    bool has(const std::string &name) const {
        return m_attributes.find(name) != m_attributes.end();
    }

    /// @brief Sets an attribute to the given value.
    template <typename T>
    std::enable_if_t<!std::is_base_of_v<Object, T>, void>
    set(const std::string &name, const T &value) {
        if (has(name)) {
            lightwave_throw("property \"%s\" redefined", name);
        }
        m_attributes[name] = value;
        m_unqueriedAttributes.insert(name);
    }

    /// @brief Sets an attribute to the given value.
    template <typename T>
    std::enable_if_t<std::is_base_of_v<Object, T>, void>
    set(const std::string &name, const ref<T> &object) {
        if (has(name)) {
            lightwave_throw("property \"%s\" redefined", name);
        }
        m_attributes[name] = std::static_pointer_cast<Object>(object);
        m_unqueriedAttributes.insert(name);
    }

    /// @brief Gets the value of an attribute.
    template <typename T>
    std::enable_if_t<
        !std::is_base_of_v<Object, T> && !std::is_same_v<T, std::string>, T>
    get(const std::string &name) const {
        auto v = m_attributes.find(name);
        if (v == m_attributes.end()) {
            lightwave_throw("missing required property \"%s\"", name);
        }

        m_unqueriedAttributes.erase(name);
        return std::visit(
            overloaded{
                [&](auto &&arg) -> T {
                    lightwave_throw("cannot cast %s into %s",
                                    demangle(typeid(arg).name()),
                                    demangle(typeid(T).name()));
                },
                [&](std::string arg) -> T {
                    if constexpr (std::is_same_v<T, std::filesystem::path>) {
                        return this->m_basePath / arg;
                    }
                    return parse_string<T>(arg);
                },
                [&](T arg) -> T { return arg; } },
            v->second);
    }

    /// @brief Gets the value of an attribute.
    template <typename T>
    std::enable_if_t<std::is_same_v<T, std::string>, T>
    get(const std::string &name) const {
        auto v = m_attributes.find(name);
        if (v == m_attributes.end()) {
            lightwave_throw("property \"%s\" undefined", name);
        }

        m_unqueriedAttributes.erase(name);
        return std::visit(
            overloaded{ [&](auto &&arg) -> std::string {
                           lightwave_throw(
                               "cannot cast %s into %s",
                               demangle(typeid(arg).name()),
                               demangle(typeid(std::string).name()));
                       },
                        [&](std::string arg) -> std::string { return arg; } },
            v->second);
    }

    /// @brief Gets the value of an attribute, with a fallback value if the
    /// attribute is not present.
    template <typename T>
    std::enable_if_t<!std::is_base_of_v<Object, T>, T>
    get(const std::string &name, const T &fallback) const {
        auto v = m_attributes.find(name);
        if (v == m_attributes.end()) {
            return fallback;
        }
        return get<T>(name);
    }

    /// @brief Gets the value of an attribute.
    template <typename T>
    std::enable_if_t<std::is_base_of_v<Object, T>, ref<T>>
    get(const std::string &name) const {
        auto v = m_attributes.find(name);
        if (v == m_attributes.end()) {
            lightwave_throw("property \"%s\" undefined", name);
        }

        m_unqueriedAttributes.erase(name);

        ref<Object> object;
        try {
            object = std::get<ref<Object>>(v->second);
        } catch (...) {
            lightwave_throw("expected \"%s\" to be an object", name);
        }

        auto casted = std::dynamic_pointer_cast<T>(object);
        if (!casted) {
            lightwave_throw("object of wrong class");
        }
        return casted;
    }

    /// @brief Gets the value of an attribute, with a fallback value if the
    /// attribute is not present.
    template <typename T>
    std::enable_if_t<std::is_base_of_v<Object, T>, ref<T>>
    get(const std::string &name, const ref<T> &fallback) const {
        auto v = m_attributes.find(name);
        if (v == m_attributes.end()) {
            return fallback;
        }
        return get<T>(name);
    }

    /// @brief Looks up a string property in a list of values and returns the
    /// matching option.
    template <typename T>
    T getEnum(const std::string &name,
              const std::vector<std::pair<std::string, T>> &options) const {
        auto value = get<std::string>(name);
        for (const auto &option : options) {
            if (option.first == value)
                return option.second;
        }
        lightwave_throw("invalid value \"%s\" specified for %s", value, name);
    }

    /// @brief Looks up a string property in a list of values and returns the
    /// matching option, or default value if none specified.
    template <typename T>
    T getEnum(const std::string &name, const T &fallback,
              const std::vector<std::pair<std::string, T>> &options) const {
        if (has(name)) {
            return getEnum<T>(name, options);
        }
        return fallback;
    }

    /// @brief Gets a child of a given type.
    template <typename T> ref<T> getChild(bool required = true) const {
        ref<T> result = nullptr;
        for (auto child : m_children) {
            if (auto casted = std::dynamic_pointer_cast<T>(child)) {
                if (result.get()) {
                    lightwave_throw("multiple %s children present",
                                    demangle(typeid(T).name()));
                }
                result = casted;
                m_unqueriedChildren.erase(child);
            }
        }
        if (required && !result.get()) {
            lightwave_throw("could not find required %s child",
                            demangle(typeid(T).name()));
        }
        return result;
    }

    /// @brief Gets a child of a given type, or returns null if no such child is
    /// present.
    template <typename T> ref<T> getOptionalChild() const {
        return getChild<T>(false);
    }

    /// @brief Gets all children of a given type of this node.
    template <typename T> std::vector<ref<T>> getChildren() const {
        std::vector<ref<T>> result;
        for (auto child : m_children) {
            if (auto casted = std::dynamic_pointer_cast<T>(child)) {
                result.push_back(casted);
                m_unqueriedChildren.erase(child);
            }
        }
        return result;
    }

    /// @brief Gets a child of a given type.
    std::vector<ref<Object>> children() const { return m_children; }

    ~Properties() {
        for (auto &child : m_unqueriedChildren) {
            logger(EWarn, "a child node was specified, but never queried: %s",
                   child);
        }

        for (auto &property : m_unqueriedAttributes) {
            logger(EWarn, "attribute \"%s\" was specified, but never queried",
                   property);
        }
    }
};

} // namespace lightwave
