#pragma once

#include "tuning.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <array>

namespace aether::dsp {

struct ScaleDefinition {
    const char* name;
    const char* description;
    double period;
    std::vector<double> ratios;
};

/**
 * @brief Built-in Exotic & Microtonal Scale Catalog.
 */
class ScalaCatalog {
public:
    static const std::vector<ScaleDefinition>& get_all_scales() {
        static const std::vector<ScaleDefinition> catalog = {
            {
                "12-EDO",
                "Standard 12-Tone Equal Temperament (Euclidean reference)",
                2.0,
                {1.0, 1.059463, 1.122462, 1.189207, 1.259921, 1.334840, 1.414214, 1.498307, 1.587401, 1.681793, 1.781797, 1.887749}
            },
            {
                "Bohlen-Pierce",
                "13-tone equal division of 3:1 tritave (perfect for inharmonic plates)",
                3.0,
                {1.0, 1.08818, 1.18414, 1.28856, 1.40219, 1.52583, 1.66038, 1.80678, 1.96611, 2.13947, 2.32812, 2.53341, 2.75681}
            },
            {
                "Wendy-Carlos-Alpha",
                "Non-octave scale with 78.0 cents per step (15.385 steps per octave)",
                2.0,
                {1.0, 1.0461, 1.0943, 1.1448, 1.1976, 1.2528, 1.3106, 1.3711, 1.4343, 1.5005, 1.5697, 1.6421, 1.7179, 1.7971, 1.8800}
            },
            {
                "Wendy-Carlos-Beta",
                "Non-octave scale with 63.8 cents per step (18.809 steps per octave)",
                2.0,
                {1.0, 1.0375, 1.0764, 1.1168, 1.1587, 1.2022, 1.2473, 1.2941, 1.3427, 1.3931, 1.4454, 1.4996, 1.5559, 1.6143, 1.6749, 1.7378, 1.8030, 1.8707}
            },
            {
                "Gamelan-Slendro",
                "Traditional Javanese/Balinese 5-tone equidistant bronze gong tuning",
                2.0,
                {1.0, 1.1487, 1.3195, 1.5157, 1.7411}
            },
            {
                "Gamelan-Pelog",
                "Traditional Indonesian 7-tone unequal metallophone tuning",
                2.0,
                {1.0, 1.0801, 1.1718, 1.3484, 1.4582, 1.5794, 1.8153}
            },
            {
                "Partch-Just-Intonation",
                "Harry Partch pure integer acoustic ratios (11-limit subset)",
                2.0,
                {1.0, 16.0/15.0, 10.0/9.0, 9.0/8.0, 8.0/7.0, 7.0/6.0, 6.0/5.0, 5.0/4.0, 4.0/3.0, 7.0/5.0, 3.0/2.0, 8.0/5.0, 5.0/3.0, 7.0/4.0, 15.0/8.0}
            },
            {
                "Fibonacci-Phi",
                "Microtonal geometric progression based on powers of the golden ratio",
                1.61803398875,
                {1.0, 1.0620, 1.1278, 1.1977, 1.2720, 1.3508, 1.4346, 1.5235}
            }
        };
        return catalog;
    }

    [[nodiscard]] static const ScaleDefinition* find_by_name(std::string_view name) noexcept {
        const auto& all = get_all_scales();
        for (const auto& scale : all) {
            if (name == scale.name) {
                return &scale;
            }
        }
        return nullptr;
    }

    [[nodiscard]] static const ScaleDefinition& get_by_index(size_t index) noexcept {
        const auto& all = get_all_scales();
        if (index >= all.size()) {
            return all[0];
        }
        return all[index];
    }
};

} // namespace aether::dsp
