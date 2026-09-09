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
    static constexpr HardwareSerial & sBusSerial = Serial7;   ///< UART used for the RC receiver
    static constexpr HardwareSerial & oDriveSerial = Serial6; ///< UART used for the ODrive
    static constexpr HardwareSerial & vescSerial = Serial2;   ///< UART used for the VESC
    
    static const size_t canBaudrate = 250'000;    ///< CAN bus baudrate in bits per second

    static const size_t oDriveBaudrate = 115'200; ///< ODrive serial baudrate in bits per second
    static const size_t vescBaudrate = 115'200;   ///< VESC serial baudrate in bits per second

    static constexpr double serialPrintFrequency = 5.0; ///< Times per second to output to Serial Monitor 
    static constexpr double telemetryFrequency = 8.0;   ///< Times per second to send telemetry to high level 
    static constexpr double ledBlinkFrequency = 1.0;    ///< Times per second to blink the LED on the Teensy 
    static constexpr double updateFrequency = 80.0;     ///< Times per second to update the RC control values 

    static const uint8_t steeringMotorCanID = 0; ///< CAN ID for the steering motor ODrive 
    
    static const uint8_t ledBuiltIn = 13; ///< Built in LED pin
  };
}

#endif // IO_CONSTANTS