#pragma once

#include "types.hpp"
#include "engines/chladni_plate.hpp"
#include "math/hyperbolic_tiling.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>

namespace aether {

/**
 * @brief High-Resolution SVG Vector Graphics Exporter (Pure C++20).
 *
 * Exports mathematical nodal curves and non-Euclidean hyperbolic tilings
 * as crisp, scalable vector graphics (.svg) for visualization, documentation, and album art.
 */
class SvgExporter {
public:
    /**
     * @brief Export 2D Chladni Nodal Plate Curves as SVG vector art.
     */
    static bool export_chladni_svg(const engines::ChladniPlate& plate,
                                   const std::string& filepath,
                                   int width = 800,
                                   int height = 800) {
        std::ofstream out(filepath);
        if (!out.is_open()) return false;

        out << R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 )"
            << width << " " << height
            << R"(" width=")" << width << R"(" height=")" << height
            << R"(" style="background:#07090e;">)" << "\n";

        // Definitions for gradients and glow filters
        out << R"(<defs>
    <filter id="glow" x="-20%" y="-20%" width="140%" height="140%">
        <feGaussianBlur stdDeviation="3" result="blur" />
        <feMerge>
            <feMergeNode in="blur" />
            <feMergeNode in="SourceGraphic" />
        </feMerge>
    </filter>
    <linearGradient id="neonGrad" x1="0%" y1="0%" x2="100%" y2="100%">
        <stop offset="0%" stop-color="#00f0ff" />
        <stop offset="50%" stop-color="#ffd700" />
        <stop offset="100%" stop-color="#ff0077" />
    </linearGradient>
</defs>)" << "\n";

        // Outer plate border
        out << R"(<rect x="20" y="20" width=")" << (width - 40) << R"(" height=")" << (height - 40)
            << R"(" rx="16" fill="#0d111a" stroke="#00f0ff" stroke-opacity="0.3" stroke-width="2" />)" << "\n";

        constexpr int GRID = 160;
        const float dx = static_cast<float>(width - 80) / GRID;
        const float dy = static_cast<float>(height - 80) / GRID;

        // Marching squares / point lattice sampling for nodal lines where |u| < epsilon
        for (int gy = 0; gy < GRID; ++gy) {
            const float ny = (static_cast<float>(gy) / (GRID - 1)) * 2.0f - 1.0f;
            for (int gx = 0; gx < GRID; ++gx) {
                const float nx = (static_cast<float>(gx) / (GRID - 1)) * 2.0f - 1.0f;
                const float val = std::abs(plate.evaluate_field({nx, ny}));

                if (val < 0.045f) {
                    const float sx = 40.0f + gx * dx;
                    const float sy = 40.0f + gy * dy;
                    out << "<circle cx=\"" << std::fixed << std::setprecision(1) << sx
                        << "\" cy=\"" << sy
                        << "\" r=\"1.6\" fill=\"#ffd700\" filter=\"url(#glow)\" />\n";
                }
            }
        }

        out << "</svg>\n";
        return true;
    }

    /**
     * @brief Export Poincare Disk Hyperbolic Tessellation as SVG.
     */
    static bool export_poincare_tiling_svg(const std::vector<math::HyperbolicTile>& tiles,
                                           const std::string& filepath,
                                           int size = 800) {
        std::ofstream out(filepath);
        if (!out.is_open()) return false;

        const float cx = size * 0.5f;
        const float cy = size * 0.5f;
        const float r_disk = size * 0.44f;

        out << R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 )"
            << size << " " << size << R"(" width=")" << size << R"(" height=")" << size
            << R"(" style="background:#05070a;">)" << "\n";

        // Boundary unit circle
        out << R"(<circle cx=")" << cx << R"(" cy=")" << cy << R"(" r=")" << r_disk
            << R"(" fill="#0a0d14" stroke="#00f0ff" stroke-width="2.5" />)" << "\n";

        // Render tiles
        for (const auto& tile : tiles) {
            if (tile.vertices.empty()) continue;

            out << R"(<polygon points=")";
            for (const auto& v : tile.vertices) {
                const float sx = cx + v.x * r_disk;
                const float sy = cy + v.y * r_disk;
                out << std::fixed << std::setprecision(1) << sx << "," << sy << " ";
            }
            out << "\" fill=\"rgba(0, 240, 255, 0.08)\" stroke=\"#00f0ff\" stroke-opacity=\"0.4\" stroke-width=\"1.2\" />\n";
        }

        out << "</svg>\n";
        return true;
    }
};

} // namespace aether
