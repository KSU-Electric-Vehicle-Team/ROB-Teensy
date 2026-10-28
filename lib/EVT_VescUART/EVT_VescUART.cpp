#include "EVT_VescUART.hpp"

#include <algorithm>
#include <cmath>

namespace MotorControls {
  VescDriver::VescDriver(HardwareSerial & vescSerial) : serial(vescSerial) {
    serial.begin(IOConstants::vescBaudrate); // Start the UART at the VESC baudrate
    vesc.setSerialPort(&serial);             // Use the UART for VescUart packets
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
      vesc.setBrakeCurrent(targetBrake);
    } else if (std::fabs(targetERPM) >= ControlConstants::vescMinERPM) {
      vesc.setRPM(targetERPM);
    } else {
      // The VESC brakes at zero duty on speed setpoints below its minimum ERPM, so coast with zero current instead
      targetERPM = 0.0f;
      vesc.setCurrent(0.0f);
    }
  }


  void VescDriver::setERPM(float erpm) {
    update(erpm, 0.0f);
  }


  void VescDriver::setBrakeCurrent(float brake) {
    update(0.0f, brake);
  }


  bool VescDriver::updateTelemetry() {
    // Drop stale bytes so VescUart only reads the reply to this request
    while (serial.available() > 0) {
      serial.read();
    }

    if (!vesc.getVescValues()) {
      return false;
    }

    telemetry.tempMosfet = vesc.data.tempMosfet;
    telemetry.tempMotor = vesc.data.tempMotor;
    telemetry.motorCurrent = vesc.data.avgMotorCurrent;
    telemetry.inputCurrent = vesc.data.avgInputCurrent;
    telemetry.dutyCycle = vesc.data.dutyCycleNow;
    telemetry.erpm = vesc.data.rpm;
    telemetry.inputVoltage = vesc.data.inpVoltage;
    telemetry.fault = vesc.data.error;

    telemetry.lastUpdate = pdTICKS_TO_MS(xTaskGetTickCount());

    return true;
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


  const char * VescDriver::faultToString(mc_fault_code fault) {
    // Switch on the raw value so newer firmware codes that VescUart's enum doesn't name can still be matched
    switch (static_cast<uint8_t>(fault)) {
      case FAULT_CODE_NONE:
        return "FAULT_CODE_NONE";
      case FAULT_CODE_OVER_VOLTAGE:
        return "FAULT_CODE_OVER_VOLTAGE";
      case FAULT_CODE_UNDER_VOLTAGE:
        return "FAULT_CODE_UNDER_VOLTAGE";
      case FAULT_CODE_DRV:
        return "FAULT_CODE_DRV";
      case FAULT_CODE_ABS_OVER_CURRENT:
        return "FAULT_CODE_ABS_OVER_CURRENT";
      case FAULT_CODE_OVER_TEMP_FET:
        return "FAULT_CODE_OVER_TEMP_FET";
      case FAULT_CODE_OVER_TEMP_MOTOR:
        return "FAULT_CODE_OVER_TEMP_MOTOR";
      case FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE:
        return "FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE";
      case FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE:
        return "FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE";
      case FAULT_CODE_MCU_UNDER_VOLTAGE:
        return "FAULT_CODE_MCU_UNDER_VOLTAGE";
      case FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET:
        return "FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET";
      case FAULT_CODE_ENCODER_SPI:
        return "FAULT_CODE_ENCODER_SPI";
      case FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE:
        return "FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE";
      case FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE:
        return "FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE";
      case FAULT_CODE_FLASH_CORRUPTION:
        return "FAULT_CODE_FLASH_CORRUPTION";
      case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1:
        return "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1";
      case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2:
        return "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2";
      case FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3:
        return "FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3";
      case FAULT_CODE_UNBALANCED_CURRENTS:
        return "FAULT_CODE_UNBALANCED_CURRENTS";
      case FAULT_CODE_BRK:
        return "FAULT_CODE_BRK";
      case FAULT_CODE_RESOLVER_LOT:
        return "FAULT_CODE_RESOLVER_LOT";
      case FAULT_CODE_RESOLVER_DOS:
        return "FAULT_CODE_RESOLVER_DOS";
      case FAULT_CODE_RESOLVER_LOS:
        return "FAULT_CODE_RESOLVER_LOS";
      case FAULT_CODE_FLASH_CORRUPTION_APP_CFG:
        return "FAULT_CODE_FLASH_CORRUPTION_APP_CFG";
      case FAULT_CODE_FLASH_CORRUPTION_MC_CFG:
        return "FAULT_CODE_FLASH_CORRUPTION_MC_CFG";
      case FAULT_CODE_ENCODER_NO_MAGNET:
        return "FAULT_CODE_ENCODER_NO_MAGNET";
      case FAULT_CODE_ENCODER_MAGNET_TOO_STRONG:
        return "FAULT_CODE_ENCODER_MAGNET_TOO_STRONG";
      case FAULT_CODE_PHASE_FILTER:
        return "FAULT_CODE_PHASE_FILTER";
      case 28:
        return "FAULT_CODE_ENCODER_FAULT";
      case 29:
        return "FAULT_CODE_LV_OUTPUT_FAULT";
      case 30:
        return "FAULT_CODE_ENCODER_SLIP";
      case 31:
        return "FAULT_CODE_OVERSPEED";
      case 32:
        return "FAULT_CODE_UNDERSPEED";
      case 33:
        return "FAULT_CODE_ABS_OVERSPEED";
      default:
        return "FAULT_CODE_UNKNOWN";
    }
  }
}
