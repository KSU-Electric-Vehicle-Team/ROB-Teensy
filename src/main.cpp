#include <IOConstants.hpp>
using Constants::IOConstants;

#include "Tasking.hpp"

/**
 * @brief One time setup
 */
void setup() {
  // Set the on board LED pin to output
  arduino::pinMode(IOConstants::ledBuiltIn, arduino::OUTPUT);

  xTaskCreate(        // Create a task to log messages to the Serial monitor
    printTask,        // Task function to be called
    "Serial Monitor", // Task name as a string
    128,              // Task size in words (1 word = 4 bytes)
    NULL,             // Parameters
    1,                // Task priority (0 - 9)
    NULL              // Task handle
  );

  xTaskCreate( // Create a task to update the RC values from SBUS
    rcTask,
    "RC Updates",
    128,
    NULL,
    3,
    NULL
  );

  xTaskCreate( // Create a task to handle the state machine
    stateMachineTask,
    "State Machine Task",
    256,
    NULL,
    4,
    NULL
  );

  Mutexes::blinkTimer = xTimerCreate(
    "Onboard LED blink",                                                                  // Timer name
    pdMS_TO_TICKS(IOConstants::ledBlinkFrequency * ConversionConstants::secToMillis / 2), // Timer period
    pdTRUE,                                                                               // Auto-reload
    (void *)0,                                                                            // Timer ID
    blinkCallback                                                                         // Callback function
  );


  // Start the timer
  xTimerStart(Mutexes::blinkTimer, portMAX_DELAY);

  // Start the RTOS scheduler
  vTaskStartScheduler();

  // Delete the setup and loop tasks after starting the scheduler
  vTaskDelete(nullptr);
}


/**
 * @brief Default loop task
 *
 * @note With the way that FreeRTOS works, loop should be left empty
 */
void loop() {}
