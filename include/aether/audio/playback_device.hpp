#pragma once

#include "../types.hpp"
#include "../engine.hpp"
#include "../ring_buffer.hpp"
#include "../dsp/scala_catalog.hpp"
#include <memory>
#include <functional>
#include <atomic>
#include <array>
#include <span>
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

    enum class CommandType : uint8_t {
        None,
        LoadPreset,
        NoteOn,
        NoteOnDegree,
        NoteOff,
        AllNotesOff,
        SetScale
    };

    struct PlaybackCommand {
        CommandType type{CommandType::None};
        int param_int{0};
        float param_f1{0.0f};
        float param_f2{0.0f};
        float param_f3{0.0f};
    };

    void post_load_preset(Preset preset) noexcept {
        PlaybackCommand cmd;
        cmd.type = CommandType::LoadPreset;
        cmd.param_int = static_cast<int>(preset);
        (void)command_queue_.push(cmd);
    }

    void post_note_on(Vec2 strike_coord = {0.2f, 0.3f}, float velocity = 0.85f) noexcept {
        PlaybackCommand cmd;
        cmd.type = CommandType::NoteOn;
        cmd.param_f1 = strike_coord.x;
        cmd.param_f2 = strike_coord.y;
        cmd.param_f3 = velocity;
        (void)command_queue_.push(cmd);
    }

    void post_note_on_degree(int degree, float velocity = 0.85f, Vec2 strike_coord = {0.3f, 0.4f}) noexcept {
        PlaybackCommand cmd;
        cmd.type = CommandType::NoteOnDegree;
        cmd.param_int = degree;
        cmd.param_f1 = velocity;
        cmd.param_f2 = strike_coord.x;
        cmd.param_f3 = strike_coord.y;
        (void)command_queue_.push(cmd);
    }

    void post_note_off() noexcept {
        PlaybackCommand cmd;
        cmd.type = CommandType::NoteOff;
        (void)command_queue_.push(cmd);
    }

    void post_all_notes_off() noexcept {
        PlaybackCommand cmd;
        cmd.type = CommandType::AllNotesOff;
        (void)command_queue_.push(cmd);
    }

    void post_set_scale(size_t scale_index) noexcept {
        PlaybackCommand cmd;
        cmd.type = CommandType::SetScale;
        cmd.param_int = static_cast<int>(scale_index);
        (void)command_queue_.push(cmd);
    }

private:
    Engine& engine_;
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    ma_device device_{};
    std::atomic<bool> is_running_{false};
    SPSCRingBuffer<PlaybackCommand, 128> command_queue_{};

    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        (void)pInput;
        auto* self = static_cast<PlaybackDevice*>(pDevice->pUserData);
        auto* out = static_cast<float*>(pOutput);

        // Process thread-safe lock-free control commands prior to audio rendering
        PlaybackCommand cmd;
        while (self->command_queue_.pop(cmd)) {
            switch (cmd.type) {
                case CommandType::LoadPreset:
                    self->engine_.load_preset(static_cast<Preset>(cmd.param_int));
                    break;
                case CommandType::NoteOn:
                    self->engine_.note_on({cmd.param_f1, cmd.param_f2}, cmd.param_f3);
                    break;
                case CommandType::NoteOnDegree:
                    self->engine_.note_on_degree(cmd.param_int, cmd.param_f1, {cmd.param_f2, cmd.param_f3});
                    break;
                case CommandType::NoteOff:
                    self->engine_.note_off();
                    break;
                case CommandType::AllNotesOff:
                    self->engine_.all_notes_off();
                    break;
                case CommandType::SetScale: {
                    const auto& catalog = aether::dsp::ScalaCatalog::get_all_scales();
                    if (!catalog.empty()) {
                        const size_t idx = static_cast<size_t>(cmd.param_int) % catalog.size();
                        const auto& sc = catalog[idx];
                        self->engine_.tuning().load_scale(sc.period, sc.ratios, sc.name);
                    }
                    break;
                }
                case CommandType::None:
                    break;
            }
        }

        // Real-time zero-heap stack buffer rendering
        constexpr size_t CHUNK_SIZE = 64;
        std::array<StereoFrame, CHUNK_SIZE> chunk{};
        ma_uint32 rendered = 0;

        while (rendered < frameCount) {
            const ma_uint32 to_process = std::min(static_cast<ma_uint32>(CHUNK_SIZE), frameCount - rendered);
            self->engine_.process_block(std::span<StereoFrame>(chunk.data(), to_process));
            for (ma_uint32 i = 0; i < to_process; ++i) {
                out[(rendered + i) * 2 + 0] = chunk[i].left;
                out[(rendered + i) * 2 + 1] = chunk[i].right;
            }
            rendered += to_process;
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
