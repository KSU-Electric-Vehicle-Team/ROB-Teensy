#include "EVT_Vesc.hpp"

#include <algorithm>
#include <cmath>

namespace MotorControls {
  namespace {
    // VESC UART command IDs (COMM_PACKET_ID in the VESC firmware)
    const uint8_t commGetValues = 4;
    const uint8_t commSetCurrent = 6;
    const uint8_t commSetCurrentBrake = 7;
    const uint8_t commSetRPM = 8;

    const uint8_t startByte = 2; // Start byte of a short frame (payload of 255 bytes or less)
    const uint8_t endByte = 3;   // End byte of every frame

    const uint8_t valuesLength = 54; // Payload bytes needed to reach the fault code in COMM_GET_VALUES


    int16_t readInt16(const uint8_t * buffer, size_t & index) {
      const int16_t value = static_cast<int16_t>((buffer[index] << 8) | buffer[index + 1]);
      index += 2;
      return value;
    }


    int32_t readInt32(const uint8_t * buffer, size_t & index) {
      const int32_t value = static_cast<int32_t>(
        (static_cast<uint32_t>(buffer[index]) << 24) |
        (static_cast<uint32_t>(buffer[index + 1]) << 16) |
        (static_cast<uint32_t>(buffer[index + 2]) << 8) |
        static_cast<uint32_t>(buffer[index + 3])
      );
      index += 4;
      return value;
    }
  }


  VescDriver::VescDriver(HardwareSerialIMXRT & vescSerial) : serial(vescSerial) {
    serial.addMemoryForRead(serialMemory, sizeof(serialMemory)); // Telemetry frames are larger than the default RX buffer
    serial.begin(IOConstants::vescBaudrate);
  }


