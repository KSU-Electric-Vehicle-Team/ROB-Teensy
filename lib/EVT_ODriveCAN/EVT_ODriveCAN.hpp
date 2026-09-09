#ifndef ODRIVE_CAN_EVT
#define ODRIVE_CAN_EVT


/*-----------------------------------------------------------------------------*/
/** 
 * @file   EVT_ODriveCAN.hpp
 * @brief  Header for ODriver struct 
 * 
 * The ODriver class is used to send and receive messages to ODrives over 
 * CAN bus on the Teensy 4.1
 * 
 * @author Lilia Turbeville
 * @date   March 18, 2026
*//*---------------------------------------------------------------------------*/

#include <EVT_SlewRateLimiter.hpp>

#include "IOConstants.hpp"
#include "ControlConstants.hpp"
#include "ConversionConstants.hpp"
using namespace Constants;

#include "Mutexes.hpp"
using RTOS::MutexValues;

#include "ODriveFlexCAN.hpp"
#include <FlexCAN_T4.h>
#include <vector>

/**
 * @brief Namespace used to control motors
 */
namespace MotorControls {
  /**
   * @brief Struct used to subvert Teensy compile errors
   */
  struct ODriveStatus;

  
  /**
   * @brief Struct used to store important ODrive related data
   */
  struct ODriveUserData {
    Heartbeat_msg_t lastHeartbeat;                    ///< Last received heartbeat message
    bool receivedHeartbeat = false;                   ///< Heartbeat flag

    Get_Encoder_Estimates_msg_t lastFeedback;         ///< Last received encoder estimates 
    bool receivedFeedback = false;                    ///< Feedback flag

    Get_Error_msg_t lastError;                        ///< Last received error message
    bool receivedError = false;                       ///< Error message flag
    
    Get_Bus_Voltage_Current_msg_t lastVoltageCurrent; ///< Last received voltage and current values
    bool receivedVI = false;                          ///< VI message flag

    Get_Temperature_msg_t lastTemperature;            ///< Last received temperature value
    bool receivedTemp = false;                        ///< Temperature message flag
  };


  /**
   * @brief Class used to control ODrives over CAN bus
   */
  class ODriver {
    private:
      ODriveCAN & oDrive;      // ODrive reference  
      ODriveUserData nodeData; // ODrive data value

      float targetPosition;    // ODrive target position 
      float absCenterPos;      // ODrive center position
      bool calibrationFlag;    // Configuration flag

      static char oDrivePrintBuffer[64]; // Character buffer for data output
      static char errorBuffer[64];       // Character buffer for error messages 

      static constexpr double centeringTime = 10.0; // Time in seconds to center motors during configuration

      SlewRateLimiter inputLimiter {ControlConstants::oDriveInputLimit}; 

      uint8_t nodeID; // ODrive CAN ID

      static std::map<uint8_t, ODriveCAN*> oDrives; // ODrives map
    public:
      /**
       * @brief Define a new ODriver instance
       * 
       * @param odrv ODriveCAN reference
       * @param node CAN ID of the ODrive
       */
      ODriver(ODriveCAN & odrv, uint8_t node);


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
       * @brief Set the input mode of the ODrive
       * 
       * @param mode ODriveInputMode enum 
       */
      void setInputMode(ODriveInputMode mode);


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
      float getTemperature();


      /**
       * @brief Get the errors from the ODrive as an enum 
       * 
       * @return ODriveError enum
       */
      ODriveError getDisarmReason() const;


      /**
       * @brief Gets the ODrive node as a map
       * 
       * @return std::map<uint8_t, ODriveCAN*> 
       */
      static std::map<uint8_t, ODriveCAN*> getODrives();


      /**
       * @brief Callback method to run on heartbeat 
       * 
       * @param msg Heartbeat message
       * @param userData ODriveUserData struct
       */
      static void onHeartbeat(Heartbeat_msg_t & msg, void * userData);

      
      /**
       * @brief Callback method to run on encoder feedback
       * 
       * @param msg Encoder feedback message
       * @param userData ODriveUserData struct
       */
      static void onFeedback(Get_Encoder_Estimates_msg_t & msg, void * userData);


      /**
       * @brief Callback method to run on errors
       * 
       * @param err Error message
       * @param userData ODriveUserData struct
       */
      static void onError(Get_Error_msg_t & err, void * userData);


      /**
       * @brief Callback used to run on voltage and current feedback
       * 
       * @param VI Voltage and Current message
       * @param userData ODriveUserData struct
       */
      static void onVoltageCurrent(Get_Bus_Voltage_Current_msg_t & VI, void * userData);


      /**
       * @brief Callback used to run on temperature feedback
       * 
       * @param temp Temperature message 
       * @param userData ODriveUserData struct
       */
      static void onTemperature(Get_Temperature_msg_t & temp, void * userData);
  };

  char ODriver::oDrivePrintBuffer[64];
  char ODriver::errorBuffer[64];
  constexpr double ODriver::centeringTime; 

  std::map<uint8_t, ODriveCAN*> ODriver::oDrives;


  /**
   * @brief Callback method to use to handle CAN communication 
   * 
   * @param msg CAN message for the ODrives
   */
  void onCanMessage(const CAN_message_t & msg) {
    for (auto node : ODriver::getODrives()) {
      onReceive(msg, *node.second);
    }
  }
  
  
  /**
   * @brief Method used to setup CAN communications
   */
  bool setupCan() {
    MutexValues::canBus.begin();
    MutexValues::canBus.setBaudRate(IOConstants::canBaudrate);
    MutexValues::canBus.setMaxMB(16);
    MutexValues::canBus.enableFIFO();
    MutexValues::canBus.enableFIFOInterrupt();
    MutexValues::canBus.onReceive(onCanMessage);

    return true;
  }
}

#endif // ODRIVE_CAN_EVT