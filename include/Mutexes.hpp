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

#include <EVT_RC.hpp>

namespace RTOS {
  /**
   * @brief A collection of mutexes used through the various tasks
   */
  struct Mutexes {
    static SemaphoreHandle_t telemetryMutex;
    static SemaphoreHandle_t rcMutex;
  }; 


  /**
   * @brief Struct used to format Ethernet packets
   */
  typedef struct {
    char buffer[128];       // Message buffer for the Ethernet connection
    char state[8];          // Current state as a string
    
    int32_t encoderCount;   // Encoder tick count since boot

    float driveRevolutions; // Number of turns of the drive motor 
    float steeringAngle;    // Current steering angle
    float oDriveTarget;     // ODrive target angle 
    float rpm;              // Drive motor RPM target

    uint16_t throttle;      // RC throttle input
    uint16_t steering;      // RC steering input

    /**
     * @brief Function used to format the buffer field of the packet_t struct
     */
    void format() {
      snprintf(
        this->buffer,
        sizeof(this->buffer),
        "%s,%0.2f,%0.2f,%0.2f,%u,%u,%0.6f,%ld",
        this->state,
        this->rpm,
        this->steeringAngle,
        this->oDriveTarget,
        this->throttle,
        this->steering,
        this->driveRevolutions,
        this->encoderCount
      );
    }
  } telemetry_t;


  /**
   * @brief Struct used to format UDP autonomous commands
   */
  typedef struct {
    char state[8];

    float steering;
    float brake;
    float erpm;
    
    bool emergency;
  } command_t;


  /**
   * @brief Struct used for creation and storage of joystick values 
   */
  typedef struct {
    uint16_t x; // X axis value
    uint16_t y; // Y axis value
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
    void update(Signals::ControlRC * transmitter) {
      leftJoystick.x = transmitter->getChannelValue(Signals::ChannelRC::LEFT_X, false);
      leftJoystick.y = transmitter->getChannelValue(Signals::ChannelRC::LEFT_Y, false);

      rightJoystick.x = transmitter->getChannelValue(Signals::ChannelRC::RIGHT_X, false);
      rightJoystick.y = transmitter->getChannelValue(Signals::ChannelRC::RIGHT_Y, false);

      swa = transmitter->getChannelValue(Signals::ChannelRC::SWA, false);
      swb = transmitter->getChannelValue(Signals::ChannelRC::SWB, false);
      swc = transmitter->getChannelValue(Signals::ChannelRC::SWC, false);
      swd = transmitter->getChannelValue(Signals::ChannelRC::SWD, false);
      swe = transmitter->getChannelValue(Signals::ChannelRC::SWE, false);
      swf = transmitter->getChannelValue(Signals::ChannelRC::SWF, false);
      swg = transmitter->getChannelValue(Signals::ChannelRC::SWG, false);
      swh = transmitter->getChannelValue(Signals::ChannelRC::SWH, false);

      vra = transmitter->getChannelValue(Signals::ChannelRC::VRA, false);
      vra = transmitter->getChannelValue(Signals::ChannelRC::VRB, false);
      vra = transmitter->getChannelValue(Signals::ChannelRC::VRC, false);
      vra = transmitter->getChannelValue(Signals::ChannelRC::VRD, false);
    }
  } transmitter_t;


  /**
   * @brief Values used through various tasks to be protected by mutex 
   */
  struct MutexValues {
    static transmitter_t transmitterValues; // Values from the RC transmitter
    static telemetry_t pandaPacket;         // Values to send to the Panda packet 
    static command_t autoCommands;          // Values for the autonomous commands
  };


  SemaphoreHandle_t Mutexes::telemetryMutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t Mutexes::rcMutex = xSemaphoreCreateMutex();
  
  transmitter_t MutexValues::transmitterValues;
  telemetry_t MutexValues::pandaPacket;
  command_t MutexValues::autoCommands;
}

#endif // MUTEXES