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

namespace RTOS {
  /**
   * @brief A collection of mutexes used through the various tasks
   */
  struct Mutexes {
    static SemaphoreHandle_t telemetryMutex;
    static SemaphoreHandle_t commandMutex;  
    static SemaphoreHandle_t rcMutex;       

    static SemaphoreHandle_t errorSemaphore;
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
  } transmitter_t;


  /**
   * @brief Values used through various tasks to be protected by mutex 
   */
  struct MutexValues {
    static transmitter_t transmitterValues; 
    static telemetry_t pandaPacket;         
    static command_t commands;              

    static bool isCalibrated; // Condition to denote whether or not the car is calibrated 
  };


  SemaphoreHandle_t Mutexes::telemetryMutex = xSemaphoreCreateMutex();  // Mutex for Ethernet telemetry values 
  SemaphoreHandle_t Mutexes::commandMutex = xSemaphoreCreateMutex();    // Mutex for motor commnds 
  SemaphoreHandle_t Mutexes::rcMutex = xSemaphoreCreateMutex();         // Mutex for RC values 

  SemaphoreHandle_t Mutexes::errorSemaphore = xSemaphoreCreateBinary(); // Binary semaphore for error handling
  
  // Structs for value packets 
  transmitter_t MutexValues::transmitterValues; // Vlaues from the RC transmitter
  telemetry_t MutexValues::pandaPacket;         // Values to send to the Panda packet 
  command_t MutexValues::commands;              // Values for the autonomous commands 

  // Other mutex values
  bool MutexValues::isCalibrated;               // Value to denote whether or not the car has been calibrated 
}

#endif // MUTEXES