#ifndef TASKING
#define TASKING

/*-----------------------------------------------------------------------------*/
/**
 * @file   Tasking.hpp
 * @brief  Methods used for RTOS tasks
 *
 * The metohods and structs found in tasking are used to make the main loop
 * a bit more streamlined. Essentially, every task method for the system is
 * defined here and later used in the setup function in main to actually create
 * and run the tasks with FreeRTOS
 *
 * @author Lilia Turbeville
 * @date   June 16, 2026
*//*---------------------------------------------------------------------------*/

#include <EVT_RC.hpp>

#include <ConversionConstants.hpp>
using namespace Constants;

#include "Mutexes.hpp"
#include "EthernetPacket.hpp"
#include "PacketIO.hpp"
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
  const char * msg;

  while (true) {
    // Print a message to the Serial Monitor if one is in the queue
    if (xQueueReceive(Queues::logQueue, (void *)&msg, pdTICKS_TO_MS(50)) == pdTRUE) {
      Serial.println(msg);
    }
  }

  // Delete the task if the while loop exits
  vTaskDelete(nullptr);
}


/**
 * @brief Task used to update the RC values from the SBUS
 */
static void rcTask(void * pvParameters) {
  Networking::ControlRC transmitter;

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


static void ethernetTask(void * pvParameters) {
  // Make ethernet object
  IPAddress teensyIP {192, 168, 0, 177};
  IPAddress panda {169, 254, 1, 10};
  byte teensy[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

  Networking::EthernetEVT ethernet {teensyIP, panda, teensy};
  // Setup ethernet
  ethernet.setup();

  while (true) {
    // Format telemetry packet.format()
    if (xSemaphoreTake(Mutexes::telemetryMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      // send telemetry (MutexValues::telemetry.buffer)
      // ethernet.sendTelemetry(buildPacket()); # Add the required parameters later

      // if auto state, recieve telemetry, check packet.state to figure out
      ethernet.receiveTelemetry();

      // Give mutex back
      xSemaphoreGive(Mutexes::telemetryMutex);
    }

    // set recieved values to MutexValues::commands
    if (xSemaphoreTake(Mutexes::commandMutex, pdMS_TO_TICKS(15)) == pdTRUE) {
      // MutexValues::commands.erpm = MutexValues::telemetry.rpm;
      // MutexValues::commands.steering = MutexValues::telemetry.oDriveTarget;
      xSemaphoreGive(Mutexes::commandMutex);
    }
  }
}


/**
 * @brief Task used to update and run the state machine
 */
static void stateMachineTask(void * pvParameters) {
  Networking::StateMachine stateMachine;

  stateMachine.defineState(Networking::States::NONE, [&] () { // Define NONE state
    // Run any pre-IDLE code

    // Move on to IDLE state
    stateMachine.setState(Networking::States::IDLE);
  });

  stateMachine.defineState(Networking::States::IDLE, [&] () { // Define IDLE state
    // Run any IDLE code

    // Move on to RC state
    stateMachine.setState(Networking::States::RC);
  });

  stateMachine.defineState(Networking::States::RC, [&] () { // Define RC state

  });

  stateMachine.defineState(Networking::States::AUTO, [&] () { // Define AUTO state

  });

  stateMachine.defineState(Networking::States::ERROR, [&] () { // Define ERROR state
    // Determine if the car is in need of an urgent stop or an E-Stop

    // Move on to STOP state if the car doesn't have to cut power
    stateMachine.setState(Networking::States::STOP);
  });

  stateMachine.defineState(Networking::States::STOP, [&] () { // Define STOP state
    // Perform an urgent stop and then record the error that occured

    // Move on to RESET state to recalibrate the motors
    stateMachine.setState(Networking::States::RESET);
  });

  stateMachine.defineState(Networking::States::RESET, [&] () { // Define RESET state
    // Clear error codes from the ODrive and VESC

    // Move on to IDLE state to return control to the car
    stateMachine.setState(Networking::States::IDLE);
  });


  while (true) {
    if (xSemaphoreTake(Mutexes::telemetryMutex, 0) == pdTRUE) {
      // Copy the state as a string to the Panda telemetry
      // strcpy(MutexValues::telemetry.state, stateMachine.toString(stateMachine.getState()));

      xSemaphoreGive(Mutexes::telemetryMutex); // Give the telemetry mutex
    }

    stateMachine.runState();
  }

  vTaskDelete(nullptr);
}


#endif // TASKING
