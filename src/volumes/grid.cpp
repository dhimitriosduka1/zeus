#include <lightwave.hpp>
#include <fstream>

namespace lightwave {

/// @brief Volume loaded from a grid of float densities
class GridVolume : public Volume {
    enum class FilterMode {
        Nearest,
        Trilinear
    };
    
    Vector3i m_resolution;
    std::vector<float> m_grid;
    float m_max_value; // Maximum density along the volume 
    FilterMode m_filter;

public:
    GridVolume(const Properties &properties) {
        // Density multiplier
        float multiplier = properties.get<float>("multiplier");

        m_filter = properties.getEnum<FilterMode>(
                    "filter", FilterMode::Trilinear,
                    {
                            {"nearest",  FilterMode::Nearest},
                            {"trilinear", FilterMode::Trilinear},
                    });

        // TODO: Use filepath instead of string
        auto path = properties.get<std::string>("filename");
        std::ifstream input(path, std::ios::binary);

        if (!input.is_open()) {
            std::cerr << "Error opening the file!" << std::endl;
            exit(1);
        }

        float fx, fy, fz;
        input.read(reinterpret_cast<char*>(&fx), sizeof(float));
        input.read(reinterpret_cast<char*>(&fy), sizeof(float));
        input.read(reinterpret_cast<char*>(&fz), sizeof(float));
        int x = fx, y = fy, z = fz;

        logger(EInfo, "loading volume \"%s\" (%d x %d x %d)", path, x, y, z);
        m_resolution = Vector3i(x, y, z);
        int voxelCount = x * y * z;
        m_grid.resize(voxelCount);
        m_max_value = 0.f;
        for (int i = 0; i < voxelCount; i++) {
            float v;
            input.read(reinterpret_cast<char*>(&v), sizeof(float));
            m_grid[i] = v * multiplier;
            if (m_grid[i] > m_max_value)
                m_max_value = m_grid[i];
        }
        logger(EInfo, "loaded volume with %d voxels", voxelCount);
    }

    float evaluate(const Point &pos) const override {
        Point p = Point(pos.x() * 0.5f + 0.5f, pos.y() * 0.5f + 0.5f, pos.z() * 0.5f + 0.5f);
        if (p.x() > 1.f || p.x() < 0.f || p.y() > 1.f || p.y() < 0.f || p.z() > 1.f || p.z() < 0.f) return 0.f;
        const Point dc = {p.x() * (float) m_resolution.x(), (1.f - p.y()) * (float) m_resolution.y(), p.z() * (float) m_resolution.z()};
        const int x = floor(dc.x());
        const int y = floor(dc.y());
        const int z = floor(dc.z());
        // int parity = x + y + z;
        // return parity % 2 == 0 ? 1.f : 0.f;

        if (m_filter == FilterMode::Nearest) {
            return getValueAt(x, y, z);
        }

        const Point fc = {dc.x() - x, dc.y() - y, dc.z() - z};
        const float v000 = getValueAt(x, y, z);
        const float v001 = getValueAt(x, y, z + 1);
        const float v010 = getValueAt(x, y + 1, z);
        const float v011 = getValueAt(x, y + 1, z + 1);
        const float v100 = getValueAt(x + 1, y, z);
        const float v101 = getValueAt(x + 1, y, z + 1);
        const float v110 = getValueAt(x + 1, y + 1, z);
        const float v111 = getValueAt(x + 1, y + 1, z + 1);

        const float v00 = lerp(v000, v100, fc.x());
        const float v01 = lerp(v001, v101, fc.x());
        const float v10 = lerp(v010, v110, fc.x());
        const float v11 = lerp(v011, v111, fc.x());

        const float v0 = lerp(v00, v01, fc.z());
        const float v1 = lerp(v10, v11, fc.z());

        const float v = lerp(v0, v1, fc.y());

        return v;
    }

    float getMaxDensity() const override { return m_max_value; }

    std::string toString() const override {
        return tfm::format("GridVolume[\n"
                           "  max_value = %s\n"
                           "]",
                           indent(m_max_value));
    }
private:
    float getValueAt(int x, int y, int z) const {
        if (x > m_resolution.x() - 1 || y > m_resolution.y() - 1 || z > m_resolution.z() - 1) {
            return 0.f;
        }
        return m_grid[(z * m_resolution.x() * m_resolution.y()) + (y * m_resolution.x()) + x];
    }
};

}

REGISTER_VOLUME(GridVolume, "grid")
