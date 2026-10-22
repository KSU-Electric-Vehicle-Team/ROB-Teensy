#pragma once
#include <cmath>         // std::isfinite
#include "PacketIO.hpp"  // crc32 + the structs

// checks one received command. returns NONE if it's good, otherwise why it got rejected
// lastBootId / lastSequence = from the last command we ACCEPTED (both start at 0)
// cmd gets filled in with the decoded packet so you can use it after
inline Networking::RejectReason checkCommand(const uint8_t * buf, int len, Networking::VehicleState state,
                                      uint32_t lastBootId, uint32_t lastSequence,
                                      Networking::Packet<Networking::CommandPayload> & cmd) {
  using Networking::RejectReason;  // lets us write RejectReason:: instead of Networking::RejectReason::

  // wrong size = not a v1 command, don't even look at it
  if (len != sizeof(cmd)) return RejectReason::BAD_LENGTH;
  // raw bytes -> struct. after this we can use field names
  memcpy(&cmd, buf, sizeof(cmd));

  // redo the checksum ourselves. if it doesn't match what they sent, a byte got messed up
  if (crc32(buf, sizeof(cmd) - sizeof(uint32_t)) != cmd.crc32) return RejectReason::BAD_CRC;
  // not our packet, or a layout version we don't understand
  if (cmd.header.magic != Networking::kMagic || cmd.header.version != Networking::kVersion) return RejectReason::BAD_VERSION;
  // not a command, or the header disagrees about the size
  if (cmd.header.type != Networking::MsgType::COMMAND || cmd.header.payload_len != sizeof(cmd.payload)) return RejectReason::BAD_LENGTH;

  // same pc boot: sequence has to go up, otherwise it's old or a duplicate. new pc boot: start over
  if (cmd.header.boot_id == lastBootId && cmd.header.sequence <= lastSequence) return RejectReason::REORDERED;

  const Networking::CommandPayload & p = cmd.payload;  // shorter name
  // NaN or infinity = garbage, never act on it
  if (!std::isfinite(p.motor_setpoint) || !std::isfinite(p.steering_target_rad) || !std::isfinite(p.brake_command)) return RejectReason::OUT_OF_BOUNDS;
  // brake is 0..1. lifetime 0 would be dead on arrival
  if (p.brake_command < 0.0f || p.brake_command > 1.0f || p.lifetime_ms == 0) return RejectReason::OUT_OF_BOUNDS;
  // only one we support right now
  if (p.setpoint_kind != Networking::SetpointKind::MOTOR_RPM) return RejectReason::UNKNOWN_KIND;

  if (p.mode == Networking::DriveMode::STOP) {
    if (p.motor_setpoint != 0.0f) return RejectReason::OUT_OF_BOUNDS;  // stop with a speed makes no sense
    return RejectReason::NONE;  // asking to stop is never wrong, any state
  }
  // unknown mode, or trying to drive and brake at the same time
  if (p.mode != Networking::DriveMode::DRIVE || p.brake_command != 0.0f) return RejectReason::OUT_OF_BOUNDS;
  // the pc only gets to drive in AUTO
  if (state != Networking::VehicleState::AUTO) return RejectReason::WRONG_STATE;
  return RejectReason::NONE;  // passed everything
}
