#pragma once

#include "../../types.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

namespace aether::dsp::fx {

/**
 * @brief High-Precision Time-Domain Convolution Processor.
 *
 * Convolves incoming audio with Non-Euclidean Manifold Impulse Responses (MIR)
 * using an internal circular history delay line with wet/dry crossfading.
 */
class ConvolutionReverb {
public:
    ConvolutionReverb() = default;

    /**
     * @brief Load impulse response stereo frames.
     */
    void load_impulse_response(const std::vector<StereoFrame>& ir, float wet_mix = 0.4f) {
        if (ir.empty()) {
            ir_left_.clear();
            ir_right_.clear();
            history_left_.clear();
            history_right_.clear();
            cursor_ = 0;
            return;
        }

        const size_t sz = std::min(ir.size(), size_t{65536});
        ir_left_.resize(sz);
        ir_right_.resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            ir_left_[i] = ir[i].left;
            ir_right_[i] = ir[i].right;
        }

        history_left_.assign(sz, 0.0f);
        history_right_.assign(sz, 0.0f);
        cursor_ = 0;
        wet_mix_ = std::clamp(wet_mix, 0.0f, 1.0f);
    }

    /**
     * @brief Set wet/dry ratio [0.0 = dry, 1.0 = 100% wet].
     */
    void set_mix(float mix) noexcept {
        wet_mix_ = std::clamp(mix, 0.0f, 1.0f);
    }

    /**
     * @brief Process a single stereo frame through convolution.
     */
    [[nodiscard]] StereoFrame process(const StereoFrame& in) noexcept {
        if (ir_left_.empty()) return in;

        const size_t len = ir_left_.size();
        history_left_[cursor_] = in.left;
        history_right_[cursor_] = in.right;

        float conv_l = 0.0f;
        float conv_r = 0.0f;

        size_t hist_idx = cursor_;
        for (size_t k = 0; k < len; ++k) {
            conv_l += history_left_[hist_idx] * ir_left_[k];
            conv_r += history_right_[hist_idx] * ir_right_[k];

            if (hist_idx == 0) {
                hist_idx = len - 1;
            } else {
                --hist_idx;
            }
        }

        cursor_ = (cursor_ + 1) % len;

        const float dry = 1.0f - wet_mix_;
        return StereoFrame{
            in.left * dry + conv_l * wet_mix_,
            in.right * dry + conv_r * wet_mix_
        };
    }

    /**
     * @brief Reset internal delay line history.
     */
    void reset() noexcept {
        std::fill(history_left_.begin(), history_left_.end(), 0.0f);
        std::fill(history_right_.begin(), history_right_.end(), 0.0f);
        cursor_ = 0;
    }

private:
    std::vector<float> ir_left_;
    std::vector<float> ir_right_;
    std::vector<float> history_left_;
    std::vector<float> history_right_;
    size_t cursor_{0};
    float wet_mix_{0.35f};
};

} // namespace aether::dsp::fx
