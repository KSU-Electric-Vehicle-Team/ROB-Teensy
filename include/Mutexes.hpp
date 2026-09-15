#ifndef MUTEXES
#define MUTEXES

/*-----------------------------------------------------------------------------*/
/**
 * @file   Mutexes.hpp
 * @brief  Header for Mutexes struct
 *
 * The Mutexes struct is used for the definition of RTOS Mutexes used for
 * making tasks thread safe throughout the code
 *
 * @author Lilia Turbeville
 * @date   June 16, 2026
*//*---------------------------------------------------------------------------*/

#include <EVT_StateMachine.hpp>
#include <EVT_RC.hpp>

#include <FlexCAN_T4.h>
#include "timers.h"

namespace RTOS {
  /**
   * @brief A collection of mutexes used through the various tasks
   */
  struct Mutexes {
    static SemaphoreHandle_t telemetryMutex;
    static SemaphoreHandle_t commandMutex;
    static SemaphoreHandle_t canMutex;
    static SemaphoreHandle_t rcMutex;

    static SemaphoreHandle_t errorSemaphore;

    static TimerHandle_t blinkTimer;
  };


  /**
   * @brief Struct used to format Ethernet packets
   */
  struct telemetry_t {
    char buffer[128];       ///< Message buffer for the Ethernet connection
    char state[8];          ///< Current state as a string

    int32_t encoderCount;   ///< Encoder tick count since boot

    float driveRevolutions; ///< Number of turns of the drive motor
    float steeringAngle;    ///< Current steering angle
    float oDriveTarget;     ///< ODrive target angle
    float rpm;              ///< Drive motor RPM target

    uint16_t throttle;      ///< RC throttle input
    uint16_t steering;      ///< RC steering input

    uint8_t steeringTemp;   ///< Steering ODrive temperature in degrees celcius

    /**
     * @brief Function used to format the buffer field of the packet_t struct
     */
    void format() {
      snprintf(
        this->buffer,
        sizeof(this->buffer),
        "%s,%0.2f,%0.2f,%0.2f,%u,%u,%0.6f,%ld, %u",
        this->state,
        this->rpm,
        this->steeringAngle,
        this->oDriveTarget,
        this->throttle,
        this->steering,
        this->driveRevolutions,
        this->encoderCount,
        this->steeringTemp
      );
    }
  };


  /**
   * @brief Struct used to format UDP autonomous commands
   */
  struct command_t {
    char stateString[8];    ///< Current state of the system as a string

    float steering;         ///< Steering target in turns
    float brake;            ///< Drive brake current in amps
    float erpm;             ///< Drive ERPM

    bool isSteeringLimited; ///< Rate limit flag for the steering motor
    bool isDriveLimited;    ///< Rate limit flag for the drive motor

    bool autonomous;        ///< Autonomous Flag
    bool emergency;         ///< Emergency flag
  };


  /**
   * @brief Struct used for creation and storage of joystick values
   */
  struct joystick_t {
    uint16_t x; ///< X axis value
    uint16_t y; ///< Y axis value

    /**
     * @brief Applies a deadband to the joystick values
     *
     * @return joystick_t of either {midRC, midRC} or {x, y}
     */
    joystick_t deadband() {
      if ((x > TransmitterConstants::deadbandBounds[0] && x < TransmitterConstants::deadbandBounds[1]) && (y > TransmitterConstants::deadbandBounds[0] && y < TransmitterConstants::deadbandBounds[1])) {
        return {TransmitterConstants::midRC, TransmitterConstants::midRC};
      } else {
        return {x, y};
      }
    }
  };


  /**
   * @brief Struct used for storing values from the RC transmitter
   */
  struct transmitter_t {
    joystick_t leftJoystick;
    joystick_t rightJoystick;

    uint16_t swa, swb, swc, swd, swe, swf, swg, swh;
    uint16_t vra, vrb, vrc, vrd;

    /**
     * @brief Updates the values of the struct
     *
     * @param transmitter ControlRC instance to use to update the values
     * @param isMapped Condition to determine whether to map the channels or not (Default false)
     */
    void update(Signals::ControlRC * transmitter, bool isMapped = false) {
      leftJoystick.x = transmitter->getChannelValue(Signals::ChannelRC::LEFT_X, isMapped);
      leftJoystick.y = transmitter->getChannelValue(Signals::ChannelRC::LEFT_Y, isMapped);

      rightJoystick.x = transmitter->getChannelValue(Signals::ChannelRC::RIGHT_X, isMapped);
      rightJoystick.y = transmitter->getChannelValue(Signals::ChannelRC::RIGHT_Y, isMapped);

      swa = transmitter->getChannelValue(Signals::ChannelRC::SWA, isMapped);
      swb = transmitter->getChannelValue(Signals::ChannelRC::SWB, isMapped);
      swc = transmitter->getChannelValue(Signals::ChannelRC::SWC, isMapped);
      swd = transmitter->getChannelValue(Signals::ChannelRC::SWD, isMapped);
      swe = transmitter->getChannelValue(Signals::ChannelRC::SWE, isMapped);
      swf = transmitter->getChannelValue(Signals::ChannelRC::SWF, isMapped);
      swg = transmitter->getChannelValue(Signals::ChannelRC::SWG, isMapped);
      swh = transmitter->getChannelValue(Signals::ChannelRC::SWH, isMapped);

      vra = transmitter->getChannelValue(Signals::ChannelRC::VRA, isMapped);
      vra = transmitter->getChannelValue(Signals::ChannelRC::VRB, isMapped);
      vra = transmitter->getChannelValue(Signals::ChannelRC::VRC, isMapped);
      vra = transmitter->getChannelValue(Signals::ChannelRC::VRD, isMapped);
    }
  };


  /**
   * @brief Values used through various tasks to be protected by mutex
   */
  struct MutexValues {
    static FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> canBus;

    static transmitter_t transmitterValues;
    static telemetry_t pandaPacket;
    static command_t commands;

    static float steeringCenter;

    static bool canSetupFlag;
    static bool oDriveSetupFlag;

    static bool ledState;
  };


  SemaphoreHandle_t Mutexes::telemetryMutex = xSemaphoreCreateMutex();  ///< Mutex for Ethernet telemetry values
  SemaphoreHandle_t Mutexes::commandMutex = xSemaphoreCreateMutex();    ///< Mutex for motor commands
  SemaphoreHandle_t Mutexes::canMutex = xSemaphoreCreateMutex();        ///< Mutex for CAN communications
  SemaphoreHandle_t Mutexes::rcMutex = xSemaphoreCreateMutex();         ///< Mutex for RC values

  SemaphoreHandle_t Mutexes::errorSemaphore = xSemaphoreCreateBinary(); ///< Binary semaphore for error handling

  TimerHandle_t Mutexes::blinkTimer;                                    ///< Timer used to blink the onboard LED

  FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> MutexValues::canBus;        ///< CAN bus instance

  // Structs for value packets
  transmitter_t MutexValues::transmitterValues; ///< Values from the RC transmitter
  telemetry_t MutexValues::pandaPacket;         ///< Values to send to the Panda packet
  command_t MutexValues::commands;              ///< Values for the autonomous commands

  // Other mutex values
  float MutexValues::steeringCenter;            ///< Value of the steering center position in turns

  bool MutexValues::canSetupFlag = false;       ///< Flag to denote whether or not CAN bus has been setup
  bool MutexValues::oDriveSetupFlag = false;    ///< Flag to denote whether or not ODrive CAN has been setup

  bool MutexValues::ledState = false;
}

#endif // MUTEXES
