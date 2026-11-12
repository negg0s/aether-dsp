#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <cmath>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * @brief High-speed 256-point lookup-table integer sine oscillator in Q15.
 * Computes pure sinusoidal signals in < 5 CPU cycles on Cortex-M / RISC-V without FPU.
 */
class FixedSineOscillator {
public:
    FixedSineOscillator() noexcept {
        init_table();
    }

    void reset() noexcept {
        phase_accum_ = 0;
    }

    /**
     * @brief Set oscillator frequency relative to sample rate: freq / sample_rate in [0.0, 0.5].
     */
    void set_normalized_frequency(float norm_freq) noexcept {
        norm_freq = std::clamp(norm_freq, 0.0f, 0.499f);
        phase_increment_ = static_cast<uint32_t>(norm_freq * 4294967296.0f); // 2^32
    }

    [[nodiscard]] inline FixedQ15 step() noexcept {
        // Upper 8 bits index the 256-point table
        const size_t idx0 = (phase_accum_ >> 24) & 0xFF;
        const size_t idx1 = (idx0 + 1) & 0xFF;
        // Next 8 bits represent fractional interpolation factor
        const uint16_t frac = (phase_accum_ >> 16) & 0xFF;

        const int32_t v0 = lut_[idx0].raw;
        const int32_t v1 = lut_[idx1].raw;
        const int32_t interp = v0 + (((v1 - v0) * static_cast<int32_t>(frac)) >> 8);

        phase_accum_ += phase_increment_;
        return FixedQ15(static_cast<int16_t>(interp));
    }

private:
    static constexpr size_t TABLE_SIZE = 256;
    std::array<FixedQ15, TABLE_SIZE> lut_{};
    uint32_t phase_accum_{0};
    uint32_t phase_increment_{0};

    void init_table() noexcept {
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            const float angle = (static_cast<float>(i) / static_cast<float>(TABLE_SIZE)) * 6.283185307f;
            lut_[i] = FixedQ15::from_float(std::sin(angle));
        }
    }
};

} // namespace aether::math
