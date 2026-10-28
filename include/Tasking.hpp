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
 * @brief Reads VESC bench test commands from the Serial monitor
 *
 * @note Commands are sent one per line: "e <erpm>" sets the ERPM, "b <amps>" sets the
 *       brake current, and "s" sends zero current so the motor coasts
 *
 * @param erpm ERPM target to update
 * @param brake Brake current target to update
 */
static void readBenchCommand(float & erpm, float & brake) {
  static char line[32];      // Serial command buffer
  static uint8_t length = 0; // Number of characters in the command buffer

  while (Serial.available() > 0) {
    const char c = Serial.read();

    if (c != '\n' && c != '\r') {
      if (length < sizeof(line) - 1) {
        line[length++] = c;
      }

      continue;
    }

    line[length] = '\0';
    length = 0;

    // Skip the command letters to find the value (accepts "e 3000", "e3000", or "erpm 3000")
    const char * value = line;
    while (isalpha(static_cast<unsigned char>(*value))) {
      value++;
    }

    switch (tolower(static_cast<unsigned char>(line[0]))) {
      case 'e':
        erpm = atoff(value);
        brake = 0.0f;
        break;
      case 'b':
        brake = atoff(value);
        erpm = 0.0f;
        break;
      case 's':
        erpm = 0.0f;
        brake = 0.0f;
        break;
      case '\0':
        break;
      default:
        Queues::logWrite("VESC bench commands: e <erpm>, b <amps>, s");
        break;
    }
  }
}


/**
 * @brief Task used to command the VESC and read its telemetry over UART
 */
static void vescTask(void * pvParameters) {
  MotorControls::VescDriver vesc {IOConstants::vescSerial};

  static char vescPrintBuffer[256]; // Kept off the task stack; the log queue only stores this pointer, so it's reused every telemetry period

  float erpm = 0.0f;  // ERPM target
  float brake = 0.0f; // Brake current target in amps

  const TickType_t telemetryPeriod = pdMS_TO_TICKS(1'000 / IOConstants::vescTelemetryFrequency);
  const TickType_t reconnectPeriod = pdMS_TO_TICKS(1'000 / IOConstants::vescReconnectFrequency);
  TickType_t lastRequest = xTaskGetTickCount() - reconnectPeriod; // Request telemetry on the first loop

  while (true) {
    if (IOConstants::vescBenchTest) {
      readBenchCommand(erpm, brake);
    } else if (xSemaphoreTake(Mutexes::commandMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      erpm = MutexValues::commands.erpm;
      brake = MutexValues::commands.brake;
      xSemaphoreGive(Mutexes::commandMutex);
    }

    // Send a command every iteration so the VESC timeout doesn't release the motor
    vesc.update(erpm, brake);

    // Each failed request waits for the full reply timeout, so poll slower while the VESC isn't responding
    const TickType_t period = vesc.isConnected() ? telemetryPeriod : reconnectPeriod;

    if (xTaskGetTickCount() - lastRequest >= period) {
      lastRequest = xTaskGetTickCount();

      // Share new telemetry with other tasks
      if (vesc.updateTelemetry() && xSemaphoreTake(Mutexes::vescMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        MutexValues::vescTelemetry = vesc.getTelemetry();
        xSemaphoreGive(Mutexes::vescMutex);
      }

      if (IOConstants::motorDataToSerial) {
        const MotorControls::vescTelemetry_t data = vesc.getTelemetry();

        if (vesc.isConnected()) {
          snprintf(
            vescPrintBuffer,
            sizeof(vescPrintBuffer),
            "VESC | Target ERPM %.0f | Brake %.1f A | ERPM %.0f | Motor %.2f A | Input %.1f V | MOSFET %.1f C | Motor %.1f C | %s (%u)",
            vesc.getTargetERPM(),
            vesc.getTargetBrake(),
            data.erpm,
            data.motorCurrent,
            data.inputVoltage,
            data.tempMosfet,
            data.tempMotor,
            MotorControls::VescDriver::faultToString(data.fault),
            static_cast<unsigned>(data.fault)
          );
        } else {
          snprintf(vescPrintBuffer, sizeof(vescPrintBuffer), "VESC | No telemetry received");
        }

        Queues::logWrite(vescPrintBuffer);
      }
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

  });

  stateMachine.defineState(Signals::States::AUTO, [&] () { // Define AUTO state
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

    stateMachine.runState();

    // Block between iterations so lower priority tasks (RC, VESC, Serial monitor) can run
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  vTaskDelete(nullptr);
}


#endif // TASKING
