#include <IOConstants.hpp>
using Constants::IOConstants;

#include "Tasking.hpp"

/**
 * @brief One time setup
 */
void setup() {
  // Set the on board LED pin to output
  arduino::pinMode(IOConstants::ledBuiltIn, arduino::OUTPUT);

  arduino::pinMode(40, arduino::OUTPUT);
  arduino::pinMode(41, arduino::OUTPUT);

  xTaskCreate(        // Create a task to log messages to the Serial monitor
    printTask,        // Task function to be called
    "Serial Monitor", // Task name as a string
    128,              // Task size in words (1 word = 4 bytes)
    NULL,             // Parameters
    1,                // Task priority (0 - 9)
    NULL              // Task handle
  );

  xTaskCreate( // Create a task to blink the onboard LED
    blinkTask,
    "Blink",
    128,
    NULL,
    2,
    NULL
  );

  xTaskCreate( // Create a task to update the RC values from SBUS
    rcTask,
    "RC Updates",
    128,
    NULL,
    3,
    NULL
  );

  xTaskCreate( // Create a task to update the ODrive using CAN bus
    oDriveTask,
    "ODrive UART Updates",
    256,
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
