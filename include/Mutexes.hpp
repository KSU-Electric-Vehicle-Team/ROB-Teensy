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
#include <EVT_Ethernet.hpp>

#include "timers.h"
#include "EthernetPacket.hpp"

namespace RTOS {
  /**
   * @brief A collection of mutexes used through the various tasks
   */
  struct Mutexes {
    static SemaphoreHandle_t telemetryMutex;
    static SemaphoreHandle_t healthMutex;
    static SemaphoreHandle_t commandMutex;
    static SemaphoreHandle_t rcMutex;

    static TimerHandle_t blinkTimer;
  };


  /**
   * @brief Struct used to format Ethernet packets
   */
  typedef struct {
    char buffer[128];         ///< Message buffer for the Ethernet connection
    char state[8];            ///< Current state as a string

    int32_t encoderCount;     ///< Encoder tick count since boot

    int sequence;             ///< Packet sequence
    int lastCommandSequence;  ///< Packet sequence
    size_t sampleTime;        ///< Current time in miliseconds
    size_t commandAge;        ///< Packet age in miliseconds

    float driveRevolutions;   ///< Number of turns of the drive motor
    float steeringAngle;      ///< Current steering angle
    float oDriveTarget;       ///< ODrive target angle
    float rpm;                ///< Drive motor RPM target

    uint16_t throttle;        ///< RC throttle input
    uint16_t steering;        ///< RC steering input

    /**
     * @brief Function used to format the buffer field of the packet_t struct
     */
    void format() {
      snprintf(
        this->buffer,
        sizeof(this->buffer),
        "%s,%0.2f,%0.2f,%0.2f,%u,%u,%0.6f,%ld",
        this->sequence,
        this->sampleTime,
        this->state,
        this->rpm,
        this->steeringAngle,
        this->oDriveTarget,
        this->throttle,
        this->steering,
        this->driveRevolutions,
        this->encoderCount,
        this->commandAge,
        this->lastCommandSequence
      );
    }
  } telemetry_t;


  /**
   * @brief Struct used to format UDP autonomous commands
   */
  typedef struct {
    char stateString[8];

    float steering;
    float brake;
    float erpm;

    bool emergency;
  } command_t;


  /**
   * @brief Struct used for creation and storage of joystick values
   */
  typedef struct {
    uint16_t x; ///< X axis value
    uint16_t y; ///< Y axis value
  } joystick_t;


  /**
   * @brief Struct used for storing values from the RC transmitter
   */
  typedef struct {
    joystick_t leftJoystick;
    joystick_t rightJoystick;

    uint16_t swa, swb, swc, swd, swe, swf, swg, swh;
    uint16_t vra, vrb, vrc, vrd;

    /**
     * @brief Updates the values of the struct
     *
     * @param transmitter ControlRC instance to use to update the values
     */
    void update(Networking::ControlRC * transmitter) {
      leftJoystick.x = transmitter->getChannelValue(Networking::ChannelRC::LEFT_X, false);
      leftJoystick.y = transmitter->getChannelValue(Networking::ChannelRC::LEFT_Y, false);

      rightJoystick.x = transmitter->getChannelValue(Networking::ChannelRC::RIGHT_X, false);
      rightJoystick.y = transmitter->getChannelValue(Networking::ChannelRC::RIGHT_Y, false);

      swa = transmitter->getChannelValue(Networking::ChannelRC::SWA, false);
      swb = transmitter->getChannelValue(Networking::ChannelRC::SWB, false);
      swc = transmitter->getChannelValue(Networking::ChannelRC::SWC, false);
      swd = transmitter->getChannelValue(Networking::ChannelRC::SWD, false);
      swe = transmitter->getChannelValue(Networking::ChannelRC::SWE, false);
      swf = transmitter->getChannelValue(Networking::ChannelRC::SWF, false);
      swg = transmitter->getChannelValue(Networking::ChannelRC::SWG, false);
      swh = transmitter->getChannelValue(Networking::ChannelRC::SWH, false);

      vra = transmitter->getChannelValue(Networking::ChannelRC::VRA, false);
      vra = transmitter->getChannelValue(Networking::ChannelRC::VRB, false);
      vra = transmitter->getChannelValue(Networking::ChannelRC::VRC, false);
      vra = transmitter->getChannelValue(Networking::ChannelRC::VRD, false);
    }
  } transmitter_t;


  /**
   * @brief Values used through various tasks to be protected by mutex
   */
  struct MutexValues {
    static transmitter_t transmitterValues;
    static Networking::TelemetryPayload telemetry;
    static Networking::InfoPayload info;
    static Networking::HealthPayload health;
    static command_t commands;

    static uint32_t bootId;
    static bool ledState;
  };


  SemaphoreHandle_t Mutexes::telemetryMutex = xSemaphoreCreateMutex(); ///< Mutex for Ethernet telemetry values
  SemaphoreHandle_t Mutexes::healthMutex = xSemaphoreCreateMutex();    ///< Mutex for Ethernet health values
  SemaphoreHandle_t Mutexes::commandMutex = xSemaphoreCreateMutex();   ///< Mutex for motor commands
  SemaphoreHandle_t Mutexes::rcMutex = xSemaphoreCreateMutex();        ///< Mutex for RC values

  TimerHandle_t Mutexes::blinkTimer;                                   ///< Timer used to blink the onboard LED

  // Structs for value packets
  transmitter_t MutexValues::transmitterValues; ///< Values from the RC transmitter
  Networking::TelemetryPayload MutexValues::telemetry;         ///< Values to send to the Panda packet
  Networking::InfoPayload MutexValues::info;
  Networking::HealthPayload MutexValues::health;
  command_t MutexValues::commands;              ///< Values for the autonomous commands

  // Other mutex values
  uint32_t MutexValues::bootId;
  bool MutexValues::ledState = false;
}

#endif // MUTEXES
