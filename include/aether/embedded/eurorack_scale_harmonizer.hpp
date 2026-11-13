#pragma once

#include "aether/math/fixed_point.hpp"
#include <cstdint>

namespace aether::embedded {

/**
 * @brief Fixed-point 16-step CV Pitch / Voltage Quantizer with Musical Scales and Hysteresis.
 * Converts raw continuous 1V/Oct analog control voltages (-5V to +5V) to discrete semitone / scale steps.
 * Designed for microcontrollers (STM32 / Daisy Patch), pure integer arithmetic, zero dynamic allocation.
 * Supported scales: Chromatic, Major, Natural Minor, Dorian, Pentatonic Major, Pentatonic Minor, Blues.
 */
class EurorackScaleHarmonizer {
public:
    enum class Scale : uint8_t {
        Chromatic = 0,
        Major = 1,
        NaturalMinor = 2,
        Dorian = 3,
        PentatonicMajor = 4,
        PentatonicMinor = 5,
        Blues = 6,
        Count = 7
    };

    EurorackScaleHarmonizer() {
        init_scale_masks();
        reset();
    }

    void reset() {
        scale_ = Scale::Chromatic;
        root_note_ = 0; // C
        hysteresis_mV_ = 10;
        last_quantized_mV_ = 0;
    }

    void set_scale(Scale sc) {
        if (static_cast<size_t>(sc) < static_cast<size_t>(Scale::Count)) {
            scale_ = sc;
        }
    }

    void set_root(int root) {
        root_note_ = ((root % 12) + 12) % 12;
    }

    void set_hysteresis(int16_t mV) {
        if (mV < 0) mV = 0;
        if (mV > 50) mV = 50;
        hysteresis_mV_ = mV;
    }

    /**
     * @brief Quantize 1V/Oct CV input in millivolts (-5000 mV to +5000 mV).
     * 1 Volt = 1000 mV = 1 Octave = 12 semitones.
     * 1 semitone = 1000 / 12 = 83.333 mV.
     */
    int16_t process(int16_t cv_in_mV) {
        // Apply Schmitt hysteresis around last quantized step
        int16_t diff = cv_in_mV - last_quantized_mV_;
        if (diff > -hysteresis_mV_ && diff < hysteresis_mV_) {
            return last_quantized_mV_;
        }

        // Convert mV to fractional semitones (in units of 1/24 semitone = 3.472 mV)
        // 1 semitone = 83.333 mV -> 83333 uV / 1000
        // Total semitones relative to 0V: (cv_in_mV * 12) / 1000
        int32_t total_semi = (static_cast<int32_t>(cv_in_mV) * 12 + ((cv_in_mV >= 0) ? 500 : -500)) / 1000;

        // Calculate octave and pitch class [0 .. 11]
        int32_t oct = total_semi / 12;
        int32_t pitch_class = total_semi % 12;
        if (pitch_class < 0) {
            pitch_class += 12;
            oct -= 1;
        }

        // Snap pitch class to current active musical scale mask
        uint16_t mask = scale_masks_[static_cast<size_t>(scale_)];
        // Rotate scale mask by root note
        uint16_t rotated_mask = 0;
        for (int i = 0; i < 12; ++i) {
            int note = (i + root_note_) % 12;
            if (mask & (1u << i)) {
                rotated_mask |= (1u << note);
            }
        }

        int32_t best_class = snap_to_mask(pitch_class, rotated_mask);
        int32_t quantized_semi = oct * 12 + best_class;

        // Convert back to exact millivolts: (semi * 1000) / 12
        int32_t out_mV = (quantized_semi * 1000) / 12;
        last_quantized_mV_ = static_cast<int16_t>(out_mV);
        return last_quantized_mV_;
    }

private:
    Scale scale_{Scale::Chromatic};
    int root_note_{0};
    int16_t hysteresis_mV_{10};
    int16_t last_quantized_mV_{0};
    uint16_t scale_masks_[static_cast<size_t>(Scale::Count)];

    void init_scale_masks() {
        // Bit i is 1 if note i is in scale (0 = tonic)
        scale_masks_[static_cast<size_t>(Scale::Chromatic)]       = 0x0FFF; // 111111111111
        scale_masks_[static_cast<size_t>(Scale::Major)]           = 0x0AB5; // 101010110101 (0, 2, 4, 5, 7, 9, 11)
        scale_masks_[static_cast<size_t>(Scale::NaturalMinor)]    = 0x05AD; // 101101011010 (0, 2, 3, 5, 7, 8, 10)
        scale_masks_[static_cast<size_t>(Scale::Dorian)]          = 0x05B5; // 101101010101 (0, 2, 3, 5, 7, 9, 10)
        scale_masks_[static_cast<size_t>(Scale::PentatonicMajor)] = 0x02A5; // 101001010001 (0, 2, 4, 7, 9)
        scale_masks_[static_cast<size_t>(Scale::PentatonicMinor)] = 0x0499; // 100100101001 (0, 3, 5, 7, 10)
        scale_masks_[static_cast<size_t>(Scale::Blues)]           = 0x04D9; // 100110101001 (0, 3, 5, 6, 7, 10)
    }

    int32_t snap_to_mask(int32_t note, uint16_t mask) const {
        if (mask & (1u << note)) return note;

        // Search closest allowed note (prefer lower on tie)
        for (int dist = 1; dist <= 6; ++dist) {
            int down = (note - dist + 12) % 12;
            if (mask & (1u << down)) return down;

            int up = (note + dist) % 12;
            if (mask & (1u << up)) return up;
        }
        return note;
    }
};

} // namespace aether::embedded
