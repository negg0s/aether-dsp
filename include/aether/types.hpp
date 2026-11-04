#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <numbers>
#include <span>
#include <concepts>
#include <array>

namespace aether {

// Audio sample representation (32-bit floating point for real-time DSP)
using Sample = float;

constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
constexpr size_t DEFAULT_BLOCK_SIZE = 64;

// Physical and mathematical constants
namespace constants {
    constexpr double PI = std::numbers::pi_v<double>;
    constexpr double TWO_PI = 2.0 * std::numbers::pi_v<double>;
    constexpr double SQRT2 = std::numbers::sqrt2_v<double>;
    constexpr double INV_PI = std::numbers::inv_pi_v<double>;
    constexpr float FLOAT_EPSILON = 1e-7f;
}

// 2D Vector for coordinate-space and geometric manifolds
struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    constexpr Vec2() noexcept = default;
    constexpr Vec2(float px, float py) noexcept : x(px), y(py) {}

    [[nodiscard]] constexpr float magnitude_squared() const noexcept {
        return x * x + y * y;
    }

    [[nodiscard]] inline float magnitude() const noexcept {
        return std::sqrt(magnitude_squared());
    }

    [[nodiscard]] constexpr Vec2 operator+(const Vec2& other) const noexcept {
        return {x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr Vec2 operator-(const Vec2& other) const noexcept {
        return {x - other.x, y - other.y};
    }

    [[nodiscard]] constexpr Vec2 operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar};
    }

    [[nodiscard]] constexpr Vec2 operator/(float scalar) const noexcept {
        return {x / scalar, y / scalar};
    }
};

// 3D Vector for state-space strange attractors
struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Vec3() noexcept = default;
    constexpr Vec3(float px, float py, float pz) noexcept : x(px), y(py), z(pz) {}

    [[nodiscard]] constexpr float magnitude_squared() const noexcept {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] inline float magnitude() const noexcept {
        return std::sqrt(magnitude_squared());
    }

    [[nodiscard]] constexpr Vec3 operator+(const Vec3& o) const noexcept {
        return {x + o.x, y + o.y, z + o.z};
    }

    [[nodiscard]] constexpr Vec3 operator-(const Vec3& o) const noexcept {
        return {x - o.x, y - o.y, z - o.z};
    }

    [[nodiscard]] constexpr Vec3 operator*(float s) const noexcept {
        return {x * s, y * s, z * s};
    }
};

// Stereo frame structure
struct StereoFrame {
    Sample left{0.0f};
    Sample right{0.0f};

    constexpr StereoFrame() noexcept = default;
    constexpr StereoFrame(Sample l, Sample r) noexcept : left(l), right(r) {}

    constexpr StereoFrame operator+(const StereoFrame& o) const noexcept {
        return {left + o.left, right + o.right};
    }

    constexpr StereoFrame operator*(float s) const noexcept {
        return {left * s, right * s};
    }
};

// Real-time audio buffer view
template <typename T>
concept SampleBuffer = requires(T b) {
    { b.data() } -> std::same_as<Sample*>;
    { b.size() } -> std::convertible_to<size_t>;
};

} // namespace aether