  void VescDriver::update(float erpm, float brake) {
    // Never send NaN or infinite values to the VESC
    erpm = std::isfinite(erpm) ? erpm : 0.0f;
    brake = std::isfinite(brake) ? brake : 0.0f;

    targetERPM = std::clamp(erpm, -ControlConstants::vescMaxReverseERPM, ControlConstants::vescMaxERPM);
    targetBrake = std::clamp(brake, 0.0f, ControlConstants::vescMaxBrake);

    if (targetBrake > 0.0f) {
      // Brake and ERPM are mutually exclusive commands, so braking takes priority
      targetERPM = 0.0f;
      sendCommand(commSetCurrentBrake, static_cast<int32_t>(targetBrake * 1'000.0f));
    } else if (targetERPM != 0.0f) {
      sendCommand(commSetRPM, static_cast<int32_t>(targetERPM));
    } else {
      // Zero current lets the motor coast instead of holding a zero ERPM setpoint
      sendCommand(commSetCurrent, 0);
    }
  }


  void VescDriver::setERPM(float erpm) {
    update(erpm, 0.0f);
  }


  void VescDriver::setBrakeCurrent(float brake) {
    update(0.0f, brake);
  }


  void VescDriver::requestTelemetry() {
    // The VESC only sends data when asked, so drop any partial frame to resync on the response
    frameIndex = 0;

    sendPacket(&commGetValues, 1);
  }


  bool VescDriver::readTelemetry() {
    bool received = false;

    while (serial.available() > 0) {
      received |= parseByte(static_cast<uint8_t>(serial.read()));
    }

    return received;
  }


  vescTelemetry_t VescDriver::getTelemetry() const {
    return telemetry;
  }


  float VescDriver::getTargetERPM() const {
    return targetERPM;
  }


  float VescDriver::getTargetBrake() const {
    return targetBrake;
  }


  bool VescDriver::isConnected() const {
    return telemetry.lastUpdate != 0 &&
      pdTICKS_TO_MS(xTaskGetTickCount()) - telemetry.lastUpdate <= telemetryTimeout;
  }


  const char * VescDriver::faultToString(VescFault fault) {
    static const char * const faultNames[] = {
      "FAULT_CODE_NONE",
      "FAULT_CODE_OVER_VOLTAGE",
      "FAULT_CODE_UNDER_VOLTAGE",
      "FAULT_CODE_DRV",
      "FAULT_CODE_ABS_OVER_CURRENT",
      "FAULT_CODE_OVER_TEMP_FET",
      "FAULT_CODE_OVER_TEMP_MOTOR",
      "FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE",
      "FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE",
      "FAULT_CODE_MCU_UNDER_VOLTAGE",
      "FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET",
      "FAULT_CODE_ENCODER_SPI",
      "FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE",
      "FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE",
      "FAULT_CODE_FLASH_CORRUPTION",
      "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1",
      "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2",
      "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3",
      "FAULT_CODE_UNBALANCED_CURRENTS",
      "FAULT_CODE_BRK",
      "FAULT_CODE_RESOLVER_LOT",
      "FAULT_CODE_RESOLVER_DOS",
      "FAULT_CODE_RESOLVER_LOS",
      "FAULT_CODE_FLASH_CORRUPTION_APP_CFG",
      "FAULT_CODE_FLASH_CORRUPTION_MC_CFG",
      "FAULT_CODE_ENCODER_NO_MAGNET",
      "FAULT_CODE_ENCODER_MAGNET_TOO_STRONG",
      "FAULT_CODE_PHASE_FILTER"
    };

    if (fault >= sizeof(faultNames) / sizeof(faultNames[0])) {
      return "NO_KNOWN_FAULTS";
    }

    return faultNames[fault];
  }


  void VescDriver::sendPacket(const uint8_t * payload, uint8_t length) {
    uint8_t packet[10]; // Start, length, 5 byte payload, CRC, and end
    const uint16_t crc = crc16(payload, length);

    packet[0] = startByte;
    packet[1] = length;
    memcpy(&packet[2], payload, length);
    packet[length + 2] = static_cast<uint8_t>(crc >> 8);
    packet[length + 3] = static_cast<uint8_t>(crc & 0xFF);
    packet[length + 4] = endByte;

    // Packets fit in the UART TX buffer, so this doesn't block
    serial.write(packet, length + 5);
  }


  void VescDriver::sendCommand(uint8_t command, int32_t value) {
    const uint8_t payload[5] = {
      command,
      static_cast<uint8_t>(value >> 24),
      static_cast<uint8_t>(value >> 16),
      static_cast<uint8_t>(value >> 8),
      static_cast<uint8_t>(value)
    };

    sendPacket(payload, sizeof(payload));
  }


  bool VescDriver::parseByte(uint8_t byte) {
    // Wait for the start of a frame
    if (frameIndex == 0 && byte != startByte) {
      return false;
    }

    frame[frameIndex++] = byte;

    if (frameIndex < 2) {
      return false;
    }

    const uint8_t length = frame[1];

    // Drop empty frames and wait for the rest of the frame
    if (length == 0) {
      frameIndex = 0;
      return false;
    } else if (frameIndex < length + 5) {
      return false;
    }

    frameIndex = 0; // The next byte starts a new frame

    const uint16_t crc = (frame[length + 2] << 8) | frame[length + 3];

    if (frame[length + 4] != endByte || crc != crc16(&frame[2], length)) {
      return false;
    }

    return parsePayload(&frame[2], length);
  }


  bool VescDriver::parsePayload(const uint8_t * payload, uint8_t length) {
    if (payload[0] != commGetValues || length < valuesLength) {
      return false;
    }

    // Field order follows COMM_GET_VALUES in the VESC firmware (commands.c)
    size_t index = 1;
    telemetry.tempMosfet = readInt16(payload, index) / 10.0f;
    telemetry.tempMotor = readInt16(payload, index) / 10.0f;
    telemetry.motorCurrent = readInt32(payload, index) / 100.0f;
    telemetry.inputCurrent = readInt32(payload, index) / 100.0f;
    index += 8; // Skip the average D and Q axis currents
    telemetry.dutyCycle = readInt16(payload, index) / 1'000.0f;
    telemetry.erpm = readInt32(payload, index);
    telemetry.inputVoltage = readInt16(payload, index) / 10.0f;
    index += 24; // Skip the amp hour, watt hour, and tachometer values
    telemetry.fault = static_cast<VescFault>(payload[index]);

    telemetry.lastUpdate = pdTICKS_TO_MS(xTaskGetTickCount());

    return true;
  }


  uint16_t VescDriver::crc16(const uint8_t * data, uint8_t length) {
    uint16_t crc = 0;

    for (uint8_t i = 0; i < length; i++) {
      crc ^= static_cast<uint16_t>(data[i]) << 8;

      for (uint8_t bit = 0; bit < 8; bit++) {
        crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
      }
    }

    return crc;
  }
}
