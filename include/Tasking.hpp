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

#include "Mutexes.hpp"
using RTOS::Mutexes;
using RTOS::MutexValues;


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
 * @brief Task used to blink the onboard LED 
 */
static void blinkTask(void * pvParameters) {
  bool ledState = false; // Current LED state as a boolean
  
  while (true) {
    // Toggle the LED state
    ledState = !ledState;

    // Write the LED state to the LED pin 
    digitalWriteFast(IOConstants::ledBuiltIn, ledState ? arduino::HIGH : arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(500 / IOConstants::ledBlinkFrequency));

    // Take the telemetry mutex if available 
    if (xSemaphoreTake(Mutexes::telemetryMutex, 0) == pdTRUE) {
      // Copy the LED state to the telemetry packet
      strcpy(MutexValues::pandaPacket.state, ledState ? "ON" : "OFF");

      // Return the telemetry mutex
      xSemaphoreGive(Mutexes::telemetryMutex);
    }
  }

  // Delete the task if the while loop exits
  vTaskDelete(nullptr);
}


/**
 * @brief Task used to write the elapsed time in seconds since boot to Serial
 */
static void messageTask(void * pvParameters) {
  while (true) {
    if (xSemaphoreTake(Mutexes::telemetryMutex, 0) == pdTRUE) {
      if (xSemaphoreTake(Mutexes::rcMutex, 0) == pdTRUE) {
        MutexValues::pandaPacket.throttle = MutexValues::transmitterValues.leftJoystick.y;
        MutexValues::pandaPacket.steering = MutexValues::transmitterValues.rightJoystick.x;

        xSemaphoreGive(Mutexes::rcMutex);
      }

      MutexValues::pandaPacket.format();                 // Format the telemetry buffer with the given data in the pandaPacket
      Queues::logWrite(MutexValues::pandaPacket.buffer); // Put the pandaPacket buffer in the log queue

      xSemaphoreGive(Mutexes::telemetryMutex); // Give the telemetry mutex 
    }

    vTaskDelay(pdMS_TO_TICKS(50));
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
    if (!transmitter.update()) {
      Queues::logWrite("Awaiting valid SBUS frame...");
    } else {
      if (xSemaphoreTake(Mutexes::rcMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        MutexValues::transmitterValues.update(&transmitter);

        xSemaphoreGive(Mutexes::rcMutex);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1'000 / IOConstants::updateFrequency));
  }

  vTaskDelete(nullptr);
}


#endif // TASKING