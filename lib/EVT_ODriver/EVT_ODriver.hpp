#ifndef EVT_ODRIVER_UART
#define EVT_ODRIVER_UART

/*-----------------------------------------------------------------------------*/
/**
 * @file   EVT_ODriver.hpp
 * @brief  Header for ODriver class
 *
 * The ODriver class is used to send and receive messages to ODrives over
 * UART on the Teensy 4.1
 *
 * @author Austin Sheppard
 * @date   March 18, 2026
*//*---------------------------------------------------------------------------*/

#include <EVT_SlewRateLimiter.hpp>

#include "IOConstants.hpp"
#include "ControlConstants.hpp"
#include "ConversionConstants.hpp"
using namespace Constants;

#include "Mutexes.hpp"
using RTOS::MutexValues;

#include <ODriveUART.h>


/**
 * @brief Namespace used to control motors and related values
 */
namespace MotorControls {
  /**
   * @brief Struct used to store important ODrive related data
   */
  struct ODriveUserData {
    ODriveFeedback lastFeedback; ///< Last received encoder estimates
    ODriveError lastError;       ///< Last received error message

    float lastVoltage;           ///< Last received voltage value in volts
    float lastCurrent;           ///< Last received current value in amps

    float lastTemperature;       ///< Last received temperature value in celcius
  };


  /**
   * @brief Class used to control ODrives over UART
   */
  class ODriver {
    private:
      ODriveUART & oDrive;  // ODriveUART reference
      ODriveUserData data;  // ODrive data values

      float targetPosition; // ODrive target position
      float absCenterPos;   // ODrive center position
      bool calibrationFlag; // Configuration flag

      static char oDrivePrintBuffer[64]; // Character buffer for data output
      static char errorBuffer[64];       // Character buffer for error messages

      static constexpr double centeringTime = 10.0; // Time in seconds to center motors during configuration

      SlewRateLimiter inputLimiter {ControlConstants::oDriveInputLimit};
    public:
      /**
       * @brief Define a new ODriver instance
       *
       * @param odrv ODriveUART reference to use
       */
      ODriver(ODriveUART & odrv);


      /**
       * @brief Checks if ODrive CAN communication has been defined
       *
       * @return ODrive CAN communication state as a boolean
       */
      bool setup();


      /**
       * @brief Runs the ODrive calibration sequence
       *
       * @return The result of the ODrive calibration sequence
       */
      bool calibrate();


      /**
       * @brief Updates the ODrive control values
       *
       * @param targetValue Target position in turns
       * @param rateLimit Flag to denote whether or not to rate limit the ODrive input (Default false)
       */
      void update(float targetValue, bool rateLimit = false);


      /**
       * @brief Reset the ODrive and clear errors
       */
      void reset();


      /**
       * @brief Block the task until the ODrive reaches a specific state
       *
       * @param state State to wait for
       * @param timeout Maximum allowed time to wait
       */
      void waitUntilState(ODriveAxisState state, TickType_t timeout = portMAX_DELAY);


      /**
       * @brief Get the value of the ODrive calibration flag
       *
       * @return The value denoting whether or not the ODrive is calibrated
       */
      bool getCalibrationFlag() const;


      /**
       * @brief Get the absolute center position of the ODrive in turns
       *
       * @return The absolute center position of the ODrive
       */
      float getCenterPosition() const;


      /**
       * @brief Get the temperature of the ODrive in degrees celcius
       *
       * @return The temperature of the ODrive in degrees celcius
       */
      float getTemperature() const;


      /**
       * @brief Get the errors from the ODrive as an enum
       *
       * @return ODriveError enum
       */
      ODriveError getDisarmReason();
  };

  char ODriver::oDrivePrintBuffer[64];
  char ODriver::errorBuffer[64];
  constexpr double ODriver::centeringTime;
}

#endif // EVT_ODRIVER_UART
