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

} // namespace aether::audio
