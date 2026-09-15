#ifndef TASKING
#define TASKING

/*-----------------------------------------------------------------------------*/
/**
 * @file   Tasking.hpp
 * @brief  Methods used for RTOS tasks
 *
 * The methods and structs found in tasking are used to make the main loop
 * a bit more streamlined. Essentially, every task method for the system is
 * defined here and later used in the setup function in main to actually create
 * and run the tasks with FreeRTOS
 *
 * @author Lilia Turbeville
 * @date   June 16, 2026
*//*---------------------------------------------------------------------------*/

#include <EVT_RC.hpp>
#include <EVT_ODriveCAN.hpp>

#include "Mutexes.hpp"
using RTOS::Mutexes;
using RTOS::MutexValues;


/**
 * @brief Timer callback used to blink the onboard LED
 */
static void blinkCallback(TimerHandle_t xTimer) {
  // Toggle the LED state
  MutexValues::ledState = !MutexValues::ledState;

  // Write the LED state to the LED pin
  digitalWriteFast(IOConstants::ledBuiltIn, MutexValues::ledState ? arduino::HIGH : arduino::LOW);
}


/**
 * @brief Task used to print messages from the log queue to Serial monitor
 */
static void printTask(void * pvParameters) {
  const char * msg; // Log message buffer
  const char * err; // Error message buffer

  while (true) {
    // Print a message to the Serial Monitor if one is in the queue
    if (xQueueReceive(Queues::logQueue, (void *)&msg, pdTICKS_TO_MS(50)) == pdTRUE) {
      Serial.println(msg);
    }

    // Prints a message to the Serial Monitor for error logging
    if (xQueueReceive(Queues::errorQueue, (void *)&err, pdTICKS_TO_MS(50)) == pdTRUE) {
      Serial.println("=============== ERROR =============");
      Serial.println(err);
      Serial.println("===================================");

      xSemaphoreGive(Mutexes::errorSemaphore);
    }
  }

  // Delete the task if the while loop exits
  vTaskDelete(nullptr);
}


/**
 * @brief Task used to update the RC values from the SBUS
 */
static void rcTask(void * pvParameters) {
  Signals::ControlRC transmitter;

  while (true) {
    if (transmitter.update()) {
      if (xSemaphoreTake(Mutexes::rcMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        MutexValues::transmitterValues.update(&transmitter);
        xSemaphoreGive(Mutexes::rcMutex);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1'000 / IOConstants::updateFrequency));
  }

  vTaskDelete(nullptr);
}


/**
 * @brief Task used to control the ODrive over CAN bus
 */
static void oDriveTask(void * pvParameters) {
  // Define the steering motor and ODriver instance
  ODriveCAN steeringODrive {wrap_can_intf(MutexValues::canBus), IOConstants::steeringMotorCanID};
  MotorControls::ODriver steeringMotor {steeringODrive, IOConstants::steeringMotorCanID};

  while (true) {
    if (xSemaphoreTake(Mutexes::canMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      if (MutexValues::oDriveSetupFlag) { // Run ODrive code if ODrive CAN has been setup
        if (steeringMotor.getCalibrationFlag()) {
          if (xSemaphoreTake(Mutexes::commandMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // Update the ODrive motor position using Mutex values
            steeringMotor.update(MutexValues::commands.steering, MutexValues::commands.isSteeringLimited);

            xSemaphoreGive(Mutexes::commandMutex);
          }
        } else {
          steeringMotor.calibrate(); // Run the calibration sequence
          MutexValues::steeringCenter = steeringMotor.getCenterPosition(); // Record the center position
        }

      } else { // Run setup functions if CAN hasn't been setup
        if (!MutexValues::canSetupFlag) {
          MutexValues::canSetupFlag = MotorControls::setupCan();
        }

        MutexValues::oDriveSetupFlag = steeringMotor.setup();
      }

      xSemaphoreGive(Mutexes::canMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(1'000 / IOConstants::updateFrequency));
  }

  vTaskDelete(nullptr);
}


/**
 * @brief Task used to update and run the state machine
 */
static void stateMachineTask(void * pvParameters) {
  Signals::StateMachine stateMachine;

  stateMachine.defineState(Signals::States::NONE, [&] () { // Define NONE state
    // Run any pre-IDLE code

    // Move on to IDLE state
    stateMachine.setState(Signals::States::IDLE);
  });

  stateMachine.defineState(Signals::States::IDLE, [&] () { // Define IDLE state
    // Run any IDLE code

    // Move on to RC state
    stateMachine.setState(Signals::States::RC);
  });

  stateMachine.defineState(Signals::States::RC, [&] () { // Define RC state
    if (xSemaphoreTake(Mutexes::commandMutex, 0) == pdTRUE && xSemaphoreTake(Mutexes::rcMutex, 0) == pdTRUE) {
      MutexValues::commands.isSteeringLimited = true; // Set the steering limiter to true
      MutexValues::commands.steering = map(           // Map the deadbanded steering input to position values
        (float)MutexValues::transmitterValues.leftJoystick.deadband().x,
        (float)TransmitterConstants::minRC,
        (float)TransmitterConstants::maxRC,
        MutexValues::steeringCenter - ControlConstants::steeringMaxTurns,
        MutexValues::steeringCenter + ControlConstants::steeringMaxTurns
      );

      xSemaphoreGive(Mutexes::rcMutex);
      xSemaphoreGive(Mutexes::commandMutex);
    }
  });

  stateMachine.defineState(Signals::States::AUTO, [&] () { // Define AUTO state
    // Set the motor commands with recieved packet values
    if (xSemaphoreTake(Mutexes::commandMutex, 0) == pdTRUE) {
      MutexValues::commands.isSteeringLimited = false;
      MutexValues::commands.isDriveLimited = false;

      xSemaphoreGive(Mutexes::commandMutex);
    }
  });

  stateMachine.defineState(Signals::States::ERROR, [&] () { // Define ERROR state
    // Determine if the car is in need of an urgent stop or an E-Stop

    // Move on to STOP state if the car doesn't have to cut power
    stateMachine.setState(Signals::States::STOP);
  });

  stateMachine.defineState(Signals::States::STOP, [&] () { // Define STOP state
    // Perform an urgent stop and then record the error that occured

    // Move on to RESET state to recalibrate the motors
    stateMachine.setState(Signals::States::RESET);
  });

  stateMachine.defineState(Signals::States::RESET, [&] () { // Define RESET state
    // Clear error codes from the ODrive and VESC

    // Move on to IDLE state to return control to the car
    stateMachine.setState(Signals::States::IDLE);
  });


  while (true) {
    if (xSemaphoreTake(Mutexes::telemetryMutex, 0) == pdTRUE) {
      // Copy the state as a string to the Panda telemetry
      strcpy(MutexValues::pandaPacket.state, stateMachine.toString(stateMachine.getState()));

      xSemaphoreGive(Mutexes::telemetryMutex); // Give the telemetry mutex
    }

    // Set the state machine into error state if the binary semaphore can be taken
    if (xSemaphoreTake(Mutexes::errorSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
      stateMachine.setErrorState();
    }

    // Run the current state in the state machine
    stateMachine.runState();

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  vTaskDelete(nullptr);
}


#endif // TASKING
