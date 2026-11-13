#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Eurorack Precision 4-Channel Matrix Mixer
// 4 Inputs x 4 Outputs with bipolar gain control (-1.0 to +1.0 in Q15)
// and soft-saturating analog diode clipping on summing buses when exceeding unity headroom.
// Zero-heap, zero-allocation bare-metal MCU matrix mixer.
class EurorackMatrixMixer {
public:
    static constexpr size_t Channels = 4;

    EurorackMatrixMixer() {
        reset();
    }

    void reset() noexcept {
        for (auto& row : gains_) {
            for (auto& g : row) {
                g = math::FixedQ15(0);
            }
        }
    }

    // Set matrix gain from input_ch to output_ch (-1.0 to +1.0)
    void set_gain(size_t in_ch, size_t out_ch, math::FixedQ15 gain) noexcept {
        if (in_ch < Channels && out_ch < Channels) {
            gains_[out_ch][in_ch] = gain;
        }
    }

    // Process a frame of 4 input signals and produce 4 mixed output signals
    // with 64-bit headroom summation and soft clipping when exceeding unity.
    std::array<math::FixedQ15, Channels> process(const std::array<math::FixedQ15, Channels>& inputs) noexcept {
        std::array<math::FixedQ15, Channels> outputs{};

        for (size_t out_idx = 0; out_idx < Channels; ++out_idx) {
            int64_t sum = 0;
            for (size_t in_idx = 0; in_idx < Channels; ++in_idx) {
                int64_t prod = static_cast<int64_t>(inputs[in_idx].raw) * gains_[out_idx][in_idx].raw;
                sum += (prod >> 15);
            }

            // Analog soft saturation when exceeding linear threshold (>= 0.75 in Q15 -> 24576)
            int64_t saturated = sum;
            if (sum > 24576) {
                int64_t excess = sum - 24576;
                // Soft compress excess: excess / (1 + excess/16384)
                int64_t compressed_excess = (excess << 14) / (16384 + excess);
                saturated = 24576 + compressed_excess;
            } else if (sum < -24576) {
                int64_t excess = -sum - 24576;
                int64_t compressed_excess = (excess << 14) / (16384 + excess);
                saturated = -(24576 + compressed_excess);
            }

            if (saturated > 32767) saturated = 32767;
            if (saturated < -32768) saturated = -32768;

            outputs[out_idx] = math::FixedQ15(static_cast<int16_t>(saturated));
        }

        return outputs;
    }

private:
    std::array<std::array<math::FixedQ15, Channels>, Channels> gains_{};
};

} // namespace aether::embedded
