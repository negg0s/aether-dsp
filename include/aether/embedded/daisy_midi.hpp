#pragma once

#include <cstdint>
#include <cstddef>
#include "aether/math/fixed_point.hpp"

namespace aether::embedded {

// MIDI message types
enum class MidiMessageType : uint8_t {
    None = 0x00,
    NoteOff = 0x80,
    NoteOn = 0x90,
    PolyAftertouch = 0xA0,
    ControlChange = 0xB0,
    ProgramChange = 0xC0,
    ChannelAftertouch = 0xD0,
    PitchBend = 0xE0,
    SystemRealTime = 0xF8
};

struct MidiEvent {
    MidiMessageType type{MidiMessageType::None};
    uint8_t channel{0}; // 0 - 15
    uint8_t data1{0};   // Note number or CC number
    uint8_t data2{0};   // Velocity or CC value
    int16_t value14{0}; // For 14-bit pitch bend (-8192 to 8191)
};

// Zero-allocation streaming MIDI 1.0 byte parser with running status support.
class DaisyMidiParser {
public:
    DaisyMidiParser() = default;

    // Feed a single byte from UART/USB CDC stream into parser.
    // Returns true when a complete valid MIDI event is assembled.
    bool parse_byte(uint8_t byte, MidiEvent& out_event) noexcept {
        // Real-time messages (0xF8 - 0xFF) can arrive anywhere, even interleaved in multi-byte messages
        if (byte >= 0xF8) {
            out_event.type = MidiMessageType::SystemRealTime;
            out_event.channel = 0;
            out_event.data1 = byte;
            out_event.data2 = 0;
            out_event.value14 = 0;
            return true;
        }

        // Status byte (bit 7 set)
        if (byte & 0x80) {
            // System common / sysex cancels running status
            if (byte >= 0xF0) {
                running_status_ = 0;
                expected_bytes_ = 0;
                data_count_ = 0;
                return false;
            }

            running_status_ = byte;
            data_count_ = 0;
            expected_bytes_ = get_expected_data_bytes(running_status_);
            return false;
        }

        // Data byte (bit 7 clear)
        if (running_status_ == 0) {
            // Stray data byte with no active status byte
            return false;
        }

        if (data_count_ < 2) {
            data_buffer_[data_count_++] = byte;
        }

        if (data_count_ == expected_bytes_) {
            out_event.channel = running_status_ & 0x0F;
            out_event.type = static_cast<MidiMessageType>(running_status_ & 0xF0);
            out_event.data1 = data_buffer_[0];
            out_event.data2 = (expected_bytes_ == 2) ? data_buffer_[1] : 0;

            if (out_event.type == MidiMessageType::PitchBend) {
                // Pitch bend: 14-bit value, LSB first (data1), MSB second (data2)
                // Range: 0 to 16383, center is 8192 -> mapped to -8192 to 8191
                int32_t raw14 = (static_cast<int32_t>(out_event.data2) << 7) | out_event.data1;
                out_event.value14 = static_cast<int16_t>(raw14 - 8192);
            } else {
                out_event.value14 = 0;
            }

            // In MIDI 1.0 NoteOn with velocity 0 is NoteOff
            if (out_event.type == MidiMessageType::NoteOn && out_event.data2 == 0) {
                out_event.type = MidiMessageType::NoteOff;
            }

            // Reset data count for subsequent running status messages
            data_count_ = 0;
            return true;
        }

        return false;
    }

    void reset() noexcept {
        running_status_ = 0;
        data_count_ = 0;
        expected_bytes_ = 0;
    }

private:
    static uint8_t get_expected_data_bytes(uint8_t status) noexcept {
        uint8_t cmd = status & 0xF0;
        switch (cmd) {
            case 0x80: // Note Off: note, velocity
            case 0x90: // Note On: note, velocity
            case 0xA0: // Poly Aftertouch: note, pressure
            case 0xB0: // Control Change: cc, value
            case 0xE0: // Pitch Bend: lsb, msb
                return 2;
            case 0xC0: // Program Change: program
            case 0xD0: // Channel Aftertouch: pressure
                return 1;
            default:
                return 0;
        }
    }

    uint8_t running_status_{0};
    uint8_t data_buffer_[2]{0, 0};
    uint8_t data_count_{0};
    uint8_t expected_bytes_{0};
};

} // namespace aether::embedded
