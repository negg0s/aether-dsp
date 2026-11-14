#pragma once

#include "../types.hpp"
#include <cstddef>
#include <cstdint>
#include <array>
#include <algorithm>
#include <cmath>

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#define AETHER_HAS_NEON 1
#else
#define AETHER_HAS_NEON 0
#endif

namespace aether::simd {

/**
 * @brief Portable 4-wide 32-bit float SIMD vector for cross-platform DSP.
 * Automatically uses ARM NEON (arm64, Raspberry Pi, Daisy/Cortex-M7) or SSE/scalar fallback.
 */
struct Float4 {
#if AETHER_HAS_NEON
    float32x4_t v;
    inline Float4() noexcept : v(vdupq_n_f32(0.0f)) {}
    inline explicit Float4(float val) noexcept : v(vdupq_n_f32(val)) {}
    inline Float4(float32x4_t raw) noexcept : v(raw) {}
    inline Float4(float a, float b, float c, float d) noexcept {
        const float data[4] = {a, b, c, d};
        v = vld1q_f32(data);
    }

    [[nodiscard]] static inline Float4 load(const float* ptr) noexcept {
        return Float4(vld1q_f32(ptr));
    }
    inline void store(float* ptr) const noexcept {
        vst1q_f32(ptr, v);
    }
    inline Float4 operator+(Float4 other) const noexcept { return Float4(vaddq_f32(v, other.v)); }
    inline Float4 operator-(Float4 other) const noexcept { return Float4(vsubq_f32(v, other.v)); }
    inline Float4 operator*(Float4 other) const noexcept { return Float4(vmulq_f32(v, other.v)); }
    inline Float4 operator/(Float4 other) const noexcept {
#if defined(__aarch64__)
        return Float4(vdivq_f32(v, other.v));
#else
        // ARMv7 reciprocal estimate
        float32x4_t rec = vrecpeq_f32(other.v);
        rec = vmulq_f32(vrecpsq_f32(other.v, rec), rec);
        return Float4(vmulq_f32(v, rec));
#endif
    }
    [[nodiscard]] inline Float4 fma(Float4 b, Float4 c) const noexcept {
        // compute this * b + c
#if defined(__aarch64__)
        return Float4(vfmaq_f32(c.v, v, b.v));
#else
        return Float4(vmlaq_f32(c.v, v, b.v));
#endif
    }
#else
    alignas(16) std::array<float, 4> data;
    inline Float4() noexcept : data{0.0f, 0.0f, 0.0f, 0.0f} {}
    inline explicit Float4(float val) noexcept : data{val, val, val, val} {}
    inline Float4(float a, float b, float c, float d) noexcept : data{a, b, c, d} {}

    [[nodiscard]] static inline Float4 load(const float* ptr) noexcept {
        return Float4(ptr[0], ptr[1], ptr[2], ptr[3]);
    }
    inline void store(float* ptr) const noexcept {
        ptr[0] = data[0];
        ptr[1] = data[1];
        ptr[2] = data[2];
        ptr[3] = data[3];
    }
    inline Float4 operator+(Float4 other) const noexcept {
        return Float4(data[0] + other.data[0], data[1] + other.data[1], data[2] + other.data[2], data[3] + other.data[3]);
    }
    inline Float4 operator-(Float4 other) const noexcept {
        return Float4(data[0] - other.data[0], data[1] - other.data[1], data[2] - other.data[2], data[3] - other.data[3]);
    }
    inline Float4 operator*(Float4 other) const noexcept {
        return Float4(data[0] * other.data[0], data[1] * other.data[1], data[2] * other.data[2], data[3] * other.data[3]);
    }
    inline Float4 operator/(Float4 other) const noexcept {
        return Float4(data[0] / other.data[0], data[1] / other.data[1], data[2] / other.data[2], data[3] / other.data[3]);
    }
    [[nodiscard]] inline Float4 fma(Float4 b, Float4 c) const noexcept {
        return Float4(std::fma(data[0], b.data[0], c.data[0]),
                      std::fma(data[1], b.data[1], c.data[1]),
                      std::fma(data[2], b.data[2], c.data[2]),
                      std::fma(data[3], b.data[3], c.data[3]));
    }
#endif
};

} // namespace aether::simd
