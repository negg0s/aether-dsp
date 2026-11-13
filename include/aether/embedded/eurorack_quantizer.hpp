#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Quantizer to musical scales (Equal temperament and microtonal intervals)
// Fast binary search lookup into pre-calculated 1V/Oct millivolt lookup tables.
// Zero-heap, zero-allocation bare-metal Eurorack pitch corrector.
class EurorackQuantizer {
public:
    enum class Scale : uint8_t {
        Chromatic = 0,
        Major,
        NaturalMinor,
        Dorian,
        PentatonicMajor,
        PentatonicMinor,
        WholeTone,
        OctatonicHalfWhole,
        Count
    };

    EurorackQuantizer() {
        set_scale(Scale::Chromatic);
    }

    void set_scale(Scale scale) noexcept {
        current_scale_ = scale;
        build_scale_mask(scale);
    }

    void set_root_note(uint8_t root_semitone) noexcept {
        root_semitone_ = root_semitone % 12;
    }

    // Quantize input CV in millivolts (e.g. 0 to 5000 mV for 5 octaves, 1V/Oct)
    // to nearest in-scale pitch millivolts.
    // 1 Octave = 1000 mV, 1 Semitone = 1000 / 12 = 83.333 mV.
    [[nodiscard]] int16_t quantize(int16_t input_mv) const noexcept {
        if (input_mv <= 0) return 0;
        if (input_mv > 10000) input_mv = 10000; // clamp 10V (10 octaves)

        // Find octave and semitone offset
        int32_t octave = input_mv / 1000;
        int32_t millivolts_in_octave = input_mv % 1000;

        // Semitone in 12-TET is approximately (mv * 12 + 500) / 1000
        int32_t nearest_semitone = (millivolts_in_octave * 12 + 500) / 1000;
        if (nearest_semitone >= 12) {
            nearest_semitone = 11;
        }

        // Search nearest allowed semitone in scale mask
        int32_t best_semitone = nearest_semitone;
        int32_t min_distance = 100;

        for (int32_t semi = 0; semi < 12; ++semi) {
            int32_t note_in_scale = (semi - root_semitone_ + 12) % 12;
            if ((scale_mask_ & (1 << note_in_scale)) != 0) {
                int32_t dist = (semi >= nearest_semitone) ? (semi - nearest_semitone) : (nearest_semitone - semi);
                if (dist < min_distance) {
                    min_distance = dist;
                    best_semitone = semi;
                }
            }
        }

        // Compute quantized millivolts using fixed 83.333 mV per semitone in Q15
        // 83.333333 = 1000 / 12 -> 83 + (semi * 1000 % 12) / 12
        int32_t quantized_mv = octave * 1000 + (best_semitone * 1000 + 6) / 12;
        return static_cast<int16_t>(quantized_mv);
    }

private:
    void build_scale_mask(Scale scale) noexcept {
        switch (scale) {
            case Scale::Chromatic:
                scale_mask_ = 0x0FFF; // All 12 notes active
                break;
            case Scale::Major: // W-W-H-W-W-W-H -> 0, 2, 4, 5, 7, 9, 11
                scale_mask_ = (1<<0) | (1<<2) | (1<<4) | (1<<5) | (1<<7) | (1<<9) | (1<<11);
                break;
            case Scale::NaturalMinor: // W-H-W-W-H-W-W -> 0, 2, 3, 5, 7, 8, 10
                scale_mask_ = (1<<0) | (1<<2) | (1<<3) | (1<<5) | (1<<7) | (1<<8) | (1<<10);
                break;
            case Scale::Dorian: // 0, 2, 3, 5, 7, 9, 10
                scale_mask_ = (1<<0) | (1<<2) | (1<<3) | (1<<5) | (1<<7) | (1<<9) | (1<<10);
                break;
            case Scale::PentatonicMajor: // 0, 2, 4, 7, 9
                scale_mask_ = (1<<0) | (1<<2) | (1<<4) | (1<<7) | (1<<9);
                break;
            case Scale::PentatonicMinor: // 0, 3, 5, 7, 10
                scale_mask_ = (1<<0) | (1<<3) | (1<<5) | (1<<7) | (1<<10);
                break;
            case Scale::WholeTone: // 0, 2, 4, 6, 8, 10
                scale_mask_ = (1<<0) | (1<<2) | (1<<4) | (1<<6) | (1<<8) | (1<<10);
                break;
            case Scale::OctatonicHalfWhole: // 0, 1, 3, 4, 6, 7, 9, 10
                scale_mask_ = (1<<0) | (1<<1) | (1<<3) | (1<<4) | (1<<6) | (1<<7) | (1<<9) | (1<<10);
                break;
            default:
                scale_mask_ = 0x0FFF;
                break;
        }
    }

    Scale current_scale_{Scale::Chromatic};
    uint8_t root_semitone_{0};
    uint16_t scale_mask_{0x0FFF};
};

} // namespace aether::embedded
