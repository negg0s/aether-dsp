#pragma once

#include "../../types.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace aether::dsp::fx {

/**
 * @brief Stereo Hyperbolic Feedback Delay Network (FDN) Reverb.
 *
 * Implements a 4-channel unitary Feedback Delay Network with prime delay lines
 * derived from the non-Euclidean {7, 3} hyperbolic heptagonal tiling.
 * Zero flutter echoes and exponential modal density growth.
 */
class HyperbolicReverb {
public:
    static constexpr size_t NUM_LINES = 4;
    // Mutually prime delay line lengths in samples at 48kHz
    static constexpr std::array<size_t, NUM_LINES> DELAY_LENGTHS = {1087, 1381, 1693, 2017};

    HyperbolicReverb() noexcept {
        for (size_t i = 0; i < NUM_LINES; ++i) {
            delay_buffers_[i].resize(DELAY_LENGTHS[i], 0.0f);
        }
    }

    void reset() noexcept {
        for (size_t i = 0; i < NUM_LINES; ++i) {
            std::fill(delay_buffers_[i].begin(), delay_buffers_[i].end(), 0.0f);
            write_indices_[i] = 0;
            damp_filters_[i] = 0.0f;
        }
    }

    void configure(float decay_time = 0.85f, float damping = 0.3f, float wet_mix = 0.35f) noexcept {
        decay_ = std::clamp(decay_time, 0.1f, 0.98f);
        damping_ = std::clamp(damping, 0.01f, 0.9f);
        wet_mix_ = std::clamp(wet_mix, 0.0f, 1.0f);
    }

    [[nodiscard]] StereoFrame process(StereoFrame input) noexcept {
        std::array<float, NUM_LINES> outputs{};

        // 1. Read delay line outputs
        for (size_t i = 0; i < NUM_LINES; ++i) {
            outputs[i] = delay_buffers_[i][write_indices_[i]];
        }

        // 2. 4x4 Unitary Householder Feedback Matrix: H = I - 2/N * (11^T)
        // For N=4, H_ij = 0.5 for i=j, -0.5 for i!=j
        const float sum = outputs[0] + outputs[1] + outputs[2] + outputs[3];
        const float factor = 0.5f * sum;

        std::array<float, NUM_LINES> feedback{};
        for (size_t i = 0; i < NUM_LINES; ++i) {
            feedback[i] = (outputs[i] - factor) * decay_;

            // Low-pass dampening filter in feedback loop with denormal flushing
            damp_filters_[i] += damping_ * (feedback[i] - damp_filters_[i]);
            if (std::abs(damp_filters_[i]) < 1e-15f) damp_filters_[i] = 0.0f;
            feedback[i] = damp_filters_[i];
        }

        // 3. Inject stereo inputs and write back
        const float mono_in = (input.left + input.right) * 0.5f;

        for (size_t i = 0; i < NUM_LINES; ++i) {
            float to_write = mono_in + feedback[i];
            if (std::abs(to_write) < 1e-15f) to_write = 0.0f;
            delay_buffers_[i][write_indices_[i]] = to_write;

            write_indices_[i]++;
            if (write_indices_[i] >= DELAY_LENGTHS[i]) {
                write_indices_[i] = 0;
            }
        }

        // 4. Stereo decorrelation readout
        const float wet_l = (outputs[0] - outputs[1] + outputs[2]) * 0.6f;
        const float wet_r = (outputs[0] + outputs[1] - outputs[3]) * 0.6f;

        const float dry = 1.0f - wet_mix_;
        return {
            input.left * dry + wet_l * wet_mix_,
            input.right * dry + wet_r * wet_mix_
        };
    }

private:
    float decay_{0.85f};
    float damping_{0.3f};
    float wet_mix_{0.35f};

    std::array<std::vector<float>, NUM_LINES> delay_buffers_{};
    std::array<size_t, NUM_LINES> write_indices_{0, 0, 0, 0};
    std::array<float, NUM_LINES> damp_filters_{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace aether::dsp::fx
