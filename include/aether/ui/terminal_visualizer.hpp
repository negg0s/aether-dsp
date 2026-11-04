#pragma once

#include "../types.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <span>

namespace aether::ui {

/**
 * @brief High-Resolution Terminal Unicode Braille & ANSI TrueColor Visualizer.
 *
 * Utilizes Unicode Braille Patterns (U+2800..U+28FF) where each character cell
 * represents a 2x4 sub-pixel dot matrix, providing a 160x80+ graphical canvas
 * directly inside modern command-line terminals with zero dependencies.
 */
class TerminalVisualizer {
public:
    /**
     * @brief Encode 2x4 subpixel dot grid into a 3-byte UTF-8 Braille character.
     *
     * Dot mapping:
     *   (0,0) -> 0x01   (1,0) -> 0x08
     *   (0,1) -> 0x02   (1,1) -> 0x10
     *   (0,2) -> 0x04   (1,2) -> 0x20
     *   (0,3) -> 0x40   (1,3) -> 0x80
     */
    static std::string braille_char(uint8_t mask) {
        std::string s;
        s.reserve(3);
        s.push_back(static_cast<char>(0xE2));
        s.push_back(static_cast<char>(0xA0 | ((mask >> 6) & 0x03)));
        s.push_back(static_cast<char>(0x80 | (mask & 0x3F)));
        return s;
    }

    /**
     * @brief 2D Sub-pixel Bitmap Canvas for Braille Terminal Rendering.
     */
    class BrailleCanvas {
    public:
        BrailleCanvas(int char_width, int char_height)
            : char_w_(char_width), char_h_(char_height),
              pixel_w_(char_width * 2), pixel_h_(char_height * 4),
              pixels_(pixel_w_ * pixel_h_, false) {}

        [[nodiscard]] int pixel_width() const noexcept { return pixel_w_; }
        [[nodiscard]] int pixel_height() const noexcept { return pixel_h_; }

        void clear() noexcept {
            std::fill(pixels_.begin(), pixels_.end(), false);
        }

        void set_pixel(int px, int py, bool val = true) noexcept {
            if (px >= 0 && px < pixel_w_ && py >= 0 && py < pixel_h_) {
                pixels_[py * pixel_w_ + px] = val;
            }
        }

        /**
         * @brief Render the canvas to an ANSI terminal string with colors.
         */
        [[nodiscard]] std::string render_ansi(const std::string& color_code = "\033[1;36m") const {
            std::string out;
            out += color_code;

            for (int cy = 0; cy < char_h_; ++cy) {
                out += "  ";
                for (int cx = 0; cx < char_w_; ++cx) {
                    uint8_t mask = 0;
                    const int base_x = cx * 2;
                    const int base_y = cy * 4;

                    if (get_pix(base_x, base_y))     mask |= 0x01;
                    if (get_pix(base_x, base_y + 1)) mask |= 0x02;
                    if (get_pix(base_x, base_y + 2)) mask |= 0x04;
                    if (get_pix(base_x + 1, base_y)) mask |= 0x08;
                    if (get_pix(base_x + 1, base_y + 1)) mask |= 0x10;
                    if (get_pix(base_x + 1, base_y + 2)) mask |= 0x20;
                    if (get_pix(base_x, base_y + 3)) mask |= 0x40;
                    if (get_pix(base_x + 1, base_y + 3)) mask |= 0x80;

                    out += braille_char(mask);
                }
                out += "\n";
            }
            out += "\033[0m";
            return out;
        }

    private:
        [[nodiscard]] bool get_pix(int px, int py) const noexcept {
            if (px < 0 || px >= pixel_w_ || py < 0 || py >= pixel_h_) return false;
            return pixels_[py * pixel_w_ + px];
        }

        int char_w_;
        int char_h_;
        int pixel_w_;
        int pixel_h_;
        std::vector<bool> pixels_;
    };

    /**
     * @brief Render Audio Waveform Oscilloscope in Braille.
     */
    static void display_waveform(std::span<const StereoFrame> frames, int char_w = 64, int char_h = 10) {
        BrailleCanvas canvas(char_w, char_h);
        const int px_w = canvas.pixel_width();
        const int px_h = canvas.pixel_height();
        const int mid_y = px_h / 2;

        if (frames.empty()) return;

        float max_peak = 1e-6f;
        for (const auto& f : frames) {
            max_peak = std::max({max_peak, std::abs(f.left), std::abs(f.right)});
        }
        const float norm_scale = max_peak > 1e-4f ? (0.92f / max_peak) : 1.0f;

        const float step = static_cast<float>(frames.size()) / px_w;

        for (int x = 0; x < px_w; ++x) {
            const size_t s_start = static_cast<size_t>(x * step);
            const size_t s_end = std::min(static_cast<size_t>((x + 1) * step), frames.size());

            float local_min = 0.0f;
            float local_max = 0.0f;

            for (size_t s = s_start; s < s_end; ++s) {
                const float mono = (frames[s].left + frames[s].right) * 0.5f * norm_scale;
                if (mono < local_min) local_min = mono;
                if (mono > local_max) local_max = mono;
            }

            const int y1 = std::clamp(mid_y - static_cast<int>(local_max * (mid_y - 1)), 0, px_h - 1);
            const int y2 = std::clamp(mid_y - static_cast<int>(local_min * (mid_y - 1)), 0, px_h - 1);

            if (local_max - local_min > 0.02f) {
                for (int y = y1; y <= y2; ++y) {
                    canvas.set_pixel(x, y, true);
                }
            } else {
                canvas.set_pixel(x, mid_y, (x % 3 == 0)); // Dotted baseline on silence
            }
        }

        std::cout << canvas.render_ansi("\033[1;36m");
    }

