#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

namespace aether::math {

/**
 * @brief Fixed-point Q15 arithmetic (1 sign bit, 15 fractional bits)
 * Ideal for ultra-low power microcontrollers (Daisy Seed Cortex-M7, ARM Cortex-M4)
 * with values normalized in range [-1.0, +0.999969].
 */
class FixedQ15 {
public:
    int16_t raw{0};

    constexpr FixedQ15() noexcept = default;
    constexpr explicit FixedQ15(int16_t v) noexcept : raw(v) {}

    static constexpr FixedQ15 from_raw(int16_t v) noexcept {
        return FixedQ15(v);
    }

    static constexpr FixedQ15 from_float(float f) noexcept {
        f = std::clamp(f, -1.0f, 0.999969f);
        return FixedQ15(static_cast<int16_t>(std::round(f * 32768.0f)));
    }

    [[nodiscard]] constexpr float to_float() const noexcept {
        return static_cast<float>(raw) / 32768.0f;
    }

    constexpr FixedQ15 operator-() const noexcept {
        return FixedQ15(static_cast<int16_t>(raw == -32768 ? 32767 : -raw));
    }

    constexpr FixedQ15 operator+(FixedQ15 other) const noexcept {
        const int32_t sum = static_cast<int32_t>(raw) + static_cast<int32_t>(other.raw);
        return FixedQ15(static_cast<int16_t>(std::clamp<int32_t>(sum, -32768, 32767)));
    }

    constexpr FixedQ15 operator-(FixedQ15 other) const noexcept {
        const int32_t diff = static_cast<int32_t>(raw) - static_cast<int32_t>(other.raw);
        return FixedQ15(static_cast<int16_t>(std::clamp<int32_t>(diff, -32768, 32767)));
    }

    constexpr FixedQ15 operator*(FixedQ15 other) const noexcept {
        const int32_t prod = (static_cast<int32_t>(raw) * static_cast<int32_t>(other.raw)) >> 15;
        return FixedQ15(static_cast<int16_t>(std::clamp<int32_t>(prod, -32768, 32767)));
    }
};

/**
 * @brief Fixed-point Q27 arithmetic (1 sign bit, 4 integer bits, 27 fractional bits)
 * Provides 27 bits of precision for 32-bit audio buffers with 24 dB of headroom.
 */
class FixedQ27 {
public:
    int32_t raw{0};

    constexpr FixedQ27() noexcept = default;
    constexpr explicit FixedQ27(int32_t v) noexcept : raw(v) {}

    static constexpr FixedQ27 from_float(float f) noexcept {
        f = std::clamp(f, -16.0f, 15.999999f);
        return FixedQ27(static_cast<int32_t>(std::round(f * 134217728.0f)));
    }

    [[nodiscard]] constexpr float to_float() const noexcept {
        return static_cast<float>(raw) / 134217728.0f;
    }

    constexpr FixedQ27 operator+(FixedQ27 other) const noexcept {
        const int64_t sum = static_cast<int64_t>(raw) + static_cast<int64_t>(other.raw);
        return FixedQ27(static_cast<int32_t>(std::clamp<int64_t>(sum, -2147483648LL, 2147483647LL)));
    }

    constexpr FixedQ27 operator-(FixedQ27 other) const noexcept {
        const int64_t diff = static_cast<int64_t>(raw) - static_cast<int64_t>(other.raw);
        return FixedQ27(static_cast<int32_t>(std::clamp<int64_t>(diff, -2147483648LL, 2147483647LL)));
    }

    constexpr FixedQ27 operator*(FixedQ27 other) const noexcept {
        const int64_t prod = (static_cast<int64_t>(raw) * static_cast<int64_t>(other.raw)) >> 27;
        return FixedQ27(static_cast<int32_t>(std::clamp<int64_t>(prod, -2147483648LL, 2147483647LL)));
    }
};

} // namespace aether::math
