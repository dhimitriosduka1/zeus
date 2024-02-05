#include <lightwave/properties.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/transform.hpp>

#include <istream>
#include <iostream>
#include <fstream>

#include "parser.hpp"

namespace lightwave {

struct SceneParser::Node {
    ref<Node> parent;

    Node(const ref<Node> &parent) : parent(parent) {}

    virtual std::filesystem::path getFilePath() const { return parent->getFilePath(); }
    virtual RootNode &getRoot() { return parent->getRoot(); }

    virtual void enter() {}
    virtual void attribute(const std::string &name, const std::string &value) { }
    virtual void addChild(const ref<Object> &object, const std::string &name) {
        lightwave_throw("children are not supported by this node");
    }
    virtual void close() {}

    virtual ~Node() {}
};

struct SceneParser::RootNode : public SceneParser::Node {
    std::map<std::string, ref<Object>> namedObjects;
    std::vector<ref<Object>> &objects;
    std::filesystem::path filepath;
    SceneParser &sceneParser;

    RootNode(std::vector<ref<Object>> &objects, const std::filesystem::path &filepath, SceneParser &sceneParser)
    : Node(nullptr), objects(objects), filepath(filepath), sceneParser(sceneParser) {}

    void nameObject(const std::string &name, const ref<Object> &object) {
        namedObjects[name] = object;
    }

    ref<Object> lookup(const std::string &name) {
        auto it = namedObjects.find(name);
        if (it == namedObjects.end()) {
            lightwave_throw("could not find an object named \"%s\"", name);
        }
        return it->second;
    }

    std::filesystem::path getFilePath() const override {
        return filepath;
    }

    RootNode &getRoot() override { return *this; }

    void addChild(const ref<Object> &object, const std::string &name) override {
        objects.push_back(object);
    }
};

struct SceneParser::ObjectNode : public SceneParser::Node {
    std::string tag;
    std::string type;
    std::string name;
    std::string id;
    Properties properties;

    ref<Transform> transform;

    ObjectNode(const std::string &tag, const ref<Node> &parent)
    : Node(parent), tag(tag), properties(parent->getFilePath().remove_filename()) {}

    void attribute(const std::string &key, const std::string &value) override {
        if (key == "type") {
            type = value;
        } else if (key == "name") {
            name = value;
        } else if (key == "id") {
            id = value;
        } else {
            properties.set<std::string>(key, value);
        }
    }

    void enter() override {
        if (tag == "transform") {
            transform = std::static_pointer_cast<Transform>(Registry::create(tag, type, properties));
        }
    }

    void addChild(const ref<Object> &object, const std::string &child_name) override {
        if (child_name == "") {
            const bool needsQuery = id == "";
            properties.addChild(object, needsQuery);
        } else {
            properties.set<Object>(child_name, object);
        }
    }

    void close() override {
        ref<Object> object = transform ? transform : Registry::create(tag, type, properties);
        if (id != "") {
            object->setId(id);
            getRoot().nameObject(id, object);
        }

        parent->addChild(object, name);
    }
};

struct SceneParser::PrimitiveNode : public SceneParser::Node {
    std::string tag;
    std::string name;
    std::string value;

    PrimitiveNode(const std::string &tag, const ref<Node> &parent) : Node(parent), tag(tag) {}

    static bool supportsTag(const std::string &tag) {
        return tag == "float" || tag == "string" || tag == "color" || tag == "boolean" || tag == "integer" || tag == "vector";
    }

    void attribute(const std::string &attr_key, const std::string &attr_value) override {
        if (attr_key == "name") {
            this->name = attr_value;
        } else if (attr_key == "value") {
            this->value = attr_value;
        } else {
            lightwave_throw("unsupported attribute \"%s\" on <%s />", attr_key, tag);
        }
    }

    void close() override {
        auto parent_node = dynamic_cast<ObjectNode *>(this->parent.get());
        if (!parent_node) {
            lightwave_throw("parameters can only be specified on objects");
        }

        if (tag == "float") {
            parent_node->properties.set(name, parse_string<float>(value));
        } else if (tag == "string") {
            parent_node->properties.set(name, value);
        } else if (tag == "color") {
            parent_node->properties.set(name, parse_string<Color>(value));
        } else if (tag == "boolean") {
            parent_node->properties.set(name, parse_string<bool>(value));
        } else if (tag == "integer") {
            parent_node->properties.set(name, parse_string<int>(value));
        } else if (tag == "vector") {
            parent_node->properties.set(name, parse_string<Vector>(value));
        }
    }
};

struct SceneParser::IncludeNode : public SceneParser::Node {
    std::string filename;
    std::filesystem::path filepath;

    IncludeNode(const ref<Node> &parent) : Node(parent) {}

    std::filesystem::path getFilePath() const override {
        return filepath;
    }

    void attribute(const std::string &key, const std::string &value) override {
        if (key == "filename") {
            filename = value;
        } else {
            lightwave_throw("unsupported attribute \"%s\" on <include />", key);
        }
    }

    void addChild(const ref<Object> &object, const std::string &name) override {
        parent->addChild(object, name);
    }

