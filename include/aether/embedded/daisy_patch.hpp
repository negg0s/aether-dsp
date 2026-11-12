#pragma once

#include "../math/fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::embedded {

/**
 * DaisyPatchEurorackInterface
 * Emulates the Electro-Smith Daisy Patch / Eurorack Subsystem:
 * - 4 CV Inputs (-5V to +5V normalized to 16-bit bipolar / Q15)
 * - 4 CV Outputs (-5V to +5V)
 * - 2 Gate Inputs (with edge detection)
 * - 2 Gate Outputs
 * - 1V/Octave Calibrated Pitch Conversion to 32-bit Phase Increment
 * Zero-heap, zero-allocation, purely deterministic fixed-point DSP.
 */
class DaisyPatchEurorackInterface {
public:
    static constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
    static constexpr int32_t BASE_NOTE_HZ = 440; // A4

    explicit constexpr DaisyPatchEurorackInterface(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) noexcept
        : sample_rate_(sample_rate > 0 ? sample_rate : DEFAULT_SAMPLE_RATE) {}

    // Process CV input from 16-bit ADC (-32768 to 32767 representing -5V to +5V)
    void set_cv_raw(size_t channel, int16_t adc_val) noexcept {
        if (channel < cv_in_.size()) {
            cv_in_[channel] = math::FixedQ15::from_raw(adc_val);
        }
    }

    [[nodiscard]] math::FixedQ15 get_cv(size_t channel) const noexcept {
        if (channel < cv_in_.size()) {
            return cv_in_[channel];
        }
        return math::FixedQ15::from_raw(0);
    }

    // Set Gate Input (high when voltage > 1.5V)
    void set_gate_in(size_t channel, bool high) noexcept {
        if (channel < gate_in_.size()) {
            prev_gate_in_[channel] = gate_in_[channel];
            gate_in_[channel] = high;
        }
    }

    [[nodiscard]] bool gate_in(size_t channel) const noexcept {
        if (channel < gate_in_.size()) {
            return gate_in_[channel];
        }
        return false;
    }

    // Returns true on rising edge of gate input
    [[nodiscard]] bool gate_in_rose(size_t channel) const noexcept {
        if (channel < gate_in_.size()) {
            return gate_in_[channel] && !prev_gate_in_[channel];
        }
        return false;
    }

    // Returns true on falling edge of gate input
    [[nodiscard]] bool gate_in_fell(size_t channel) const noexcept {
        if (channel < gate_in_.size()) {
            return !gate_in_[channel] && prev_gate_in_[channel];
        }
        return false;
    }

    // CV Out (-5V to +5V)
    void set_cv_out(size_t channel, math::FixedQ15 val) noexcept {
        if (channel < cv_out_.size()) {
            cv_out_[channel] = val;
        }
    }

    [[nodiscard]] math::FixedQ15 cv_out(size_t channel) const noexcept {
        if (channel < cv_out_.size()) {
            return cv_out_[channel];
        }
        return math::FixedQ15::from_raw(0);
    }

    // Gate Out
    void set_gate_out(size_t channel, bool high) noexcept {
        if (channel < gate_out_.size()) {
            gate_out_[channel] = high;
        }
    }

    [[nodiscard]] bool gate_out(size_t channel) const noexcept {
        if (channel < gate_out_.size()) {
            return gate_out_[channel];
        }
        return false;
    }

    /**
     * Convert 1V/Oct CV to 32-bit Phase Increment for FixedOscillator.
     * In Eurorack: 1.0 Volt = 1 Octave (doubling frequency).
     * 0V corresponds to Base Note (440 Hz).
     * Range: -5V (approx 13.75 Hz) to +5V (approx 14080 Hz).
     * CV is Q15 where 0V = 0, +5V = +32767 (+1.0 Q15), -5V = -32768 (-1.0 Q15).
     */
    [[nodiscard]] uint32_t cv_to_phase_increment(math::FixedQ15 cv) const noexcept {
        uint64_t base_inc = (static_cast<uint64_t>(BASE_NOTE_HZ) << 32) / sample_rate_;

        int32_t semitones = (static_cast<int32_t>(cv.raw) * 60) >> 15;
        semitones = std::clamp(semitones, -60, 60);

        int32_t full_octaves = semitones / 12;
        int32_t rem_semitones = semitones % 12;
        if (rem_semitones < 0) {
            full_octaves -= 1;
            rem_semitones += 12;
        }

        static constexpr std::array<uint32_t, 12> SEMITONE_LUT_Q15 = {
            32768, 34716, 36780, 38968, 41285, 43740,
            46341, 49097, 52016, 55109, 58386, 61858
        };

        uint64_t scaled_inc = (base_inc * SEMITONE_LUT_Q15[static_cast<size_t>(rem_semitones)]) >> 15;

        if (full_octaves >= 0) {
            scaled_inc <<= std::min(full_octaves, 5);
        } else {
            scaled_inc >>= std::min(-full_octaves, 6);
        }

        return static_cast<uint32_t>(std::clamp<uint64_t>(scaled_inc, 1ULL, 0xFFFFFFFFULL));
    }

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    std::array<math::FixedQ15, 4> cv_in_{};
    std::array<math::FixedQ15, 4> cv_out_{};
    std::array<bool, 2> gate_in_{};
    std::array<bool, 2> prev_gate_in_{};
    std::array<bool, 2> gate_out_{};
};

} // namespace aether::embedded
