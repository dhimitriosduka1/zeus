#include <lightwave/registry.hpp>
#include <lightwave/shape.hpp>

namespace lightwave {

Registry::ConstructorMap &Registry::constructors() {
    static ConstructorMap constructors;
    return constructors;
}

bool Registry::exists(const std::string &category) {
    auto &constr = Registry::constructors();
    auto cat = constr.find(category);
    return cat != constr.end();
}

bool Registry::exists(const std::string &category, const std::string &name) {
    auto &constr = Registry::constructors();
    auto cat = constr.find(category);
    if (cat == constr.end()) return false;
    return cat->second.find(name) != cat->second.end();
}

void Registry::listAvailable(std::ostream &os) {
    for (auto &kv : constructors()) {
        os << "Plugin category '" << kv.first << "':" << std::endl;
        for (auto &kv2 : kv.second) {
            os << "* '" << kv2.first << "'" << std::endl;
        }
    }
}

ref<Object> Registry::create(const std::string &category, const std::string &name, const Properties &properties) {
    if (!exists(category)) {
        lightwave_throw("unknown node <%s />", category);
    }
    
    std::string qualifiedName = name;
    if (qualifiedName == "") {
        if (exists(category, "default")) {
            qualifiedName = "default";
        } else {
            lightwave_throw("<%s /> requires a type=\"...\" to be specified", category);
        }
    }
    
    if (!exists(category, qualifiedName)) {
        lightwave_throw("<%s /> with unknown type=\"%s\"", category, qualifiedName);
    }

    auto constructor = Registry::constructors()[category][qualifiedName];
    return constructor(properties);
}

}
