#include <lightwave/properties.hpp>
#include <string>

// adapted from https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cstring>
#include <cxxabi.h>

std::string lightwave::demangle(const char* name) {
    int status = 0;
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };
    if (status) return name;
    
    std::string result = res.get();
    if (result.rfind("lightwave::", 0) == 0) {
        // strip default namespace from typename
        result = result.substr(strlen("lightwave::"));
    }
    return result;
}
#else
std::string lightwave::demangle(const char* name) {
    // does nothing if not _GNUG_
    return name;
}
#endif

namespace lightwave {

float stof_substr(const std::string &str, size_t &index) {
    size_t tmp;
    auto result = float(std::stod(str.substr(index), &tmp));
    index += tmp;
    return result;
}

template<> float parse_string(const std::string &str) {
    return float(std::stod(str));
}

template<> int parse_string(const std::string &str) {
    return std::stoi(str);
}

template<> bool parse_string(const std::string &str) {
    if (str == "true") return true;
    if (str == "false") return false;
    lightwave_throw("cannot interpret string \"%s\" as boolean", str);
}

template<> Color parse_string(const std::string &str) {
    const auto vec = parse_string<Vector>(str);
    return Color(vec.x(), vec.y(), vec.z());
}

template<typename T> T parse_vector_string(const std::string &str) {
    T result;

    size_t i = 0;
    for (int dim = 0; dim < result.Dimension; dim++) {
        if (i >= str.size()) {
            if (dim == 1) {
                // if only a single value is specified, set all components to that value
                return T(result[0]);
            }
            lightwave_throw("expected more values");
        }
        if (i && str.at(i++) != ',') {
            lightwave_throw("expected ','");
        }
        result[dim] = stof_substr(str, i);
    }

    return result;
}

template<> Vector2 parse_string(const std::string &str) {
    return parse_vector_string<Vector2>(str);
}

template<> Vector parse_string(const std::string &str) {
    return parse_vector_string<Vector>(str);
}

template<> Point2 parse_string(const std::string &str) {
    return parse_vector_string<Point2>(str);
}

template<> Point parse_string(const std::string &str) {
    return parse_vector_string<Point>(str);
}

template<> Matrix4x4 parse_string(const std::string &str) {
    Matrix4x4 result;

    size_t i = 0;
    for (int row = 0; row < result.Rows; row++) {
        for (int column = 0; column < result.Columns; column++) {
            if (i >= str.size()) {
                lightwave_throw("expected more values");
            }
            if (i && str.at(i++) != ',') {
                lightwave_throw("expected ','");
            }
            result(row, column) = stof_substr(str, i);
        }
    }

    return result;
}

}
