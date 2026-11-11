#pragma once

#include "types.hpp"
#include <fstream>
#include <vector>
#include <span>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>

namespace aether {

/**
 * @brief High-precision lossless RIFF WAV audio writer.
 * Supports 32-bit IEEE Float and 24-bit PCM stereo output.
 */
class WavWriter {
public:
    enum class Format : uint16_t {
        PCM_16 = 1,
        PCM_24 = 1,
        IEEE_FLOAT_32 = 3
    };

    static bool write_stereo_wav(const std::string& filepath,
                                 std::span<const StereoFrame> frames,
                                 uint32_t sample_rate = DEFAULT_SAMPLE_RATE,
                                 bool use_float = true) {
        if (frames.size() > 0x1FFFFFFF) return false;
        sample_rate = std::clamp(sample_rate, 1000u, 384000u);

        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        const uint16_t num_channels = 2;
        const uint32_t num_frames = static_cast<uint32_t>(frames.size());

        if (use_float) {
            const uint16_t bits_per_sample = 32;
            const uint16_t block_align = num_channels * (bits_per_sample / 8);
            const uint32_t byte_rate = sample_rate * block_align;
            const uint32_t data_size = num_frames * block_align;
            const uint32_t chunk_size = 36 + data_size;

            // RIFF Header
            file.write("RIFF", 4);
            file.write(reinterpret_cast<const char*>(&chunk_size), 4);
            file.write("WAVE", 4);

            // fmt subchunk
            file.write("fmt ", 4);
            const uint32_t subchunk1_size = 16;
            const uint16_t audio_format = static_cast<uint16_t>(Format::IEEE_FLOAT_32);
            file.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
            file.write(reinterpret_cast<const char*>(&audio_format), 2);
            file.write(reinterpret_cast<const char*>(&num_channels), 2);
            file.write(reinterpret_cast<const char*>(&sample_rate), 4);
            file.write(reinterpret_cast<const char*>(&byte_rate), 4);
            file.write(reinterpret_cast<const char*>(&block_align), 2);
            file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);

            // data subchunk
            file.write("data", 4);
            file.write(reinterpret_cast<const char*>(&data_size), 4);

            for (const auto& frame : frames) {
                float left = std::isfinite(frame.left) ? frame.left : 0.0f;
                float right = std::isfinite(frame.right) ? frame.right : 0.0f;
                file.write(reinterpret_cast<const char*>(&left), sizeof(float));
                file.write(reinterpret_cast<const char*>(&right), sizeof(float));
            }
        } else {
            // 24-bit PCM
            const uint16_t bits_per_sample = 24;
            const uint16_t block_align = num_channels * 3;
            const uint32_t byte_rate = sample_rate * block_align;
            const uint32_t data_size = num_frames * block_align;
            const uint32_t chunk_size = 36 + data_size;

            file.write("RIFF", 4);
            file.write(reinterpret_cast<const char*>(&chunk_size), 4);
            file.write("WAVE", 4);

            file.write("fmt ", 4);
            const uint32_t subchunk1_size = 16;
            const uint16_t audio_format = static_cast<uint16_t>(Format::PCM_24);
            file.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
            file.write(reinterpret_cast<const char*>(&audio_format), 2);
            file.write(reinterpret_cast<const char*>(&num_channels), 2);
            file.write(reinterpret_cast<const char*>(&sample_rate), 4);
            file.write(reinterpret_cast<const char*>(&byte_rate), 4);
            file.write(reinterpret_cast<const char*>(&block_align), 2);
            file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);

            file.write("data", 4);
            file.write(reinterpret_cast<const char*>(&data_size), 4);

            for (const auto& frame : frames) {
                auto write_sample24 = [&](float s) {
                    float finite_s = std::isfinite(s) ? s : 0.0f;
                    float clamped = std::clamp(finite_s, -1.0f, 1.0f);
                    int32_t sample24 = static_cast<int32_t>(clamped * 8388607.0f);
                    uint8_t bytes[3];
                    bytes[0] = static_cast<uint8_t>(sample24 & 0xFF);
                    bytes[1] = static_cast<uint8_t>((sample24 >> 8) & 0xFF);
                    bytes[2] = static_cast<uint8_t>((sample24 >> 16) & 0xFF);
                    file.write(reinterpret_cast<const char*>(bytes), 3);
                };
                write_sample24(frame.left);
                write_sample24(frame.right);
            }
        }

        return file.good();
    }
};

} // namespace aether
