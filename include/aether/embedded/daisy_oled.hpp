#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// Daisy Patch Eurorack OLED Graphic Display (128x64 pixels, monochrome 1-bit)
// Zero-heap, zero-allocation framebuffer with direct rendering primitives
// for oscilloscope waveform monitoring, envelope curves, and parameter menus.
class DaisyOledDisplay {
public:
    static constexpr size_t Width = 128;
    static constexpr size_t Height = 64;
    static constexpr size_t BufferSize = (Width * Height) / 8; // 1024 bytes

    DaisyOledDisplay() {
        clear();
    }

    void clear() noexcept {
        buffer_.fill(0);
    }

    void fill() noexcept {
        buffer_.fill(0xFF);
    }

    void draw_pixel(int x, int y, bool color = true) noexcept {
        if (x < 0 || x >= static_cast<int>(Width) || y < 0 || y >= static_cast<int>(Height)) {
            return;
        }

        // SSD1306 / SH1106 memory organization: 8 vertical pages of 128 horizontal columns
        size_t page = static_cast<size_t>(y) / 8;
        size_t bit = static_cast<size_t>(y) % 8;
        size_t index = page * Width + static_cast<size_t>(x);

        if (color) {
            buffer_[index] |= static_cast<uint8_t>(1 << bit);
        } else {
            buffer_[index] &= static_cast<uint8_t>(~(1 << bit));
        }
    }

    [[nodiscard]] bool get_pixel(int x, int y) const noexcept {
        if (x < 0 || x >= static_cast<int>(Width) || y < 0 || y >= static_cast<int>(Height)) {
            return false;
        }
        size_t page = static_cast<size_t>(y) / 8;
        size_t bit = static_cast<size_t>(y) % 8;
        size_t index = page * Width + static_cast<size_t>(x);
        return (buffer_[index] & (1 << bit)) != 0;
    }

    // Bresenham's line algorithm for audio vector graphics and waveforms
    void draw_line(int x0, int y0, int x1, int y1, bool color = true) noexcept {
        int dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
        int dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            draw_pixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    // Fast horizontal line
    void draw_hline(int x, int y, int width, bool color = true) noexcept {
        if (y < 0 || y >= static_cast<int>(Height) || width <= 0) return;
        int x_end = x + width;
        if (x < 0) x = 0;
        if (x_end > static_cast<int>(Width)) x_end = static_cast<int>(Width);
        for (int curr_x = x; curr_x < x_end; ++curr_x) {
            draw_pixel(curr_x, y, color);
        }
    }

    // Fast vertical line
    void draw_vline(int x, int y, int height, bool color = true) noexcept {
        if (x < 0 || x >= static_cast<int>(Width) || height <= 0) return;
        int y_end = y + height;
        if (y < 0) y = 0;
        if (y_end > static_cast<int>(Height)) y_end = static_cast<int>(Height);
        for (int curr_y = y; curr_y < y_end; ++curr_y) {
            draw_pixel(x, curr_y, color);
        }
    }

    // Draw audio oscilloscope waveform from 128 Q15 audio samples
    void draw_waveform(const math::FixedQ15* samples, size_t count, int y_center = 32, int scale_height = 28) noexcept {
        if (samples == nullptr || count < 2) return;
        size_t steps = (count < Width) ? count : Width;

        int prev_y = y_center - static_cast<int>((static_cast<int64_t>(samples[0].raw) * scale_height) >> 15);
        if (prev_y < 0) prev_y = 0;
        if (prev_y >= static_cast<int>(Height)) prev_y = static_cast<int>(Height) - 1;

        for (size_t x = 1; x < steps; ++x) {
            int y = y_center - static_cast<int>((static_cast<int64_t>(samples[x].raw) * scale_height) >> 15);
            if (y < 0) y = 0;
            if (y >= static_cast<int>(Height)) y = static_cast<int>(Height) - 1;

            draw_line(static_cast<int>(x - 1), prev_y, static_cast<int>(x), y, true);
            prev_y = y;
        }
    }

    [[nodiscard]] const uint8_t* raw_buffer() const noexcept {
        return buffer_.data();
    }

    [[nodiscard]] size_t buffer_size() const noexcept {
        return BufferSize;
    }

private:
    std::array<uint8_t, BufferSize> buffer_{};
};

} // namespace aether::embedded
