#pragma once

#include "../../types.hpp"
#include "../biquad.hpp"
#include "../../math/fft.hpp"
#include <vector>
#include <complex>
#include <span>
#include <algorithm>
#include <cassert>

namespace aether::dsp::fx {

/**
 * @brief Zero-Latency Uniform Partitioned Overlap-Save (UPOLS) FFT Convolution Engine.
 *
 * Partitions long impulse responses (e.g. cathedral hyperbolic manifold reverberations)
 * into equal-sized blocks of size B, computing block convolutions in the frequency domain
 * using Radix-2 Cooley-Tukey FFTs. Reduces complexity from O(L * N) to O(P * B log B)
 * with low latency.
 */
class PartitionedConvolution {
public:
    using Complex = std::complex<float>;

    static constexpr size_t BLOCK_SIZE = 128;
    static constexpr size_t FFT_SIZE = BLOCK_SIZE * 2; // 256

    PartitionedConvolution() = default;

    /**
     * @brief Load and partition impulse response of arbitrary length.
     * @param ir Mono impulse response samples
     * @param wet_gain Output wet gain scaling
     */
    void load_impulse_response(std::span<const float> ir, float wet_gain = 0.5f) {
        wet_gain_ = wet_gain;
        if (ir.empty()) {
            num_partitions_ = 0;
            return;
        }

        num_partitions_ = (ir.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
        freq_partitions_.assign(num_partitions_ * FFT_SIZE, Complex(0.0f, 0.0f));
        input_history_.assign(num_partitions_ * FFT_SIZE, Complex(0.0f, 0.0f));
        time_input_buffer_.assign(FFT_SIZE, 0.0f);
        output_fifo_.assign(BLOCK_SIZE, 0.0f);
        fifo_read_idx_ = BLOCK_SIZE; // Triggers immediate block process on first sample

        std::vector<Complex> fft_buf(FFT_SIZE);

        for (size_t p = 0; p < num_partitions_; ++p) {
            std::fill(fft_buf.begin(), fft_buf.end(), Complex(0.0f, 0.0f));
            const size_t start_idx = p * BLOCK_SIZE;
            const size_t count = std::min(BLOCK_SIZE, ir.size() - start_idx);

            for (size_t i = 0; i < count; ++i) {
                fft_buf[i] = Complex(ir[start_idx + i], 0.0f);
            }

            // Zero-padded forward FFT of partition
            math::FFT::forward(fft_buf);

            // Store in frequency partition table
            for (size_t k = 0; k < FFT_SIZE; ++k) {
                freq_partitions_[p * FFT_SIZE + k] = fft_buf[k];
            }
        }
    }

    /**
     * @brief Load and partition stereo impulse response (mixes down to mono kernel).
     */
    void load_impulse_response(std::span<const StereoFrame> ir_stereo, float wet_gain = 0.5f) {
        std::vector<float> mono(ir_stereo.size());
        for (size_t i = 0; i < ir_stereo.size(); ++i) {
            mono[i] = (ir_stereo[i].left + ir_stereo[i].right) * 0.5f;
        }
        load_impulse_response(mono, wet_gain);
    }

    /**
     * @brief Process a single input sample through the partitioned convolution reverb.
     */
    [[nodiscard]] StereoFrame process(StereoFrame input) noexcept {
        if (num_partitions_ == 0) return input;

        const float mono_in = (input.left + input.right) * 0.5f;

        // Check if FIFO needs a new processed block
        if (fifo_read_idx_ >= BLOCK_SIZE) {
            process_new_block();
            fifo_read_idx_ = 0;
        }

        current_input_block_[fifo_read_idx_] = mono_in;
        const float wet_out = output_fifo_[fifo_read_idx_++] * wet_gain_;
        const float dry_gain = 1.0f - wet_gain_ * 0.5f;

        // Stereo spreading
        return StereoFrame{
            input.left * dry_gain + wet_out * 0.95f,
            input.right * dry_gain + wet_out * 1.05f
        };
    }

    /**
     * @brief Process an entire block of stereo frames directly.
     */
    void process_block(std::span<StereoFrame> frames) noexcept {
        for (auto& f : frames) {
            f = process(f);
        }
    }

    [[nodiscard]] size_t num_partitions() const noexcept { return num_partitions_; }
    [[nodiscard]] float wet_gain() const noexcept { return wet_gain_; }

private:
    void process_new_block() noexcept {
        // Shift time input buffer (overlap-save): copy second half to first half
        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            time_input_buffer_[i] = time_input_buffer_[i + BLOCK_SIZE];
            time_input_buffer_[i + BLOCK_SIZE] = current_input_block_[i];
        }

        // Forward FFT of current [x_{n-1}, x_n] input window
        std::vector<Complex> x_fft(FFT_SIZE);
        for (size_t i = 0; i < FFT_SIZE; ++i) {
            x_fft[i] = Complex(time_input_buffer_[i], 0.0f);
        }
        math::FFT::forward(x_fft);

        // Shift input history pointer (circular buffer of frequency-domain blocks)
        history_head_ = (history_head_ + num_partitions_ - 1) % num_partitions_;
        for (size_t k = 0; k < FFT_SIZE; ++k) {
            input_history_[history_head_ * FFT_SIZE + k] = x_fft[k];
        }

        // Frequency-domain complex multiply-accumulate: Y = sum_{p=0}^{P-1} X_p * H_p
        std::vector<Complex> y_accum(FFT_SIZE, Complex(0.0f, 0.0f));
        for (size_t p = 0; p < num_partitions_; ++p) {
            const size_t hist_idx = (history_head_ + p) % num_partitions_;
            const size_t h_base = p * FFT_SIZE;
            const size_t x_base = hist_idx * FFT_SIZE;

            for (size_t k = 0; k < FFT_SIZE; ++k) {
                const Complex x_val = input_history_[x_base + k];
                const Complex h_val = freq_partitions_[h_base + k];
                y_accum[k] += x_val * h_val;
            }
        }

        // Inverse FFT of accumulated frequency response
        math::FFT::inverse(y_accum);

        // Overlap-save: discard the first BLOCK_SIZE samples, save second BLOCK_SIZE samples
        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            output_fifo_[i] = y_accum[i + BLOCK_SIZE].real();
        }

        std::fill(current_input_block_.begin(), current_input_block_.end(), 0.0f);
    }

    size_t num_partitions_{0};
    float wet_gain_{0.5f};
    size_t history_head_{0};
    size_t fifo_read_idx_{BLOCK_SIZE};

    std::vector<Complex> freq_partitions_;
    std::vector<Complex> input_history_;
    std::vector<float> time_input_buffer_{std::vector<float>(FFT_SIZE, 0.0f)};
    std::array<float, BLOCK_SIZE> current_input_block_{};
    std::vector<float> output_fifo_{std::vector<float>(BLOCK_SIZE, 0.0f)};
};

} // namespace aether::dsp::fx
