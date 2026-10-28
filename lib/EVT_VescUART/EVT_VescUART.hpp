#ifndef EVT_VESC_UART
#define EVT_VESC_UART

/*-----------------------------------------------------------------------------*/
/**
 * @file   EVT_VescUART.hpp
 * @brief  Header for VescDriver class
 *
 * The VescDriver class is used to control a VESC motor controller over UART
 * using the VescUart library. It sends ERPM and brake current commands within
 * the limits in ControlConstants, and reads telemetry such as the MOSFET and
 * motor temperatures so that other RTOS tasks can use it.
 *
 * @author FluenceCodez
 * @date   October 28, 2026
*//*---------------------------------------------------------------------------*/

#include "arduino_freertos.h"
#include <VescUart.h>

#include <ControlConstants.hpp>
#include <IOConstants.hpp>
using namespace Constants;

/**
 * @brief Namespace used to control motors
 */
namespace MotorControls {
  /**
   * @brief Struct used to store telemetry values received from the VESC
   */
  typedef struct {
    float tempMosfet;    ///< MOSFET temperature in degrees Celsius
    float tempMotor;     ///< Motor temperature in degrees Celsius
    float motorCurrent;  ///< Average motor current in amps
    float inputCurrent;  ///< Average input current in amps
    float dutyCycle;     ///< Duty cycle from -1.0 to 1.0
    float erpm;          ///< Measured electrical RPM
    float inputVoltage;  ///< Input voltage in volts

    mc_fault_code fault; ///< Latest fault code reported by the VESC

    size_t lastUpdate;   ///< Time in milliseconds when the values were received (0 if never received)
  } vescTelemetry_t;


  /**
   * @brief Class used to control a VESC over UART
   */
  class VescDriver {
    private:
      static const uint32_t replyTimeout = 20UL;    // Maximum time in milliseconds to wait for a telemetry reply
      static const size_t telemetryTimeout = 500UL; // Maximum time in milliseconds between replies before the VESC counts as disconnected

      HardwareSerial & serial;                      // UART connected to the VESC
      VescUart vesc {replyTimeout};                 // VescUart instance used to build and parse VESC packets

      vescTelemetry_t telemetry {};                 // Latest telemetry received from the VESC

      float targetERPM = 0.0f;                      // Last ERPM command sent to the VESC
      float targetBrake = 0.0f;                     // Last brake current command sent to the VESC
    public:
      /**
       * @brief Define a new VescDriver instance and start its UART
       *
       * @param vescSerial UART connected to the VESC
       */
      VescDriver(HardwareSerial & vescSerial);


      /**
       * @brief Sends an ERPM or brake current command to the VESC
       *
       * @note Brake current takes priority over ERPM. ERPM below ControlConstants::vescMinERPM with no
       *       brake makes the motor coast. Commands must be sent faster than the VESC timeout or the
       *       VESC releases the motor.
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
       * @brief Requests telemetry from the VESC and stores it if a valid reply arrives
       *
       * @note VescUart busy-waits for the reply without yielding, so this keeps the CPU for up to 20 ms.
       *       Higher priority tasks can still preempt it, but lower priority tasks can't run until it returns.
       *
       * @return Whether new telemetry values were received
       */
      bool updateTelemetry();


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
       * @brief Checks if the VESC has sent valid telemetry in the last 500 ms
       *
       * @return Whether the VESC is responding
       */
      bool isConnected() const;


      /**
       * @brief Converts a VESC fault code to a string
       *
       * @param fault Fault code to convert
       * @return Name of the fault code, or "FAULT_CODE_UNKNOWN" if the code isn't recognized
       */
      static const char * faultToString(mc_fault_code fault);
  };
}

#endif // EVT_VESC_UART
