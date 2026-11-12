#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedLfo
 * Low-Frequency Oscillator in Q15 for embedded targets (Daisy Seed, Cortex-M4/M7).
 * Shapes:
 * 0 - Sine (via quarter-wave LUT)
 * 1 - Triangle
 * 2 - Sawtooth (ramp down)
 * 3 - Square
 * 4 - Sample & Hold (pseudo-random Galois LFSR)
 * Deterministic, zero-allocation, 32-bit phase accumulator.
 */
class FixedLfo {
public:
    enum class Waveform : uint8_t {
        Sine = 0,
        Triangle = 1,
        Sawtooth = 2,
        Square = 3,
        SampleAndHold = 4
    };

    constexpr FixedLfo() noexcept {
        init_sine_lut();
    }

    void set_phase_increment(uint32_t inc) noexcept {
        phase_inc_ = inc;
    }

    void set_frequency(float freq_hz, uint32_t sample_rate) noexcept {
        if (sample_rate == 0) return;
        freq_hz = std::clamp(freq_hz, 0.01f, 100.0f);
        phase_inc_ = static_cast<uint32_t>((static_cast<double>(freq_hz) * 4294967296.0) / sample_rate);
    }

    void set_waveform(Waveform wf) noexcept {
        waveform_ = wf;
    }

    void reset_phase() noexcept {
        phase_ = 0;
    }

    [[nodiscard]] FixedQ15 process() noexcept {
        int16_t out_raw = 0;
        uint32_t old_phase = phase_;
        phase_ += phase_inc_;

        switch (waveform_) {
            case Waveform::Sine: {
                uint32_t idx = (phase_ >> 24) & 0xFF; // 8-bit index [0, 255]
                out_raw = sine_lut_[idx];
                break;
            }
            case Waveform::Triangle: {
                // Phase top 16 bits: 0..65535
                uint16_t p = static_cast<uint16_t>(phase_ >> 16);
                if (p < 32768) {
                    // Ramp up: -32768 to 32767
                    out_raw = static_cast<int16_t>((static_cast<int32_t>(p) * 2) - 32768);
                } else {
                    // Ramp down: 32767 to -32768
                    out_raw = static_cast<int16_t>(32767 - (static_cast<int32_t>(p - 32768) * 2));
                }
                break;
            }
            case Waveform::Sawtooth: {
                // Top 16 bits: 0..65535 -> 32767 to -32768
                uint16_t p = static_cast<uint16_t>(phase_ >> 16);
                out_raw = static_cast<int16_t>(32767 - static_cast<int32_t>(p));
                break;
            }
            case Waveform::Square: {
                out_raw = (phase_ & 0x80000000) ? 32767 : -32768;
                break;
            }
            case Waveform::SampleAndHold: {
                // Trigger new pseudo-random value when phase wraps around
                if (phase_ < old_phase) {
                    step_lfsr();
                }
                out_raw = static_cast<int16_t>(lfsr_ & 0xFFFF);
                break;
            }
        }

        return FixedQ15::from_raw(out_raw);
    }

private:
    constexpr void init_sine_lut() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            int32_t x = static_cast<int32_t>(i);
            int32_t q = 0;
            if (x < 64) {
                q = (x * 32767) / 64;
            } else if (x < 128) {
                q = ((128 - x) * 32767) / 64;
            } else if (x < 192) {
                q = -(((x - 128) * 32767) / 64);
            } else {
                q = -(((256 - x) * 32767) / 64);
            }
            sine_lut_[i] = static_cast<int16_t>(q);
        }
    }

    void step_lfsr() noexcept {
        // 32-bit Galois LFSR with polynomial x^32 + x^22 + x^2 + x^1 + 1
        uint32_t bit = ((lfsr_ >> 0) ^ (lfsr_ >> 1) ^ (lfsr_ >> 2) ^ (lfsr_ >> 22)) & 1u;
        lfsr_ = (lfsr_ >> 1) | (bit << 31);
    }

    uint32_t phase_{0};
    uint32_t phase_inc_{0};
    uint32_t lfsr_{0xACE1ACE1u};
    Waveform waveform_{Waveform::Sine};
    std::array<int16_t, 256> sine_lut_{};
};

} // namespace aether::math