    /**
     * @brief Render 2D Poincare Pressure/Displacement Disk in Braille.
     */
    static void display_poincare_disk(const std::vector<float>& grid, int grid_size, int char_size = 28) {
        BrailleCanvas canvas(char_size, char_size / 2);
        const int px_w = canvas.pixel_width();
        const int px_h = canvas.pixel_height();
        const float r_disk = std::min(px_w, px_h) * 0.48f;
        const float cx = px_w * 0.5f;
        const float cy = px_h * 0.5f;

        for (int py = 0; py < px_h; ++py) {
            for (int px = 0; px < px_w; ++px) {
                const float dx = (px - cx) / r_disk;
                const float dy = (py - cy) / r_disk;
                const float r2 = dx * dx + dy * dy;

                if (r2 <= 1.0f) {
                    // Map unit disk to grid
                    const int gx = std::clamp(static_cast<int>((dx * 0.5f + 0.5f) * (grid_size - 1)), 0, grid_size - 1);
                    const int gy = std::clamp(static_cast<int>((dy * 0.5f + 0.5f) * (grid_size - 1)), 0, grid_size - 1);
                    const float val = std::abs(grid[gy * grid_size + gx]);

                    if (val > 0.08f || std::abs(r2 - 1.0f) < 0.06f) {
                        canvas.set_pixel(px, py, true);
                    }
                }
            }
        }

        std::cout << canvas.render_ansi("\033[1;35m");
    }

    /**
     * @brief Render Stereo VU meter bar in ANSI TrueColor.
     */
    static std::string render_vu_meter(float left_peak, float right_peak, int width = 28) {
        auto make_bar = [width](float level) {
            std::string b = "[";
            const int filled = std::clamp(static_cast<int>(level * width), 0, width);
            for (int i = 0; i < width; ++i) {
                if (i < filled) {
                    if (i > width * 0.85f) b += "\033[1;31m█\033[0m";
                    else if (i > width * 0.65f) b += "\033[1;33m█\033[0m";
                    else b += "\033[1;32m█\033[0m";
                } else {
                    b += "\033[90m░\033[0m";
                }
            }
            b += "]";
            return b;
        };

        const float l_db = left_peak > 1e-4f ? 20.0f * std::log10(left_peak) : -80.0f;
        const float r_db = right_peak > 1e-4f ? 20.0f * std::log10(right_peak) : -80.0f;

        std::string res;
        res += "  \033[1mL:\033[0m " + make_bar(std::clamp(left_peak, 0.0f, 1.0f)) + " " + std::to_string(static_cast<int>(l_db)) + " dB\n";
        res += "  \033[1mR:\033[0m " + make_bar(std::clamp(right_peak, 0.0f, 1.0f)) + " " + std::to_string(static_cast<int>(r_db)) + " dB\n";
        return res;
    }

    /**
     * @brief Return waveform Braille visualization as a string for real-time live displays.
     */
    static std::string get_waveform_string(std::span<const StereoFrame> frames, int char_w = 64, int char_h = 8) {
        BrailleCanvas canvas(char_w, char_h);
        const int px_w = canvas.pixel_width();
        const int px_h = canvas.pixel_height();
        const int mid_y = px_h / 2;

        if (frames.empty()) return canvas.render_ansi("\033[1;36m");

        float max_peak = 1e-6f;
        for (const auto& f : frames) {
            max_peak = std::max({max_peak, std::abs(f.left), std::abs(f.right)});
        }
        const float norm_scale = max_peak > 1e-4f ? (0.92f / max_peak) : 1.0f;
        const float step = static_cast<float>(frames.size()) / px_w;

        for (int x = 0; x < px_w; ++x) {
            const size_t s_start = static_cast<size_t>(x * step);
            const size_t s_end = std::min(static_cast<size_t>((x + 1) * step), frames.size());

            float local_min = 0.0f;
            float local_max = 0.0f;
            for (size_t s = s_start; s < s_end; ++s) {
                const float mono = (frames[s].left + frames[s].right) * 0.5f * norm_scale;
                if (mono < local_min) local_min = mono;
                if (mono > local_max) local_max = mono;
            }

            const int y1 = std::clamp(mid_y - static_cast<int>(local_max * (mid_y - 1)), 0, px_h - 1);
            const int y2 = std::clamp(mid_y - static_cast<int>(local_min * (mid_y - 1)), 0, px_h - 1);

            if (local_max - local_min > 0.02f) {
                for (int y = y1; y <= y2; ++y) {
                    canvas.set_pixel(x, y, true);
                }
            } else {
                canvas.set_pixel(x, mid_y, (x % 3 == 0));
            }
        }

        return canvas.render_ansi("\033[1;36m");
    }

