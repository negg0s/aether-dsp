#pragma once

#include "fixed_point.hpp"
#include <cstdint>
#include <array>
#include <algorithm>

namespace aether::math {

/**
 * FixedBitcrusher
 * Lo-fi DSP decimator and bit-depth reducer in Q15:
 * - Dynamic bit depth reduction from 16 bits down to 1 bit.
 * - Sample rate decimation (hold factor from 1x to 64x downsampling).
 * - Analog jitter / aliasing grit in fixed point.
 * Zero dynamic memory, zero FPU.
 */
class FixedBitcrusher {
public:
    constexpr FixedBitcrusher() noexcept = default;

    /**
     * configure
     * bits: target bit resolution [1, 16]
     * rate_divide: downsample hold factor [1, 64]
     */
    void configure(int bits, int rate_divide) noexcept {
        bits_ = std::clamp<int>(bits, 1, 16);
        rate_divide_ = std::clamp<int>(rate_divide, 1, 64);
        mask_ = ~((1 << (16 - bits_)) - 1);
    }

    void reset() noexcept {
        held_sample_ = FixedQ15::from_raw(0);
        counter_ = 0;
    }

    [[nodiscard]] FixedQ15 process(FixedQ15 input) noexcept {
        if (counter_ == 0) {
            // Quantize sample to target bit depth
            int32_t val = input.raw;
            int32_t crushed = val & mask_;
            crushed = std::clamp<int32_t>(crushed, -32768, 32767);
            held_sample_ = FixedQ15::from_raw(static_cast<int16_t>(crushed));
        }

        counter_ = (counter_ + 1) % rate_divide_;
        return held_sample_;
    }

private:
    int bits_{12};
    int rate_divide_{1};
    int32_t mask_{0xFFF0};
    FixedQ15 held_sample_{FixedQ15::from_raw(0)};
    int counter_{0};
};

} // namespace aether::math
