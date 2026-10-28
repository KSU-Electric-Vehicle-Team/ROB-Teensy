# VescDriver Class

## Overview

The `VescDriver` module controls a VESC motor controller over UART. It is a thin wrapper around the [VescUart](https://github.com/SolidGeek/VescUart) library: VescUart builds and parses the VESC packets, and `VescDriver` adds the limits, safety checks, and telemetry handling the car needs.

With it, the Teensy can:
  - Set the motor's target ERPM
  - Set a brake current
  - Read the MOSFET and motor temperatures, motor and input current, duty cycle, measured ERPM, input voltage, and fault code

It replaces the old `EVT_VescDriver` from the deprecated repository. The packet handling (VescUart) is the same, but the structure has changed to fit FreeRTOS:
  - **RC mapping moves out of the driver:** the old driver mapped RC channels to throttle and brake itself. Now the driver only takes an ERPM and a brake target. Mapping the RC sticks belongs in the state machine's RC state, which should write `MutexValues::commands`. That isn't implemented yet, so outside bench test mode `vescTask` reads zeros and the motor coasts.
  - **One task owns the VESC:** the old driver was called from wherever it was needed. Now only `vescTask` touches the VESC, and other tasks read the shared copy of the telemetry.
  - **Fault strings without heap use:** `faultToString()` returns a `const char *` instead of an Arduino `String`. It also knows the newer fault codes (28 to 33) that VescUart's enum doesn't name.
  - **Standard clamping:** `std::clamp` replaces Arduino's `constrain` macro, which `arduino_freertos.h` removes.
  - **Safety checks:** NaN and infinite commands become 0, and ERPM commands below the VESC's minimum make the motor coast instead of braking.

---

## Important Methods

There are three important methods outside of the constructor:
  1. [`update(float erpm, float brake)`](#update)
  2. [`updateTelemetry()`](#update-telemetry)
  3. [`isConnected()`](#is-connected)

The getter methods are outlined in [getters](#getter-methods).

---

## Constructor

`VescDriver(HardwareSerial & vescSerial)` starts the UART at `IOConstants::vescBaudrate` (115200) and gives it to VescUart. The VescUart instance is created with a 20 ms reply timeout instead of its 100 ms default. That shortens how long a telemetry request can hold the CPU (see [Update Telemetry](#update-telemetry)).

---

## Update

`update()` sends exactly one command to the VESC each time it is called:
  1. If either value is NaN or infinite, it becomes 0.
  2. The ERPM is clamped between `-ControlConstants::vescMaxReverseERPM` and `ControlConstants::vescMaxERPM`.
  3. The brake current is clamped between 0 and `ControlConstants::vescMaxBrake`.
  4. Then one of three commands is sent:
      - If the brake current is above 0, `setBrakeCurrent()` is sent and the ERPM is ignored. Brake and ERPM fight each other, so braking always wins.
      - Otherwise, if the ERPM is at least `ControlConstants::vescMinERPM` in either direction, `setRPM()` is sent and the VESC's speed controller holds that ERPM.
      - Otherwise, `setCurrent(0)` is sent so the motor coasts, and the stored target becomes 0. The VESC's speed controller doesn't run below its own minimum ERPM (VESC Tool's Speed PID Minimum ERPM, 900 by default). It brakes at zero duty instead, so a small ERPM command would brake rather than spin slowly.

`setERPM(erpm)` and `setBrakeCurrent(brake)` are shortcuts for `update(erpm, 0)` and `update(0, brake)`.

These commands only write about 10 bytes into the UART's transmit buffer, so they return in microseconds and never wait for a reply.

The VESC has its own safety timeout (App Settings > General > Timeout in VESC Tool). If no command arrives within that time, it releases the motor. That is why `vescTask` calls `update()` on every loop, even when the target hasn't changed.

---

## Update Telemetry

`updateTelemetry()` asks the VESC for its values and copies them into a `vescTelemetry_t`:
  1. Any bytes already waiting in the UART are thrown away, so VescUart only sees the reply to this request.
  2. `VescUart::getVescValues()` sends `COMM_GET_VALUES` and waits for the reply.
  3. If a valid reply arrives (correct length and CRC), the temperatures, currents, duty cycle, ERPM, input voltage and fault code are copied, `lastUpdate` is set to the current time, and the method returns `true`.

**This is the one call that holds the CPU.** VescUart waits for the reply in a busy loop and never yields to FreeRTOS:
  - When the VESC is connected, the roughly 80 byte reply takes about 7 ms at 115200 baud, so the call takes about 8 ms.
  - When the VESC is disconnected, the call waits for the full 20 ms reply timeout.

To keep that from hurting other tasks:
  - `vescTask` runs at priority 2. The RC task (3) and the state machine (4) can still interrupt it while it waits; only the Serial monitor task (1) waits on it.
  - Telemetry is requested at `IOConstants::vescTelemetryFrequency` (10 Hz) instead of on every loop. That is about 8% of the CPU while connected.
  - While the VESC isn't responding, requests drop to `IOConstants::vescReconnectFrequency` (1 Hz), so a missing VESC costs about 2% instead of 20%.

---

## Is Connected

`isConnected()` returns `true` if valid telemetry has arrived in the last 500 ms. At 10 Hz, that means five requests in a row have to fail before the VESC counts as disconnected.

---

## Getter Methods

There are four getter methods:
  1. `getTelemetry()`
      - Gets a copy of the latest `vescTelemetry_t`
  2. `getTargetERPM()`
      - Gets the last ERPM command after the limits were applied (0 when the motor was set to coast)
  3. `getTargetBrake()`
      - Gets the last brake current command after the limits were applied
  4. `VescDriver::faultToString()`
      - Converts the VESC fault code to a string such as `FAULT_CODE_OVER_TEMP_FET`, or `FAULT_CODE_UNKNOWN` for a code it doesn't recognize

---

## RTOS Integration

`vescTask` in `Tasking.hpp` is the only code that uses the `VescDriver`. On every loop (`IOConstants::updateFrequency`, 80 Hz) it:
  1. Gets the ERPM and brake targets:
      - Normally, it copies `MutexValues::commands.erpm` and `.brake` while holding `commandMutex`.
      - In bench test mode, it reads them from the Serial monitor (see [Bench Testing](#bench-testing)).
  2. Calls `update()` to send the command.
  3. Requests telemetry on the first loop, then every 100 ms (or every second while the VESC isn't responding). It calls `updateTelemetry()` and copies the result into `MutexValues::vescTelemetry` while holding `vescMutex`.
  4. If `IOConstants::motorDataToSerial` is `true`, it formats the telemetry and sends it to the Serial monitor through the log queue.

Other tasks read the VESC values by taking `vescMutex` and copying `MutexValues::vescTelemetry`. They never call the driver directly. VescUart isn't safe to use from two tasks at once: their packets would mix on the UART, and one task could read the other's reply.

The task stack is 1024 words (4 KB). VescUart keeps two 256 byte packet buffers on the stack during a telemetry request, and `snprintf` with floats needs more.

`stateMachineTask` ends each loop with `vTaskDelay(10 ms)`. FreeRTOS always runs the highest priority task that is ready, so a priority 4 loop that never blocks would keep every lower priority task (RC, `vescTask`, and the Serial monitor) from running. Those lower priority tasks now run for the first time, so the RC and Serial monitor stacks were raised from 128 to 256 words to fit their call depth plus the 104 byte floating point frame every interrupt pushes.

---

## Bench Testing

1. In VESC Tool, set the app to use UART at 115200 baud. Check the motor's current and ERPM limits, and check that the Speed PID Minimum ERPM matches `ControlConstants::vescMinERPM`.
2. Wire the Teensy `Serial2` pins (RX2 pin 7, TX2 pin 8) to the VESC's TX and RX, and connect the grounds.
3. Set `ControlConstants::vescMaxERPM`, `vescMaxReverseERPM`, `vescMinERPM`, and `vescMaxBrake` to safe values for the bench motor.
4. Set `IOConstants::vescBenchTest` and `IOConstants::motorDataToSerial` to `true`, then build and upload.
5. Open the Serial monitor and send one command per line:
    - `e 2000` sets the target ERPM to 2000
    - `b 5` sets the brake current to 5 A
    - `s` sends zero current so the motor coasts
6. The Serial monitor prints a line like this 10 times per second:
    ```
    VESC | Target ERPM 2000 | Brake 0.0 A | ERPM 1998 | Motor 1.25 A | Input 24.1 V | MOSFET 31.4 C | Motor 29.8 C | FAULT_CODE_NONE (0)
    ```
    Compare the temperatures with VESC Tool. The number in brackets is the raw fault code.

Set both flags back to `false` when you're done. In bench mode the motor keeps its last command until you send `s`, or until the Teensy stops sending and the VESC timeout releases the motor.
