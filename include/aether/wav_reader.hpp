#pragma once

#include "types.hpp"
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <iostream>

namespace aether {

/**
 * @brief High-precision lossless RIFF WAV audio reader (Pure C++20).
 * Supports 16-bit PCM, 24-bit PCM, and 32-bit IEEE Float (Mono & Stereo).
 */
class WavReader {
public:
    struct AudioData {
        std::vector<StereoFrame> frames{};
        uint32_t sample_rate{DEFAULT_SAMPLE_RATE};
        uint16_t num_channels{2};
        bool valid{false};
    };

    static AudioData read_wav(const std::string& filepath) {
        AudioData result{};
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) return result;

        char chunk_id[4];
        file.read(chunk_id, 4);
        if (std::memcmp(chunk_id, "RIFF", 4) != 0) return result;

        uint32_t chunk_size = 0;
        file.read(reinterpret_cast<char*>(&chunk_size), 4);

        char format[4];
        file.read(format, 4);
        if (std::memcmp(format, "WAVE", 4) != 0) return result;

        uint16_t audio_format = 0;
        uint16_t num_channels = 0;
        uint32_t sample_rate = 0;
        uint16_t bits_per_sample = 0;
        bool found_fmt = false;

        while (file.read(chunk_id, 4)) {
            uint32_t subchunk_size = 0;
            file.read(reinterpret_cast<char*>(&subchunk_size), 4);

            if (std::memcmp(chunk_id, "fmt ", 4) == 0) {
                file.read(reinterpret_cast<char*>(&audio_format), 2);
                file.read(reinterpret_cast<char*>(&num_channels), 2);
                file.read(reinterpret_cast<char*>(&sample_rate), 4);

                uint32_t byte_rate = 0;
                uint16_t block_align = 0;
                file.read(reinterpret_cast<char*>(&byte_rate), 4);
                file.read(reinterpret_cast<char*>(&block_align), 2);
                file.read(reinterpret_cast<char*>(&bits_per_sample), 2);

                if (subchunk_size > 16) {
                    file.seekg(subchunk_size - 16, std::ios::cur);
                }
                found_fmt = true;
            } else if (std::memcmp(chunk_id, "data", 4) == 0 && found_fmt) {
                const size_t bytes_per_sample = bits_per_sample / 8;
                const size_t total_samples = subchunk_size / bytes_per_sample;
                const size_t total_frames = total_samples / num_channels;

                result.frames.resize(total_frames);
                result.sample_rate = sample_rate;
                result.num_channels = num_channels;

                std::vector<uint8_t> raw_data(subchunk_size);
                file.read(reinterpret_cast<char*>(raw_data.data()), subchunk_size);

                size_t byte_offset = 0;
                for (size_t f = 0; f < total_frames; ++f) {
                    float left = 0.0f;
                    float right = 0.0f;

                    for (uint16_t ch = 0; ch < num_channels; ++ch) {
                        float sample_val = 0.0f;

                        if (audio_format == 3 && bits_per_sample == 32) {
                            // 32-bit Float
                            std::memcpy(&sample_val, &raw_data[byte_offset], 4);
                            byte_offset += 4;
                        } else if (audio_format == 1 && bits_per_sample == 16) {
                            // 16-bit PCM
                            int16_t s16 = 0;
                            std::memcpy(&s16, &raw_data[byte_offset], 2);
                            sample_val = static_cast<float>(s16) / 32768.0f;
                            byte_offset += 2;
                        } else if (audio_format == 1 && bits_per_sample == 24) {
                            // 24-bit PCM
                            int32_t s24 = (static_cast<int32_t>(raw_data[byte_offset + 0])) |
                                          (static_cast<int32_t>(raw_data[byte_offset + 1]) << 8) |
                                          (static_cast<int32_t>(raw_data[byte_offset + 2]) << 16);
                            if (s24 & 0x800000) s24 |= 0xFF000000;
                            sample_val = static_cast<float>(s24) / 8388608.0f;
                            byte_offset += 3;
                        } else {
                            byte_offset += bytes_per_sample;
                        }

                        if (ch == 0) left = sample_val;
                        if (ch == 1) right = sample_val;
                    }

                    if (num_channels == 1) right = left;
                    result.frames[f] = StereoFrame{left, right};
                }

                result.valid = true;
                break;
            } else {
                file.seekg(subchunk_size, std::ios::cur);
            }
        }

        return result;
    }
};

} // namespace aether