    void close() override {
        filepath = parent->getFilePath().remove_filename() / filename;
        XMLParser(getRoot().sceneParser, filepath);
    }
};

struct SceneParser::ReferenceNode : public SceneParser::Node {
    std::string id;
    std::string name;

    ReferenceNode(const ref<Node> &parent) : Node(parent) {}

    void attribute(const std::string &key, const std::string &value) override {
        if (key == "id") id = value; else
        if (key == "name") name = value; else
        {
            lightwave_throw("unsupported attribute \"%s\" on <ref />", key);
        }
    }

    void close() override {
        this->parent->addChild(getRoot().lookup(id), name);
    }
};

struct SceneParser::TransformNode : public SceneParser::Node {
    std::string tag;
    Transform *transform;

    // for matrix
    Matrix4x4 matrix;
    
    // for scale, translate
    Vector value;
    
    // for lookat
    Vector origin, target, up;
    
    // for rotate
    Vector axis;
    float angle;

    TransformNode(const std::string &tag, const ref<Node> &parent) : Node(parent), tag(tag) {
        if (auto p = dynamic_cast<ObjectNode *>(parent.get())) {
            transform = p->transform.get();
        }

        if (!transform) {
            lightwave_throw("<%s /> can only be applied on <transform />", tag);
        }

        if (tag == "translate") value = Vector(0); else
        if (tag == "scale") value = Vector(1);
    }

    static bool supportsTag(const std::string &tag) {
        return tag == "matrix" || tag == "translate" || tag == "scale" || tag == "rotate" || tag == "lookat";
    }

    void attribute(const std::string &attr_key, const std::string &attr_value) override {
        if (tag == "matrix") {
            if (attr_key == "value") { this->matrix = parse_string<Matrix4x4>(attr_value); return; }
        }

        if (tag == "translate" || tag == "scale") {
            if (attr_key == "x") { this->value.x() = parse_string<float>(attr_value); return; }
            if (attr_key == "y") { this->value.y() = parse_string<float>(attr_value); return; }
            if (attr_key == "z") { this->value.z() = parse_string<float>(attr_value); return; }
            if (attr_key == "value") { this->value = parse_string<Vector>(attr_value); return; }
        }

        if (tag == "rotate") {
            if (attr_key == "axis") { this->axis = parse_string<Vector>(attr_value); return; }
            if (attr_key == "angle") { this->angle = parse_string<float>(attr_value); return; }
        }

        if (tag == "lookat") {
            if (attr_key == "origin") { this->origin = parse_string<Vector>(attr_value); return; }
            if (attr_key == "target") { this->target = parse_string<Vector>(attr_value); return; }
            if (attr_key == "up") { this->up = parse_string<Vector>(attr_value); return; }
        }

        lightwave_throw("unsupported attribute \"%s\" on <%s />", attr_key, tag);
    }

    void close() override {
        if (tag == "matrix") transform->matrix(matrix); else
        if (tag == "translate") transform->translate(value); else
        if (tag == "scale") transform->scale(value); else
        if (tag == "rotate") transform->rotate(axis, angle * Pi / 180); else
        if (tag == "lookat") transform->lookat(origin, target, up);
    }
};

void SceneParser::open(const std::string &tag) {
    auto parent = m_stack.top();
    if (tag == "include") {
        m_stack.push(std::make_shared<IncludeNode>(parent));
    } else if (tag == "ref") {
        m_stack.push(std::make_shared<ReferenceNode>(parent));
    } else if (PrimitiveNode::supportsTag(tag)) {
        m_stack.push(std::make_shared<PrimitiveNode>(tag, parent));
    } else if (TransformNode::supportsTag(tag)) {
        m_stack.push(std::make_shared<TransformNode>(tag, parent));
    } else {
        m_stack.push(std::make_shared<ObjectNode>(tag, parent));
    }
}

void SceneParser::enter() {
    m_stack.top()->enter();
}

std::string SceneParser::resolveVariables(const std::string &value) {
    std::string result = "";
    size_t j = value.size();
    for (size_t i = 0; i < j; i++) {
        if (i < (j - 1) && value.at(i) == '$' && value.at(i + 1) == '{') {
            std::string varName = "";
            
            i += 2;
            while (true) {
                if (i >= j) {
                    lightwave_throw("expected end of variable '}'")
                }

                const char chr = value.at(i++);
                if (chr == '}') break;
                varName += chr;
            }

            lightwave_throw("unknown variable \"%s\"", varName);
        } else {
            result += value.at(i);
        }
    }
    return result;
}

void SceneParser::attribute(const std::string &name, const std::string &value) {
    m_stack.top()->attribute(name, resolveVariables(value));
}

void SceneParser::close() {
    auto ctx = m_stack.top();
    ctx->close();
    m_stack.pop();
}

SceneParser::SceneParser(const std::filesystem::path &path) {
    m_stack.push(std::make_shared<RootNode>(m_objects, path, *this));
    XMLParser(*this, path);
}

std::vector<ref<Object>> SceneParser::objects() const { return m_objects; }

}
