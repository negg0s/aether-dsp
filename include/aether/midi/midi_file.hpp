#pragma once

#include "../types.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <iostream>
#include <algorithm>

namespace aether::midi {

enum class EventType : uint8_t {
    NoteOff = 0x80,
    NoteOn = 0x90,
    PolyAftertouch = 0xA0,
    ControlChange = 0xB0,
    ProgramChange = 0xC0,
    ChannelAftertouch = 0xD0,
    PitchBend = 0xE0,
    Meta = 0xFF
};

struct MidiEvent {
    uint32_t delta_ticks{0};
    uint8_t status{0};
    uint8_t channel{0};
    uint8_t data1{0};
    uint8_t data2{0};
    std::vector<uint8_t> meta_data{};
};

struct MidiTrack {
    std::string name{"Track"};
    std::vector<MidiEvent> events{};
};

/**
 * @brief Standard MIDI File (SMF Format 0/1) Parser and Serializer (Pure C++20).
 */
class MidiFile {
public:
    uint16_t format{1};
    uint16_t ticks_per_quarter_note{480};
    std::vector<MidiTrack> tracks{};

    /**
     * @brief Save MIDI sequence to a standard .mid binary file.
     */
    bool save(const std::string& filepath) const {
        std::ofstream out(filepath, std::ios::binary);
        if (!out.is_open()) return false;

        // 1. MThd Chunk
        out.write("MThd", 4);
        write_be32(out, 6);
        write_be16(out, format);
        write_be16(out, static_cast<uint16_t>(tracks.size()));
        write_be16(out, ticks_per_quarter_note);

        // 2. MTrk Chunks
        for (const auto& track : tracks) {
            std::vector<uint8_t> track_data;

            for (const auto& ev : track.events) {
                write_vlq(track_data, ev.delta_ticks);

                if (ev.status == 0xFF) {
                    // Meta event
                    track_data.push_back(0xFF);
                    track_data.push_back(ev.data1); // Meta type
                    write_vlq(track_data, static_cast<uint32_t>(ev.meta_data.size()));
                    track_data.insert(track_data.end(), ev.meta_data.begin(), ev.meta_data.end());
                } else {
                    // Voice event
                    track_data.push_back(static_cast<uint8_t>(ev.status | (ev.channel & 0x0F)));
                    track_data.push_back(ev.data1);
                    if (ev.status != 0xC0 && ev.status != 0xD0) {
                        track_data.push_back(ev.data2);
                    }
                }
            }

            // End of Track meta event: 00 FF 2F 00
            write_vlq(track_data, 0);
            track_data.push_back(0xFF);
            track_data.push_back(0x2F);
            track_data.push_back(0x00);

            // Write MTrk header
            out.write("MTrk", 4);
            write_be32(out, static_cast<uint32_t>(track_data.size()));
            out.write(reinterpret_cast<const char*>(track_data.data()), track_data.size());
        }

        return out.good();
    }

    /**
     * @brief Load and parse a standard .mid file.
     */
    bool load(const std::string& filepath) {
        std::ifstream in(filepath, std::ios::binary);
        if (!in.is_open()) return false;

        char chunk_id[4];
        in.read(chunk_id, 4);
        if (std::string(chunk_id, 4) != "MThd") return false;

        uint32_t header_len = read_be32(in);
        (void)header_len;
        format = read_be16(in);
        uint16_t num_tracks = read_be16(in);
        ticks_per_quarter_note = read_be16(in);

        tracks.clear();
        for (uint16_t t = 0; t < num_tracks && in.good(); ++t) {
            in.read(chunk_id, 4);
            if (std::string(chunk_id, 4) != "MTrk") break;

            uint32_t track_len = read_be32(in);
            std::vector<uint8_t> buffer(track_len);
            in.read(reinterpret_cast<char*>(buffer.data()), track_len);

            MidiTrack track;
            size_t cursor = 0;
            uint8_t running_status = 0;

            while (cursor < buffer.size()) {
                uint32_t delta = read_vlq(buffer, cursor);
                if (cursor >= buffer.size()) break;

                uint8_t status_byte = buffer[cursor];
                if (status_byte & 0x80) {
                    running_status = status_byte;
                    cursor++;
                }

                if (running_status == 0xFF) {
                    // Meta event
                    uint8_t meta_type = buffer[cursor++];
                    uint32_t len = read_vlq(buffer, cursor);
                    std::vector<uint8_t> meta_data;
                    if (cursor + len <= buffer.size()) {
                        meta_data.assign(buffer.begin() + cursor, buffer.begin() + cursor + len);
                        cursor += len;
                    }
                    if (meta_type == 0x2F) break; // End of track delimiter
                    track.events.push_back({delta, 0xFF, 0, meta_type, 0, meta_data});
                } else {
                    uint8_t type = running_status & 0xF0;
                    uint8_t channel = running_status & 0x0F;
                    uint8_t d1 = buffer[cursor++];
                    uint8_t d2 = 0;
                    if (type != 0xC0 && type != 0xD0 && cursor < buffer.size()) {
                        d2 = buffer[cursor++];
                    }
                    track.events.push_back({delta, type, channel, d1, d2, {}});
                }
            }

            tracks.push_back(std::move(track));
        }

        return true;
    }

private:
    static void write_be16(std::ofstream& out, uint16_t val) {
        char bytes[2] = {static_cast<char>((val >> 8) & 0xFF), static_cast<char>(val & 0xFF)};
        out.write(bytes, 2);
    }

    static void write_be32(std::ofstream& out, uint32_t val) {
        char bytes[4] = {
            static_cast<char>((val >> 24) & 0xFF),
            static_cast<char>((val >> 16) & 0xFF),
            static_cast<char>((val >> 8) & 0xFF),
            static_cast<char>(val & 0xFF)
        };
        out.write(bytes, 4);
    }

    static uint16_t read_be16(std::ifstream& in) {
        uint8_t b[2];
        in.read(reinterpret_cast<char*>(b), 2);
        return (static_cast<uint16_t>(b[0]) << 8) | b[1];
    }

    static uint32_t read_be32(std::ifstream& in) {
        uint8_t b[4];
        in.read(reinterpret_cast<char*>(b), 4);
        return (static_cast<uint32_t>(b[0]) << 24) |
               (static_cast<uint32_t>(b[1]) << 16) |
               (static_cast<uint32_t>(b[2]) << 8) |
               b[3];
    }

    static void write_vlq(std::vector<uint8_t>& buf, uint32_t value) {
        uint32_t buffer = value & 0x7F;
        while ((value >>= 7) > 0) {
            buffer <<= 8;
            buffer |= 0x80;
            buffer += (value & 0x7F);
        }
        while (true) {
            buf.push_back(static_cast<uint8_t>(buffer & 0xFF));
            if (buffer & 0x80) buffer >>= 8;
            else break;
        }
    }

    static uint32_t read_vlq(const std::vector<uint8_t>& buf, size_t& cursor) {
        uint32_t value = 0;
        uint8_t byte = 0;
        do {
            if (cursor >= buf.size()) return value;
            byte = buf[cursor++];
            value = (value << 7) | (byte & 0x7F);
        } while (byte & 0x80);
        return value;
    }
};

} // namespace aether::midi
