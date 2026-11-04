#pragma once

#include "../types.hpp"
#include "../engine.hpp"
#include <memory>
#include <functional>
#include <atomic>
#include <iostream>

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#include "miniaudio.h"

namespace aether::audio {

/**
 * @brief High-performance real-time cross-platform audio playback device.
 * Wraps miniaudio with zero-latency lock-free callbacks.
 */
class PlaybackDevice {
public:
    explicit PlaybackDevice(Engine& engine, uint32_t sample_rate = DEFAULT_SAMPLE_RATE)
        : engine_(engine), sample_rate_(sample_rate) {}

    ~PlaybackDevice() {
        stop();
    }

    PlaybackDevice(const PlaybackDevice&) = delete;
    PlaybackDevice& operator=(const PlaybackDevice&) = delete;

    bool start() {
        if (is_running_.load(std::memory_order_relaxed)) {
            return true;
        }

        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = 2;
        config.sampleRate        = sample_rate_;
        config.dataCallback      = data_callback;
        config.pUserData         = this;
        config.periodSizeInFrames = DEFAULT_BLOCK_SIZE;

        if (ma_device_init(nullptr, &config, &device_) != MA_SUCCESS) {
            std::cerr << "[ERROR] Failed to initialize audio playback device.\n";
            return false;
        }

        if (ma_device_start(&device_) != MA_SUCCESS) {
            std::cerr << "[ERROR] Failed to start audio playback device.\n";
            ma_device_uninit(&device_);
            return false;
        }

        is_running_.store(true, std::memory_order_release);
        return true;
    }

    void stop() {
        if (is_running_.exchange(false, std::memory_order_acq_rel)) {
            ma_device_stop(&device_);
            ma_device_uninit(&device_);
        }
    }

    [[nodiscard]] bool is_running() const noexcept {
        return is_running_.load(std::memory_order_relaxed);
    }

private:
    Engine& engine_;
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    ma_device device_{};
    std::atomic<bool> is_running_{false};

    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        (void)pInput;
        auto* self = static_cast<PlaybackDevice*>(pDevice->pUserData);
        auto* out = static_cast<float*>(pOutput);

        std::vector<StereoFrame> block(frameCount);
        self->engine_.process_block(block);

        for (ma_uint32 i = 0; i < frameCount; ++i) {
            out[i * 2 + 0] = block[i].left;
            out[i * 2 + 1] = block[i].right;
        }
    }
};

/**
 * @brief High-precision real-time audio playback hardware device for recorded WAV buffers.
 */
class WavPlayerDevice {
public:
    explicit WavPlayerDevice(const std::vector<StereoFrame>& frames, uint32_t sample_rate = DEFAULT_SAMPLE_RATE)
        : frames_(frames), sample_rate_(sample_rate) {}

    ~WavPlayerDevice() { stop(); }

    WavPlayerDevice(const WavPlayerDevice&) = delete;
    WavPlayerDevice& operator=(const WavPlayerDevice&) = delete;

    bool start() {
        if (is_running_.load(std::memory_order_relaxed)) return true;

        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = 2;
        config.sampleRate        = sample_rate_;
        config.dataCallback      = data_callback;
        config.pUserData         = this;
        config.periodSizeInFrames = DEFAULT_BLOCK_SIZE;

        if (ma_device_init(nullptr, &config, &device_) != MA_SUCCESS) {
            std::cerr << "[ERROR] Failed to initialize WAV audio playback hardware.\n";
            return false;
        }

        if (ma_device_start(&device_) != MA_SUCCESS) {
            std::cerr << "[ERROR] Failed to start WAV audio playback hardware.\n";
            ma_device_uninit(&device_);
            return false;
        }

        is_running_.store(true, std::memory_order_release);
        return true;
    }

    void stop() {
        if (is_running_.exchange(false, std::memory_order_acq_rel)) {
            ma_device_stop(&device_);
            ma_device_uninit(&device_);
        }
    }

    [[nodiscard]] size_t current_frame() const noexcept {
        return cursor_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool is_finished() const noexcept {
        return cursor_.load(std::memory_order_relaxed) >= frames_.size();
    }

    [[nodiscard]] float current_peak_left() const noexcept { return peak_l_.load(std::memory_order_relaxed); }
    [[nodiscard]] float current_peak_right() const noexcept { return peak_r_.load(std::memory_order_relaxed); }

private:
    const std::vector<StereoFrame>& frames_;
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    ma_device device_{};
    std::atomic<bool> is_running_{false};
    std::atomic<size_t> cursor_{0};
    std::atomic<float> peak_l_{0.0f};
    std::atomic<float> peak_r_{0.0f};

    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        (void)pInput;
        auto* self = static_cast<WavPlayerDevice*>(pDevice->pUserData);
        auto* out = static_cast<float*>(pOutput);

        size_t cur = self->cursor_.load(std::memory_order_relaxed);
        float p_l = 0.0f;
        float p_r = 0.0f;

        for (ma_uint32 i = 0; i < frameCount; ++i) {
            if (cur < self->frames_.size()) {
                const auto& f = self->frames_[cur++];
                out[i * 2 + 0] = f.left;
                out[i * 2 + 1] = f.right;
                if (std::abs(f.left) > p_l) p_l = std::abs(f.left);
                if (std::abs(f.right) > p_r) p_r = std::abs(f.right);
            } else {
                out[i * 2 + 0] = 0.0f;
                out[i * 2 + 1] = 0.0f;
            }
        }
        self->cursor_.store(cur, std::memory_order_relaxed);
        self->peak_l_.store(p_l, std::memory_order_relaxed);
        self->peak_r_.store(p_r, std::memory_order_relaxed);
    }
};

} // namespace aether::audio
