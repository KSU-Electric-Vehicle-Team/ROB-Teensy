#ifndef STATE_MACHINE
#define STATE_MACHINE

/*-----------------------------------------------------------------------------*/
/**
 * @file   EVT_StateMachine.hpp
 * @brief  Header for StateMachine class
 *
 * The StateMachine class is used for control of a system with predefined states.
 * Through its use, multiple states can be defined an run easily.
 *
 * @author Lilia Turbeville
 * @date   July 1, 2026
*//*---------------------------------------------------------------------------*/

#include "arduino_freertos.h"
#include <map>

#include <functional>
using std::function;

#include "Queues.hpp"
using RTOS::Queues;

/**
 * @brief Namespace used for commuinications drivers
 */
namespace Signals {
  /**
   * @brief Enum used to denote the current state of the system
   */
  typedef enum : uint8_t {
    NONE = 0, ///< State for pre-idle tasks
    IDLE,     ///< State for idle tasks
    RC,       ///< State for remote control tasks
    AUTO,     ///< State for autonomous tasks
    ERROR,    ///< State for error monitoring
    STOP,     ///< State for error handling
    RESET     ///< State for resetting
  } States;


  /**
   * @brief Struct used for control over states
   */
  typedef struct {
    States state;              ///< State enum for map indexing
    const char * name;         ///< Name of the state as a string

    function<void(void)> func; ///< State function
  } state_t;


  /**
   * @brief Class used for definition and control of a state machine
   */
  class StateMachine {
    private:
      std::map<States, state_t> stateMap; // State map to store and index states
      States currentState;                // Current system state as an enum

      char statePrint[32];                // State print serial buffer


      /**
       * @brief Sets all states to do nothing
       *
       * @note If the Teensy has an undefined state, it will throw an error.
       *       Because of this, defining all states to have something to
       *       run is absolutely necessary
       */
      void setStateDefaults();
    public:
      /**
       * @brief Defines a new instance of state machine
       *
       * @param initState Initial state for the system to be defined with (Default NONE)
       */
      StateMachine(States initState = States::NONE);


      /**
       * @brief Sets the current state
       *
       * @param newState New state to run
       */
      void setState(States newState);


      /**
       * @brief Runs the current state
       */
      void runState();


      /**
       * @brief Defines a state behavior given the state and a void function
       *
       * @note States should be defined in a way where they cannot be a separate task.
       *       If a state is something that should run continuously, or if it requires
       *       a large amount of time, it should be made as a task instead
       *
       * @param state State to set the behavior of
       * @param name State name as a string
       * @param func Function to run for the given state
       */
      void defineState(States state, const char * name, function<void(void)> func);


      /**
       * @brief Defines a state behavior given the state and a void function
       *
       * @note States should be defined in a way where they cannot be a separate task.
       *       If a state is something that should run continuously, or if it requires
       *       a large amount of time, it should be made as a task instead
       *
       * @param state State to set the behavior of
       * @param func Function to run for the given state
       */
      void defineState(States state, function<void(void)>);


      /**
       * @brief Prints the current state to the Serial monitor
       */
      void printState();


      /**
       * @brief Set the state machine into ERROR state
       */
      void setErrorState();


      /**
       * @brief Gets the current state as an enum
       *
       * @return The current state as a States enum
       */
      States getState() const;


      /**
       * @brief Checks if the state machine is in a given state
       *
       * @param checkState State to check for
       * @return Whether or not the state machine is currently running the checked state
       */
      bool isInState(States checkState) const;


      /**
       * @brief Gets the given state as a string
       *
       * @param state State to get the value of as a string
       * @return The name of the given state as a string
       */
      const char * toString(States state);
  };
}

#endif // STATE_MACHINE
