#pragma once

#include <lightwave/core.hpp>

#include "xml.hpp"

#include <vector>
#include <stack>
#include <map>
#include <filesystem>

namespace lightwave {

class SceneParser : public XMLParser::Delegate {
protected:
    struct Node;
    struct RootNode;
    struct ObjectNode;
    struct PrimitiveNode;
    struct IncludeNode;
    struct ReferenceNode;
    struct TransformNode;

    std::stack<ref<Node>> m_stack;
    std::vector<ref<Object>> m_objects;

    std::string resolveVariables(const std::string &value);

    void open(const std::string &tag) override;
    void enter() override;
    void attribute(const std::string &name, const std::string &value) override;
    void close() override;

public:
    SceneParser(const std::filesystem::path &path);
    std::vector<ref<Object>> objects() const;
};

}
