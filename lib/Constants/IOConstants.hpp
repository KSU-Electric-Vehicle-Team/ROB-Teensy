#ifndef IO_CONSTANTS
#define IO_CONSTANTS

/*-----------------------------------------------------------------------------*/
/** 
 * @file   IOConstants.hpp
 * @brief  Header for IOConstants struct 
 * 
 * The IOConstants struct is used for the definition of various constants 
 * that are used for things like pin definitions and UART baudrates.
 * 
 * @author Lilia Turbeville
 * @date   March 18, 2026
*//*---------------------------------------------------------------------------*/

#include "arduino_freertos.h"

namespace Constants {
  /**
   * @brief Constants used to interface input and output 
   */
  struct IOConstants {
    static constexpr HardwareSerial & sBusSerial = Serial7;   // UART used for the RC receiver
    static constexpr HardwareSerial & oDriveSerial = Serial6; // UART used for the ODrive
    static constexpr HardwareSerial & vescSerial = Serial2;   // UART used for the VESC
    
    static const size_t oDriveBaudrate = 115'200;
    static const size_t vescBaudrate = 115'200;

    static constexpr double updateFrequency = 80.0;
    static constexpr double ledBlinkFrequency = 1.0;
    static constexpr double telemetryFrequency = 8.0;

    static constexpr uint16_t driveEncoderPulsesPerRev = 600;
    
    static const uint8_t ledBuiltIn = 13; // Built in LED pin

    static const uint8_t oDriveRelay = 3;
    static const uint8_t eBrakeRelay = 4;
    static const uint8_t vescRelay = 5;

    static const uint8_t yellowLedRelay = 21;
    static const uint8_t greenLedRelay = 22;
    static const uint8_t redLedRelay = 23;

    static const uint8_t fanPin = 29; 

    static const uint8_t dynamicBrakePulsePin = 31;
    static const uint8_t dynamicBrakeDirPin = 32;
    static const uint8_t dynamicBrakeLimitSwitchPin = 33;

    static constexpr uint8_t driveEncoderPinA = 1; // E38S6G5 channel A
    static constexpr uint8_t driveEncoderPinB = 0; // E38S6G5 channel B
    
    static const bool telemetryToSerial = false;
    static const bool motorDataToSerial = false;
  };
}

#endif // IO_CONSTANTS