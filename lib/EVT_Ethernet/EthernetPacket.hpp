// ROB host <-> Teensy packets, v1 (proposal)
// little-endian, packed, every packet = PacketHeader + payload + crc32
// serialize with memcpy, don't cast received buffers to these structs

#pragma once      // only include this file once per build
#include <cstddef>  // offsetof, for the alignment checks at the bottom
#include <cstdint>  // exact-size ints (uint8_t, uint32_t...) so both sides agree on sizes

namespace Networking {  // everything lives in rob:: so names don't clash with the rest of the firmware

constexpr uint16_t kMagic = 0x4252;  // "RB", first 2 bytes of every packet so we can spot ours
constexpr uint8_t kVersion = 1;      // bump on ANY layout change

// which kind of packet this is, goes in the header
enum class MsgType : uint8_t {
  TELEMETRY = 1,  // teensy -> pc, 100 Hz + immediately on state change / new fault
  HEALTH = 2,     // teensy -> pc, ~5 Hz
  INFO = 3,       // teensy -> pc, at boot + every ~5 s
  COMMAND = 4,    // pc -> teensy
};

// firmware state machine state, same numbers as Networking::States
// append only, never renumber
enum class VehicleState : uint8_t {
  NONE = 0, IDLE = 1, RC = 2, AUTO = 3, ERROR = 4, STOP = 5, RESET = 6
};

// what units motor_setpoint is in. only MOTOR_RPM works right now, the rest are for later
enum class SetpointKind : uint8_t {
  NONE = 0, MOTOR_RPM = 1, MOTOR_CURRENT_A = 2, MOTOR_TORQUE_NM = 3, ACCEL_MPS2 = 4
};

// what the pc wants the kart to do
enum class DriveMode : uint8_t {
  STOP = 1,   // setpoint must be 0, teensy picks the brake sequence
  DRIVE = 2,  // brake_command must be 0
};

// why the teensy threw out a command
enum class RejectReason : uint8_t {
  NONE = 0,           // not rejected
  BAD_CRC = 1,        // checksum didn't match, a byte got messed up on the way
  BAD_VERSION = 2,    // wrong magic or version
  BAD_LENGTH = 3,     // wrong size, or not a command
  STALE = 4,          // too old (nothing uses this yet)
  REORDERED = 5,      // same or older sequence than one we already accepted
  OUT_OF_BOUNDS = 6,  // NaN, brake outside 0..1, lifetime 0, bad mode combo
  WRONG_STATE = 7,    // DRIVE while not in AUTO
  UNKNOWN_KIND = 8    // setpoint kind we don't support yet
};

// yes/no flags packed into status_flags, one bit each. check with (status_flags & ARMED)
enum StatusFlag : uint32_t {
  RC_LINK_OK = 1u << 0,         // getting good frames from the rc receiver
  COMMAND_FRESH = 1u << 1,      // teensy's own call, last command still in its lifetime
  ARMED = 1u << 2,              // never set in IDLE / ERROR / STOP
  AUTO_PERMITTED = 1u << 3,     // rc switch / race control allows AUTO
  BRAKE_APPLIED = 1u << 4,      // brake is on
  BRAKE_FEEDBACK_OK = 1u << 5,  // brake_measured is real, not NaN
  ODRIVE_OK = 1u << 6,          // steering controller talking and no errors
  VESC_OK = 1u << 7,            // motor controller talking and no errors
  ESTOP_ACTIVE = 1u << 8,       // an e-stop is pressed
  LIMIT_ACTIVE = 1u << 9,       // teensy is clamping what the pc asked for
};

// fault bits: TBD with integration. fault_active = now, fault_latched = since last re-arm

// first 24 bytes of every packet, same for every type
struct __attribute__((packed)) PacketHeader {  // packed = no hidden padding bytes between fields
  uint16_t magic;        // always kMagic
  uint8_t version;       // always kVersion
  MsgType type;          // which payload comes after the header
  uint16_t payload_len;  // must equal sizeof(payload) for this version/type exactly
  uint16_t reserved;     // send 0, ignore on receive
  uint32_t boot_id;      // random per sender boot (teensy TRNG)
  uint32_t sequence;     // sender's own tx counter, teensy and pc counters are separate
  uint64_t tx_time_us;   // sender monotonic clock at send
};

// when the teensy READ the value, not when the physical thing happened
// age_us == min(tx_time_us - sample_time_us, UINT32_MAX) or drop the packet
// never sampled: sample_time_us = 0, age_us = UINT32_MAX
struct __attribute__((packed)) Stamp {
  uint64_t sample_time_us;  // teensy clock when the value was read
  uint32_t age_us;          // how long ago that was when the packet got sent
};

// teensy -> pc, the fast one
struct __attribute__((packed)) TelemetryPayload {
  VehicleState state;              // current state machine state
  SetpointKind setpoint_kind;      // what motor_setpoint means
  RejectReason last_reject_reason; // why the last rejected command got rejected
  uint8_t reserved0;               // padding, send 0
  uint32_t status_flags;           // StatusFlag bits
  uint32_t fault_active;           // fault bits happening right now
  uint32_t fault_latched;          // only cleared by re-arm

