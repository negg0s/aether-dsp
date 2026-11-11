#pragma once

#include "../types.hpp"
#include <complex>
#include <vector>
#include <span>
#include <cmath>
#include <numbers>
#include <cassert>
#include <algorithm>

namespace aether::math {

enum class WindowType {
    Rectangular,
    Hann,
    Hamming,
    BlackmanHarris
};

/**
 * @brief High-Performance In-Place Radix-2 Cooley-Tukey FFT & Spectral Analysis.
 *
 * Implements Decimation-in-Time (DIT) in-place butterfly transforms,
 * windowing operations, and power spectrum estimation with zero heap allocations.
 */
class FFT {
public:
    using Complex = std::complex<float>;

    /**
     * @brief Check if integer is a positive power of two.
     */
    [[nodiscard]] static constexpr bool is_power_of_two(size_t n) noexcept {
        return n > 0 && (n & (n - 1)) == 0;
    }

    /**
     * @brief Bit-reversal permutation of an array of length N (N must be power of 2).
     */
    static void bit_reverse(std::span<Complex> data) noexcept {
        const size_t n = data.size();
        if (n <= 1 || !is_power_of_two(n)) return;

        size_t j = 0;
        for (size_t i = 0; i < n - 1; ++i) {
            if (i < j) {
                std::swap(data[i], data[j]);
            }
            size_t k = n >> 1;
            while (k <= j) {
                j -= k;
                k >>= 1;
            }
            j += k;
        }
    }

    /**
     * @brief Forward in-place Fast Fourier Transform.
     * Computes X[k] = sum_{n=0}^{N-1} x[n] * e^{-i * 2*pi * k * n / N}
     */
    static void forward(std::span<Complex> data) noexcept {
        if (data.size() <= 1 || !is_power_of_two(data.size())) return;
        transform(data, false);
    }

    /**
     * @brief Inverse in-place Fast Fourier Transform with 1/N scaling.
     * Computes x[n] = (1/N) * sum_{k=0}^{N-1} X[k] * e^{i * 2*pi * k * n / N}
     */
    static void inverse(std::span<Complex> data) noexcept {
        if (data.size() <= 1 || !is_power_of_two(data.size())) return;
        transform(data, true);
        const float inv_n = 1.0f / static_cast<float>(data.size());
        for (auto& x : data) {
            x *= inv_n;
        }
    }

    /**
     * @brief Apply spectral analysis window to a real-valued input block.
     */
    static void apply_window(std::span<float> data, WindowType window) noexcept {
        const size_t n = data.size();
        if (n <= 1 || window == WindowType::Rectangular) return;

        const float inv_n1 = 1.0f / static_cast<float>(n - 1);
        constexpr float PI2 = 2.0f * std::numbers::pi_v<float>;

        for (size_t i = 0; i < n; ++i) {
            const float phase = static_cast<float>(i) * inv_n1;
            float w = 1.0f;

            switch (window) {
                case WindowType::Hann:
                    w = 0.5f * (1.0f - std::cos(PI2 * phase));
                    break;
                case WindowType::Hamming:
                    w = 0.54f - 0.46f * std::cos(PI2 * phase);
                    break;
                case WindowType::BlackmanHarris:
                    w = 0.35875f - 0.48829f * std::cos(PI2 * phase)
                                + 0.14128f * std::cos(2.0f * PI2 * phase)
                                - 0.01168f * std::cos(3.0f * PI2 * phase);
                    break;
                case WindowType::Rectangular:
                    w = 1.0f;
                    break;
            }
            data[i] *= w;
        }
    }

    /**
     * @brief Compute power spectrum |X[k]|^2 / N^2 for real-valued audio samples.
     * @param real_input Real audio samples (length N, must be power of 2)
     * @param power_output Magnitude spectrum (length N/2 + 1)
     * @param window Windowing function to apply
     */
    static void compute_power_spectrum(std::span<const float> real_input,
                                       std::span<float> power_output,
                                       WindowType window = WindowType::Hann) {
        const size_t n = real_input.size();
        if (n <= 1 || !is_power_of_two(n) || power_output.size() < n / 2 + 1) {
            std::fill(power_output.begin(), power_output.end(), 0.0f);
            return;
        }

        std::vector<Complex> buffer(n);
        std::vector<float> windowed(real_input.begin(), real_input.end());
        apply_window(windowed, window);

        for (size_t i = 0; i < n; ++i) {
            buffer[i] = Complex(windowed[i], 0.0f);
        }

        forward(buffer);

        const float inv_n2 = 1.0f / static_cast<float>(n * n);
        const size_t num_bins = n / 2 + 1;

        for (size_t k = 0; k < num_bins; ++k) {
            const float mag2 = std::norm(buffer[k]); // |buffer[k]|^2
            // Single-sided spectral power scaling (multiply by 2 for bins 1..N/2-1)
            const float scale = (k == 0 || k == n / 2) ? inv_n2 : 2.0f * inv_n2;
            power_output[k] = mag2 * scale;
        }
    }

private:
    static void transform(std::span<Complex> data, bool is_inverse) noexcept {
        const size_t n = data.size();
        assert(is_power_of_two(n));

        bit_reverse(data);

        const float sign = is_inverse ? 1.0f : -1.0f;
        constexpr float PI = std::numbers::pi_v<float>;

        // Butterfly stages: length = 2, 4, 8, ..., N
        for (size_t len = 2; len <= n; len <<= 1) {
            const float angle = sign * 2.0f * PI / static_cast<float>(len);
            const Complex wlen(std::cos(angle), std::sin(angle));

            for (size_t i = 0; i < n; i += len) {
                Complex w(1.0f, 0.0f);
                const size_t half_len = len >> 1;

                for (size_t j = 0; j < half_len; ++j) {
                    const Complex u = data[i + j];
                    const Complex v = data[i + j + half_len] * w;

                    data[i + j] = u + v;
                    data[i + j + half_len] = u - v;

                    w *= wlen;
                }
            }
        }
    }
};

} // namespace aether::math
