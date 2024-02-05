#include <lightwave/math.hpp>

#include <string>
#include <vector>
#include <filesystem>

namespace lightwave {

void readPLY(
    const std::filesystem::path &path,
    std::vector<Vector3i> &indices,
    std::vector<Vertex> &vertices
);

}
