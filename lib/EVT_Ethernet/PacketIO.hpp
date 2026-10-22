#pragma once                   // only include this file once per build
#include "arduino_freertos.h"  // micros(), taskENTER_CRITICAL, Serial
#include <Entropy.h>           // teensy hardware random numbers, for boot_id
#include <cstring>             // memcpy
#include "EthernetPacket.hpp"       // the packet structs

// micros() is 32 bits so it wraps back to 0 every ~71 min here is fix lololol
// only works if something calls it at least once per 71 min. the ethernet task calls it 100x a second so we're good
inline uint64_t micros64() {
  static uint32_t last = 0;              // micros() from the previous call, static = remembered between calls
  static uint64_t high = 0;              // how many times micros() has wrapped, already shifted into the top 32 bits
  taskENTER_CRITICAL();                  // pause other tasks so two of them can't update last/high at the same time
  uint32_t now = arduino::micros();      // current 32-bit time
  if (now < last) high += (1ULL << 32);  // time went backwards = it wrapped, add one full lap (2^32 us)
  last = now;                            // remember for next call
  uint64_t t = high | now;               // lap count on top, current time on the bottom
  taskEXIT_CRITICAL();                   // let other tasks run again
  return t;
}

// checksum so the receiver can tell if any byte got changed on the way
// CRC-32/ISO-HDLC, same as python zlib.crc32. crc32("123456789") == 0xCBF43926
inline uint32_t crc32(const uint8_t * data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;          // standard starting value for this crc
  for (size_t i = 0; i < len; i++) {  // every byte
    crc ^= data[i];                   // mix the byte into the low 8 bits
    for (int b = 0; b < 8; b++) {     // then crunch those 8 bits one at a time
      // shift right. if the bit that fell off was 1, xor in the crc polynomial
      // -(crc & 1) is all 1s or all 0s, so it does that without an if
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }
  return ~crc;                        // flip every bit at the end, also part of the standard
}

// call this right when you read a sensor value, records when we read it
inline void stamp(Networking::Stamp & s) { s.sample_time_us = micros64(); }

// fills in age_us right before sending: how long ago the value was read
inline void setAge(Networking::Stamp & s, uint64_t txTime) {
  if (s.sample_time_us == 0) { s.age_us = UINT32_MAX; return; }  // 0 = never read, max age = "never sampled"
  uint64_t age = txTime - s.sample_time_us;                        // send time minus read time
  s.age_us = age > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(age);  // only 32 bits to fit in, cap it instead of letting it wrap
}

// turns a payload struct into bytes ready to send. returns how many bytes to send
// out has to be at least sizeof(Networking::Packet<Payload>) bytes
template <typename Payload>  // works for any payload: telemetry, health, info, command
size_t buildPacket(Networking::MsgType type, const Payload & payload, uint32_t bootId,
                   uint32_t sequence, uint64_t txTime, uint8_t * out) {
  Networking::Packet<Payload> pkt {};               // header + payload + crc. {} zeroes everything, reserved fields included
  pkt.header.magic = Networking::kMagic;            // "RB"
  pkt.header.version = Networking::kVersion;        // layout version
  pkt.header.type = type;                    // which payload this is
  pkt.header.payload_len = sizeof(Payload);  // payload size so the receiver can double check
  pkt.header.boot_id = bootId;               // random number we picked at boot
  pkt.header.sequence = sequence;            // our packet counter
  pkt.header.tx_time_us = txTime;            // use the same micros64() value you gave setAge
  pkt.payload = payload;                     // copy the actual data in
  memcpy(out, &pkt, sizeof(pkt));            // struct -> raw bytes
  uint32_t crc = crc32(out, sizeof(pkt) - sizeof(uint32_t));        // checksum everything except the last 4 bytes
  memcpy(out + sizeof(pkt) - sizeof(uint32_t), &crc, sizeof(crc));  // write the crc into those last 4 bytes
  return sizeof(pkt);                        // total bytes to send
}
