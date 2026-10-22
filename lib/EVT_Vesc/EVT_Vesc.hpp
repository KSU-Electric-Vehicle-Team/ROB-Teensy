#ifndef EVT_VESC
#define EVT_VESC

/*-----------------------------------------------------------------------------*/
/**
 * @file   EVT_Vesc.hpp
 * @brief  Header for VescDriver class
 *
 * The VescDriver class is used to control a VESC motor controller over UART
 * with a Teensy 4.1. It sends ERPM and brake current commands, and requests
 * and parses telemetry from the VESC without blocking, so it can be called
 * from an RTOS task without starving other tasks.
 *
 * @author FluenceCodez
 * @date   October 21, 2026
*//*---------------------------------------------------------------------------*/

#include "arduino_freertos.h"

#include <ControlConstants.hpp>
#include <IOConstants.hpp>
using namespace Constants;

namespace MotorControls {
  /**
   * @brief Enum used for the fault codes reported by the VESC
   */
  typedef enum : uint8_t {
    FAULT_CODE_NONE = 0,
    FAULT_CODE_OVER_VOLTAGE,
    FAULT_CODE_UNDER_VOLTAGE,
    FAULT_CODE_DRV,
    FAULT_CODE_ABS_OVER_CURRENT,
    FAULT_CODE_OVER_TEMP_FET,
    FAULT_CODE_OVER_TEMP_MOTOR,
    FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE,
    FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE,
    FAULT_CODE_MCU_UNDER_VOLTAGE,
    FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET,
    FAULT_CODE_ENCODER_SPI,
    FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE,
    FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE,
    FAULT_CODE_FLASH_CORRUPTION,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3,
    FAULT_CODE_UNBALANCED_CURRENTS,
    FAULT_CODE_BRK,
    FAULT_CODE_RESOLVER_LOT,
    FAULT_CODE_RESOLVER_DOS,
    FAULT_CODE_RESOLVER_LOS,
    FAULT_CODE_FLASH_CORRUPTION_APP_CFG,
    FAULT_CODE_FLASH_CORRUPTION_MC_CFG,
    FAULT_CODE_ENCODER_NO_MAGNET,
    FAULT_CODE_ENCODER_MAGNET_TOO_STRONG,
    FAULT_CODE_PHASE_FILTER
  } VescFault;


  /**
   * @brief Struct used to store telemetry received from the VESC
   */
  typedef struct {
    float tempMosfet;   ///< MOSFET temperature in degrees celsius
    float tempMotor;    ///< Motor temperature in degrees celsius
    float motorCurrent; ///< Average motor current in amps
    float inputCurrent; ///< Average input current in amps
    float dutyCycle;    ///< Duty cycle from -1.0 to 1.0
    float erpm;         ///< Measured electrical RPM
    float inputVoltage; ///< Input voltage in volts

    VescFault fault;    ///< Latest fault code

    size_t lastUpdate;  ///< Time in milliseconds when the values were received (0 if never received)
  } vescTelemetry_t;


  /**
   * @brief Class used to control a VESC over UART
   */
  class VescDriver {
    private:
      static const uint8_t maxPayload = 255;              // Largest payload a short VESC frame can hold
      static const size_t telemetryTimeout = 500UL;       // Maximum allowed time between telemetry frames

      HardwareSerialIMXRT & serial;                       // UART connected to the VESC
      vescTelemetry_t telemetry {};                       // Latest telemetry received from the VESC

      float targetERPM = 0.0f;                            // Last ERPM command sent
      float targetBrake = 0.0f;                           // Last brake current command sent

      uint8_t frame[maxPayload + 5];                      // Frame being received (start, length, payload, CRC, end)
      uint16_t frameIndex = 0;                            // Number of frame bytes received so far

      uint8_t serialMemory[256];                          // Extra UART RX memory so a full telemetry frame fits between reads


      /**
       * @brief Frames and sends a payload to the VESC
       *
       * @param payload Payload to send
       * @param length Length of the payload (5 bytes maximum)
       */
      void sendPacket(const uint8_t * payload, uint8_t length);


      /**
       * @brief Sends a command with a single 32 bit argument to the VESC
       *
       * @param command VESC command ID
       * @param value Argument for the command
       */
      void sendCommand(uint8_t command, int32_t value);


      /**
       * @brief Adds a received byte to the current frame
       *
       * @param byte Byte received from the VESC
       * @return Whether the byte completed a valid telemetry frame
       */
      bool parseByte(uint8_t byte);


      /**
       * @brief Parses the payload of a valid frame into the telemetry values
       *
       * @param payload Payload of the frame
       * @param length Length of the payload
       * @return Whether the payload contained telemetry values
       */
      bool parsePayload(const uint8_t * payload, uint8_t length);


      /**
       * @brief Calculates the CRC16 (XMODEM) checksum used by the VESC
       *
       * @param data Data to calculate the checksum of
       * @param length Length of the data
       * @return Checksum of the data
       */
      static uint16_t crc16(const uint8_t * data, uint8_t length);
    public:
      /**
       * @brief Define a new VescDriver instance and start its UART
       *
       * @param vescSerial UART connected to the VESC
       */
      VescDriver(HardwareSerialIMXRT & vescSerial);


      /**
       * @brief Sends an ERPM or brake current command to the VESC
       *
       * @note Brake current takes priority over ERPM. When both are zero the motor is released.
       *       Commands must be sent faster than the VESC timeout or the VESC releases the motor.
       *
       * @param erpm Target electrical RPM
       * @param brake Target brake current in amps
       */
      void update(float erpm, float brake);


      /**
       * @brief Sets the target ERPM of the VESC
       *
       * @param erpm Target electrical RPM
       */
      void setERPM(float erpm);


      /**
       * @brief Sets the brake current of the VESC
       *
       * @param brake Target brake current in amps
       */
      void setBrakeCurrent(float brake);


      /**
       * @brief Requests a new set of telemetry values from the VESC
       */
      void requestTelemetry();


      /**
       * @brief Reads any telemetry the VESC has sent without blocking
       *
       * @return Whether new telemetry values were received
       */
      bool readTelemetry();


      /**
       * @brief Get the latest telemetry received from the VESC
       *
       * @return Latest VESC telemetry values
       */
      vescTelemetry_t getTelemetry() const;


      /**
       * @brief Get the last ERPM command sent to the VESC after limits are applied
       *
       * @return Last ERPM command
       */
      float getTargetERPM() const;


      /**
       * @brief Get the last brake current command sent to the VESC after limits are applied
       *
       * @return Last brake current command in amps
       */
      float getTargetBrake() const;


      /**
       * @brief Checks if the VESC has sent telemetry recently
       *
       * @return Whether the VESC is responding
       */
      bool isConnected() const;


      /**
       * @brief Converts a VESC fault code to a string
       *
       * @param fault Fault code to convert
       * @return Name of the fault code
       */
      static const char * faultToString(VescFault fault);
  };
}

#endif // EVT_VESC
