#pragma once

#include <cstdint>
#include <cstddef>
#include "../math/fixed_point.hpp"

namespace aether {
namespace embedded {

/**
 * Eurorack Indonesian Gamelan Microtonal Quantizer.
 * Supports Slendro (5-tone near-equidistant pentatonic, ~240 cents step)
 * and Pelog (7-tone heptatonic scale with characteristic unequal step intervals).
 * In Eurorack 1V/Oct standard: 1V = 1200 cents = 1000 mV (1 cent = 1000/1200 mV = 5/6 mV).
 *
 * Fixed-point arithmetic, zero heap allocation.
 */
enum class GamelanMode : uint8_t {
    Slendro = 0, // 5 tones: 0, 240, 480, 720, 960 cents
    Pelog = 1    // 7 tones (Pelog Selisir / Bem typical cents: 0, 120, 270, 540, 670, 785, 950)
};

class EurorackSlendroPelog {
public:
    EurorackSlendroPelog()
        : mode_(GamelanMode::Slendro),
          hysteresis_mV_(8),
          last_quantized_mV_(0) {}

    void set_mode(GamelanMode mode) {
        mode_ = mode;
    }

    void set_hysteresis(int16_t hysteresis_mV) {
        hysteresis_mV_ = (hysteresis_mV < 0) ? 0 : ((hysteresis_mV > 50) ? 50 : hysteresis_mV);
    }

    // Process CV in millivolts (-5000 mV to +5000 mV for -5V to +5V range)
    int16_t process(int16_t cv_in_mV) {
        // Octave range is 1000 mV per octave
        int16_t octave = 0;
        int16_t note_mV = cv_in_mV;

        while (note_mV < 0) {
            note_mV += 1000;
            octave -= 1000;
        }
        while (note_mV >= 1000) {
            note_mV -= 1000;
            octave += 1000;
        }

        const int16_t* intervals = (mode_ == GamelanMode::Slendro) ? kSlendro_mV : kPelog_mV;
        size_t count = (mode_ == GamelanMode::Slendro) ? 5 : 7;

        // Find closest scale degree
        int16_t best_deg = intervals[0];
        int16_t min_dist = 2000;

        for (size_t i = 0; i < count; ++i) {
            int16_t dist = note_mV - intervals[i];
            if (dist < 0) dist = -dist;
            if (dist < min_dist) {
                min_dist = dist;
                best_deg = intervals[i];
            }
        }
        // Also check upper octave root (1000 mV)
        int16_t dist_top = 1000 - note_mV;
        if (dist_top < 0) dist_top = -dist_top;
        if (dist_top < min_dist) {
            best_deg = 1000;
        }

        int16_t candidate_out = octave + best_deg;

        // Apply Schmitt hysteresis
        int16_t diff = candidate_out - last_quantized_mV_;
        if (diff < 0) diff = -diff;
        if (diff > hysteresis_mV_) {
            last_quantized_mV_ = candidate_out;
        }

        return last_quantized_mV_;
    }

    void reset() {
        last_quantized_mV_ = 0;
    }

private:
    // Scale tables in millivolts (1000 mV = 1200 cents, mV = cents * 10 / 12)
    // Slendro 5 tones: 0, 240, 480, 720, 960 cents -> 0, 200, 400, 600, 800 mV
    static constexpr int16_t kSlendro_mV[5] = {
        0, 200, 400, 600, 800
    };

    // Pelog 7 tones (traditional Javanese / Balinese tuning):
    // 0, 120, 270, 540, 670, 785, 950 cents
    // mV conversions:
    // 0 * 10 / 12 = 0
    // 120 * 10 / 12 = 100
    // 270 * 10 / 12 = 225
    // 540 * 10 / 12 = 450
    // 670 * 10 / 12 = 558
    // 785 * 10 / 12 = 654
    // 950 * 10 / 12 = 792
    static constexpr int16_t kPelog_mV[7] = {
        0, 100, 225, 450, 558, 654, 792
    };

    GamelanMode mode_;
    int16_t hysteresis_mV_;
    int16_t last_quantized_mV_;
};

} // namespace embedded
} // namespace aether
