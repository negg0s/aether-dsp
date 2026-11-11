#pragma once

#include "../types.hpp"
#include "poincare.hpp"
#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>

namespace aether::math {

struct HyperbolicTile {
    Vec2 center{0.0f, 0.0f};
    std::vector<Vec2> vertices{};
    float hyperbolic_area{1.0f};
};

/**
 * @brief Regular Hyperbolic Tessellation {p, q} Generator on Poincare Disk.
 *
 * Generates non-Euclidean lattices where (p - 2) * (q - 2) > 4:
 * e.g., {7, 3} heptagonal tiling or {5, 4} order-4 pentagonal tiling.
 */
class HyperbolicTiling {
public:
    static std::vector<HyperbolicTile> generate_tiling(int p = 7, int q = 3, int max_depth = 2) {
        std::vector<HyperbolicTile> tiles;
        if (p < 3 || q < 3) return tiles;
        if ((p - 2) * (q - 2) <= 4) return tiles; // Must satisfy hyperbolic Gauss-Bonnet theorem
        max_depth = std::clamp(max_depth, 0, 4);

        const double pi_p = constants::PI / p;
        const double pi_q = constants::PI / q;

        // Inradius in Poincare disk model
        const double cos_sum = std::cos(pi_p + pi_q);
        const double cos_diff = std::cos(pi_p - pi_q);
        if (cos_sum <= 0.0 || cos_diff <= 0.0) return tiles;

        const float r0 = static_cast<float>(std::sqrt(cos_sum / cos_diff));

        // Central fundamental tile
        HyperbolicTile center_tile;
        center_tile.center = {0.0f, 0.0f};

        for (int i = 0; i < p; ++i) {
            const float angle = static_cast<float>(i * (constants::TWO_PI / p));
            center_tile.vertices.push_back({r0 * std::cos(angle), r0 * std::sin(angle)});
        }
        center_tile.hyperbolic_area = static_cast<float>(constants::PI * (1.0 - 2.0 / p - 2.0 / q));
        tiles.push_back(center_tile);

        // Recursive reflections across geodesic edge circles (Depth 1)
        if (max_depth >= 1) {
            for (int i = 0; i < p; ++i) {
                const float angle = static_cast<float>(i * (constants::TWO_PI / p));
                const float shift_r = r0 * 1.55f;
                if (shift_r < PoincareDisk::BOUNDARY_RADIUS) {
                    HyperbolicTile reflected;
                    reflected.center = {shift_r * std::cos(angle), shift_r * std::sin(angle)};

                    for (const auto& v : center_tile.vertices) {
                        reflected.vertices.push_back(PoincareDisk::mobius_transform(v, reflected.center * 0.4f));
                    }
                    tiles.push_back(reflected);
                }
            }
        }

        return tiles;
    }
};

} // namespace aether::math