    /**
     * @brief Render 2D Time-Frequency Spectrogram in High-Resolution Unicode Braille.
     * Horizontal axis: Frequency [0..4000 Hz]
     * Vertical axis: Time [0..Duration]
     */
    static void display_spectrogram(std::span<const StereoFrame> frames, uint32_t sample_rate, int char_w = 60, int char_h = 16) {
        BrailleCanvas canvas(char_w, char_h);
        const int px_w = canvas.pixel_width();
        const int px_h = canvas.pixel_height();

        if (frames.size() < 256) return;

        constexpr size_t N_FFT = 256;
        const size_t time_step = frames.size() / px_h;
        const float max_freq = 4000.0f;
        const float bin_hz = static_cast<float>(sample_rate) / N_FFT;

        for (int py = 0; py < px_h; ++py) {
            const size_t center_s = std::min(static_cast<size_t>(py * time_step + N_FFT / 2), frames.size() - 1);
            const size_t start_s = center_s >= N_FFT / 2 ? center_s - N_FFT / 2 : 0;

            for (int px = 0; px < px_w; ++px) {
                const float target_f = (static_cast<float>(px) / px_w) * max_freq;
                const int k = std::clamp(static_cast<int>(target_f / bin_hz), 0, static_cast<int>(N_FFT / 2 - 1));
                const float omega = (2.0f * std::numbers::pi_v<float> * k) / N_FFT;

                float real = 0.0f;
                float imag = 0.0f;
                for (size_t n = 0; n < N_FFT && (start_s + n) < frames.size(); n += 2) {
                    const float mono = (frames[start_s + n].left + frames[start_s + n].right) * 0.5f;
                    const float win = 0.5f * (1.0f - std::cos((2.0f * std::numbers::pi_v<float> * n) / N_FFT));
                    real += mono * win * std::cos(omega * n);
                    imag -= mono * win * std::sin(omega * n);
                }

                const float mag = std::sqrt(real * real + imag * imag);
                if (mag > 0.06f) {
                    canvas.set_pixel(px, py, true);
                }
            }
        }

        std::string border;
        border.reserve(char_w * 3);
        for (int i = 0; i < char_w; ++i) border += "\xe2\x94\x80";

        std::cout << "  0 Hz" << std::string(char_w > 14 ? char_w - 14 : 2, ' ') << "4000 Hz\n";
        std::cout << "  \xe2\x94\x8c" << border << "\xe2\x94\x90\n";
        std::cout << canvas.render_ansi("\033[1;33m");
        std::cout << "  \xe2\x94\x94" << border << "\xe2\x94\x98\n";
        std::cout << "  t = 0 s" << std::string(char_w > 18 ? char_w - 18 : 2, ' ') << "t = "
                  << std::fixed << std::setprecision(1) << (static_cast<float>(frames.size()) / sample_rate) << " s\n\n";
    }

    /**
     * @brief Render 2D Phase Space / Poincare Section in High-Resolution Unicode Braille.
     * @param points Vector of (x, y) coordinates
     * @param min_x Minimum x boundary
     * @param max_x Maximum x boundary
     * @param min_y Minimum y boundary
     * @param max_y Maximum y boundary
     * @param char_w Width in character cells
     * @param char_h Height in character cells
     * @param title Plot title
     */
    static void display_phase_space(const std::vector<std::pair<float, float>>& points,
                                    float min_x, float max_x, float min_y, float max_y,
                                    int char_w = 40, int char_h = 16,
                                    const std::string& title = "Phase Space (q, p)") {
        BrailleCanvas canvas(char_w, char_h);
        const int px_w = canvas.pixel_width();
        const int px_h = canvas.pixel_height();

        const float range_x = (max_x - min_x) > 1e-6f ? (max_x - min_x) : 1.0f;
        const float range_y = (max_y - min_y) > 1e-6f ? (max_y - min_y) : 1.0f;

        for (const auto& [x, y] : points) {
            const int px = std::clamp(static_cast<int>(((x - min_x) / range_x) * (px_w - 1)), 0, px_w - 1);
            const int py = std::clamp(static_cast<int>((px_h - 1) - ((y - min_y) / range_y) * (px_h - 1)), 0, px_h - 1);
            canvas.set_pixel(px, py, true);
        }

        std::string border;
        border.reserve(char_w * 3);
        for (int i = 0; i < char_w; ++i) border += "\xe2\x94\x80";

        std::cout << "\033[1;36m  " << title << "\033[0m\n";
        std::cout << "  \xe2\x94\x8c" << border << "\xe2\x94\x90\n";
        std::cout << canvas.render_ansi("\033[1;32m");
        std::cout << "  \xe2\x94\x94" << border << "\xe2\x94\x98\n";
        std::cout << "  x: [" << min_x << ", " << max_x << "]  y: [" << min_y << ", " << max_y << "]\n\n";
    }
};

} // namespace aether::ui