  Stamp last_cmd;              // when the teensy accepted it
  uint32_t last_cmd_sequence;  // pc's sequence number for that command
  uint32_t last_cmd_boot_id;   // pc ignores acks that aren't its current boot_id
  uint32_t cmd_reject_count;   // total commands thrown out since boot

  Stamp encoder;               // when encoder_count was read
  int32_t encoder_count;       // raw since boot, 2400 counts/rev. diff with wraparound

  Stamp motor;                 // when motor_erpm was read
  float motor_erpm;            // raw vesc eRPM, pc converts with pole pairs

  Stamp steering;              // when steering_angle_rad was read
  float steering_angle_rad;    // measured, +left, per INFO steering_calibration_id

  Stamp brake;                 // when brake_measured was read
  float brake_measured;        // 0..1, NaN until we have a sensor

  Stamp rc;                    // when the rc values were read
  uint16_t rc_throttle_raw;    // raw sbus value
  uint16_t rc_steering_raw;    // raw sbus value

  // what the teensy is actually applying after its limits, not an echo
  float motor_setpoint;        // units per setpoint_kind
  float steering_target_rad;   // what the odrive is being told
  float brake_command;         // 0..1
  uint32_t reserved1;          // padding, send 0
};

// teensy -> pc, slow stuff
struct __attribute__((packed)) HealthPayload {
  Stamp vesc;                  // when the vesc values below were read
  float vesc_input_voltage_v;  // race control wants drivetrain power
  float vesc_input_current_a;  // voltage x current = power
  float vesc_temp_c;           // controller temp
  float motor_temp_c;          // motor temp
  uint32_t reserved0;          // padding, send 0

  Stamp odrive;                // when odrive_error was read
  uint32_t odrive_error;       // raw odrive error bits

  uint32_t tx_count;           // packets the teensy has sent since boot
  uint32_t rx_ok_count;        // commands accepted since boot
  uint32_t rx_reject_count;    // commands rejected since boot
  uint32_t reserved1;          // padding, send 0
};

// teensy -> pc, stuff that doesn't change while running
struct __attribute__((packed)) InfoPayload {
  uint8_t firmware_git_sha[20];      // which firmware build is flashed, zeros until we wire it up
  uint32_t steering_calibration_id;  // which steering calibration made steering_angle_rad
  uint32_t encoder_counts_per_rev;   // 2400
  uint16_t motor_pole_pairs;         // for eRPM -> RPM, 0 until we know it
  uint16_t reserved;                 // padding, send 0
};

// pc -> teensy
struct __attribute__((packed)) CommandPayload {
  uint32_t lifetime_ms;        // counted from teensy receipt, not pc send time
  DriveMode mode;              // STOP or DRIVE
  SetpointKind setpoint_kind;  // what motor_setpoint means
  uint16_t reserved;           // padding, send 0
  float motor_setpoint;        // units per setpoint_kind
  float steering_target_rad;   // +left
  float brake_command;         // 0..1
};

// a full packet: header, then whichever payload, then the checksum
// crc32 = CRC-32/ISO-HDLC (same as zlib.crc32) over header + payload
template <typename Payload>  // Packet<TelemetryPayload>, Packet<CommandPayload>, etc
struct __attribute__((packed)) Packet {
  PacketHeader header;
  Payload payload;
  uint32_t crc32;
};

// build fails if a size is wrong. if one fails, someone changed the layout
static_assert(sizeof(PacketHeader) == 24);
static_assert(sizeof(Stamp) == 12);
static_assert(sizeof(TelemetryPayload) == 136);
static_assert(sizeof(HealthPayload) == 64);
static_assert(sizeof(InfoPayload) == 32);
static_assert(sizeof(CommandPayload) == 20);
static_assert(sizeof(Packet<TelemetryPayload>) == 164);

// uint64s stay 8-byte aligned in the packet
static_assert(offsetof(Packet<TelemetryPayload>, payload.last_cmd) % 8 == 0);
static_assert(offsetof(Packet<TelemetryPayload>, payload.encoder) % 8 == 0);
static_assert(offsetof(Packet<TelemetryPayload>, payload.rc) % 8 == 0);
static_assert(offsetof(Packet<HealthPayload>, payload.odrive) % 8 == 0);

}  // namespace Networking
