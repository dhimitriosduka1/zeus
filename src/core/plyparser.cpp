#include "plyparser.hpp"
#include <lightwave/logger.hpp>

#include <climits>
#include <fstream>

namespace lightwave {

// https://stackoverflow.com/questions/105252/how-do-i-convert-between-big-endian-and-little-endian-values-in-c
template <typename T>
inline T swap_endian(T u) {
    static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");

    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

struct Header {
    int VertexCount       = 0;
    int FaceCount         = 0;
    int XElem             = -1;
    int YElem             = -1;
    int ZElem             = -1;
    int NXElem            = -1;
    int NYElem            = -1;
    int NZElem            = -1;
    int UElem             = -1;
    int VElem             = -1;
    int VertexPropCount   = 0;
    int IndElem           = -1;
    int MatElem           = -1;
    bool SwitchEndianness = false;
    bool IsAscii          = false;

    [[nodiscard]] inline bool hasVertices() const { return XElem >= 0 && YElem >= 0 && ZElem >= 0; }
    [[nodiscard]] inline bool hasNormals() const { return NXElem >= 0 && NYElem >= 0 && NZElem >= 0; }
    [[nodiscard]] inline bool hasUVs() const { return UElem >= 0 && VElem >= 0; }
    [[nodiscard]] inline bool hasIndices() const { return IndElem >= 0; }
    [[nodiscard]] inline bool hasMaterials() const { return MatElem >= 0; }
};

static void readPlyContent(
    std::istream& stream, const Header& header, 
    std::vector<Vector3i> &indices,
    std::vector<Vertex> &vertices
) {
    const auto readFloat = [&]() {
        float val = 0;
        stream.read(reinterpret_cast<char*>(&val), sizeof(val));
        if (header.SwitchEndianness)
            val = swap_endian<float>(val);
        return val;
    };

    const auto readIdx = [&]() {
        uint32_t val = 0;
        stream.read(reinterpret_cast<char*>(&val), sizeof(val));
        if (header.SwitchEndianness)
            val = swap_endian<uint32_t>(val);
        return val;
    };

    vertices.reserve(header.VertexCount);
    if (!header.hasNormals()) lightwave_throw("no normals found");

    for (int i = 0; i < header.VertexCount; ++i) {
        float x = 0, y = 0, z = 0;
        float nx = 0, ny = 0, nz = 0;
        float u = 0, v = 0;

        if (header.IsAscii) {
            std::string line;
            if (!std::getline(stream, line))
                lightwave_throw("not enough vertices given");

            std::stringstream sstream(line);
            int elem = 0;
            while (sstream) {
                float val = 0;
                sstream >> val;

                if      (header.XElem  == elem) x = val;
                else if (header.YElem  == elem) y = val;
                else if (header.ZElem  == elem) z = val;
                else if (header.NXElem == elem) nx = val;
                else if (header.NYElem == elem) ny = val;
                else if (header.NZElem == elem) nz = val;
                else if (header.UElem  == elem) u = val;
                else if (header.VElem  == elem) v = val;

                elem++;
            }
        } else {
            for (int elem = 0; elem < header.VertexPropCount; ++elem) {
                float val = readFloat();
                if      (header.XElem  == elem) x = val;
                else if (header.YElem  == elem) y = val;
                else if (header.ZElem  == elem) z = val;
                else if (header.NXElem == elem) nx = val;
                else if (header.NYElem == elem) ny = val;
                else if (header.NZElem == elem) nz = val;
                else if (header.UElem  == elem) u = val;
                else if (header.VElem  == elem) v = val;
            }
        }

        Vertex vertex;
        vertex.position = { x, y, z };
        vertex.normal = Vector(nx, ny, nz).normalized();
        vertex.texcoords = Vector2(u, v);
        vertices.push_back(vertex);
    }

    if (vertices.empty()) lightwave_throw("no vertices found");

    indices.resize(header.FaceCount);
    size_t indicesIndex = 0;

    if (header.IsAscii) {
        for (int i = 0; i < header.FaceCount; ++i) {
            std::string line;
            if (!std::getline(stream, line)) 
                lightwave_throw("not enough indices given");

            std::stringstream sstream(line);

            uint32_t elems = 0;
            sstream >> elems;
            if (elems != 3) lightwave_throw("only triangles supported");

            for (uint32_t elem = 0; elem < elems; ++elem) {
                sstream >> indices[indicesIndex][elem];
            }
            indicesIndex++;
        }
    } else {
        for (int i = 0; i < header.FaceCount; ++i) {
            uint8_t elems = 0;
            stream.read(reinterpret_cast<char*>(&elems), sizeof(elems));
            if (elems != 3) lightwave_throw("only triangles supported");

            for (uint32_t elem = 0; elem < elems; ++elem) {
                indices[indicesIndex][elem] = readIdx();
            }
            indicesIndex++;
        }
    }

    if (indicesIndex != indices.size()) lightwave_throw("too few faces (%d found, %d needed)", indicesIndex, indices.size());

    if (!header.hasUVs()) {
        Bounds bbox;
        for (const Vertex &v : vertices) bbox.extend(v.position);

        for (size_t i = 0; i < vertices.size(); ++i) {
            auto &v = vertices.at(i);
            const Vector d = bbox.diagonal();
            const Vector t = v.position - bbox.min();

            Vector2 p = Vector2(0);
            if (d.x() > Epsilon) p.x() = t.x() / d.x();
            if (d.y() > Epsilon) p.y() = t.y() / d.y();
            v.texcoords = p; // Drop the z coordinate
        }

    }
}

static inline bool isAllowedVertIndType(const std::string& str) {
    return str == "uchar"
           || str == "int"
           || str == "uint8_t"
           || str == "uint";
}

void readPLY(
    const std::filesystem::path &path,
    std::vector<Vector3i> &indices,
    std::vector<Vertex> &vertices
) {
    logger(EInfo, "loading mesh %s", path);
    try {
        std::fstream stream(path, std::ios::in | std::ios::binary);
        if (!stream)
            lightwave_throw("error opening file");

        // Header
        std::string magic;
        stream >> magic;
        if (magic != "ply")
            lightwave_throw("file is not in PLY format");

        std::string method;
        Header header;

        int facePropCounter = 0;
        for (std::string line; std::getline(stream, line);) {
            std::stringstream sstream(line);

            std::string action;
            sstream >> action;
            if (action == "comment")
                continue;
            else if (action == "format") {
                sstream >> method;
            } else if (action == "element") {
                std::string type;
                sstream >> type;
                if (type == "vertex")
                    sstream >> header.VertexCount;
                else if (type == "face")
                    sstream >> header.FaceCount;
            } else if (action == "property") {
                std::string type;
                sstream >> type;
                if (type == "float") {
                    std::string name;
                    sstream >> name;
                    if      (name == "x" ) header.XElem  = header.VertexPropCount;
                    else if (name == "y" ) header.YElem  = header.VertexPropCount;
                    else if (name == "z" ) header.ZElem  = header.VertexPropCount;
                    else if (name == "nx") header.NXElem = header.VertexPropCount;
                    else if (name == "ny") header.NYElem = header.VertexPropCount;
                    else if (name == "nz") header.NZElem = header.VertexPropCount;
                    else if (name == "u" || name == "s") header.UElem = header.VertexPropCount;
                    else if (name == "v" || name == "t") header.VElem = header.VertexPropCount;
                    ++header.VertexPropCount;
                } else if (type == "list") {
                    ++facePropCounter;

                    std::string countType;
                    sstream >> countType;

                    std::string indType;
                    sstream >> indType;

                    std::string name;
                    sstream >> name;
                    if (!isAllowedVertIndType(countType)) {
                        lightwave_throw("only 'property list uchar int' is supported");
                        continue;
                    }

                    if (name == "vertex_indices" || name == "vertex_index")
                        header.IndElem = facePropCounter - 1;
                } else {
                    lightwave_throw("only float or list properties allowed");
                    ++header.VertexPropCount;
                }
            } else if (action == "end_header")
                break;
        }

        // Content
        if (!header.hasVertices() || !header.hasIndices() || header.VertexCount <= 0 || header.FaceCount <= 0)
            lightwave_throw("does not contain valid mesh data");

        header.SwitchEndianness = (method == "binary_big_endian");
        header.IsAscii          = (method == "ascii");
        readPlyContent(stream, header, indices, vertices);
    } catch (...) {
        lightwave_throw_nested("while parsing %s", path);
    }
}

}
