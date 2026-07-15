#ifndef EVT_RC
#define EVT_RC

/*-----------------------------------------------------------------------------*/
/** 
 * @file   EVT_RC.hpp
 * @brief  Header for ControlRC class
 * 
 * The ControlRC class is used to receive data from an RC receiver over SBUS. 
 * The UART TX from the RC receiver is connected to the SBUS serial port defined 
 * in the IOConstants struct. In addition, this class also allows for the retrevial 
 * of specific channels through enums, as well as multiple mapping functions 
 * for those received channels.
 * 
 * @author Lilia Turbeville
 * @date   June 16, 2026
*//*---------------------------------------------------------------------------*/

#include <sbus.h>
using bfs::SbusRx;
using bfs::SbusData;

#include <TransmitterConstants.hpp>
#include <IOConstants.hpp>
using namespace Constants;

#include "Queues.hpp"
using RTOS::Queues;

namespace Signals {
  /**
   * @brief Enum used for selection of RC channel values 
   */
  typedef enum : uint8_t {
    RIGHT_X = 0, // Joystick axis for steering
    RIGHT_Y,     // Unassigned joystick axis 
    LEFT_Y,      // Joystick axis for throttle and brake
    LEFT_X,      // Unassinged joystick axis
    SWA,         // Toggle switch for calibration 
    SWB,         // Unassinged three state toggle switch
    SWC,         // Unassinged toggle switch
    SWD,         // Toggle switch for entering AUTO
    SWF,         // Toggle switch for entering RC 
    SWE,         // Unassinged three state toggle switch
    SWH,         // Momentary toggle switch for reset 
    SWG,         // Unassinged three state toggle switch
    VRA,         // Unassinged variable knob
    VRB,         // Unassigned variable knob 
    VRC,         // Unassigned variable knob
    VRD          // Unassigned variable knob
  } ChannelRC;


  /**
   * @brief Class used to receive and interpret SBUS data 
   */
  class ControlRC {
    private:
      char channelPrint[32];                           // Channel Serial print buffer

      SbusRx sbusRX {&IOConstants::sBusSerial};        // SBUS receiver pin
      SbusData data;                                   // SBUS data instance

      size_t lastValidFrame = 0UL;                     // Timestamp of the last frame 
      static const size_t validFrameTimeout = 1'000UL; // Maximum allowed time between valid frames

      uint16_t channelVal[TransmitterConstants::numChannels]; // Values of each RC channel as an array
      uint16_t nextValues[TransmitterConstants::numChannels]; // Values of the next RC channels

      uint16_t joystickMap[2];                         // Default joystick map
      uint16_t switchMap[2];                           // Default toggle switch map
      uint16_t triSwitchMap[3];                        // Default three state switch map
      uint16_t knobMap[2];                             // Default knob map

      bool hasValidFrame;                              
    public:
      /**
       * @brief Enum used for selection of mapping values
       */
      typedef enum : uint8_t {
        JOYSTICK = 0,
        SWITCH,
        TRI_SWITCH,
        KNOB
      } mapType;


      /**
       * @brief Define a new ControlRC instance
       */
      ControlRC();


      /**
       * @brief Sets the mapping array given the type of mapping to set
       * 
       * @param mapArray Array to use to set the mapping values
       * @param mappingType Type of mapping to set
       */
      void setMapping(const uint16_t mapArray[2], mapType mappingType);


      /**
       * @brief Update the data received from the SBUS 
       */
      bool update();  


      /**
       * @brief Prints the value of a given channel
       * 
       * @param isMapped Condition to map the values when printing (Default false)
       */
      void printChannel(ChannelRC channel, bool isMapped = false);


      /**
       * @brief Get the value of a given channel
       * 
       * @param channel Channel to get the value of
       * @param mapChannel Condition for mapping the value from the channel
       * @return Value received, and possibly mapped, from the channel  
       */
      uint16_t getChannelValue(ChannelRC channel, bool mapChannel = true);


      /**
       * @brief Gets the value of a channel given a method to map to non-integer types
       * 
       * @note This method can also be used to apply mapping curves 
       * 
       * @tparam T Type to map to 
       * @param channel Channel to get the value from 
       * @param mapFunction Method to use to map the value (Must only take in a single uint16_t)
       * @return Mapped value from the channel
       */
      template <class T>
      inline T getChannelValue(ChannelRC channel, T (*mapFunction)(uint16_t)) {
        return mapFunction(channelVal[channel]);
      }


      /**
       * @brief Gets the value of a chanel given a lambda expression to map to non-integer types
       * 
       * @note When using this method, you have to specify the expected return type when calling 
       * 
       * @tparam T Type to map to 
       * @tparam F Lambda template type
       * @param channel Channel to get the value from 
       * @param mapLambda Lambda expression to use to map the value (Must only take in a single uint16_t)
       * @return T Mapped value from the channel
       */
      template <class T, typename F>
      inline T getChannelValue(ChannelRC channel, F&& mapLambda) {
        return mapLambda(channelVal[channel]);
      } 


      /**
       * @brief Gets the data from the SBUS
       * 
       * @return The data from the SBUS UART 
       */
      SbusData getData() const;


      /**
       * @brief Maps the value of switches to booleans 
       * 
       * @param val The value of the switch as an uint16_t
       * @return The value of the switch as a boolean 
       */
      static bool mapSwitches(uint16_t val);


      /**
       * @brief Maps a value within a range to a percentage value 
       * 
       * @tparam T Data type of the value to be mapped (double, int, etc.)
       * @param value Value to be mapped to a percent
       * @param mapArray Array to use to map the value
       * @return Mapped percentage value
       */
      template <class T>
      static T mapToPercent(T value, const T (&mapArray)[2]);
  };
}

#endif // EVT_RC