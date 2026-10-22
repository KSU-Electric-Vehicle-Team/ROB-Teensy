# VescDriver Class

## Overview

The `VescDriver` module is used to control a VESC motor controller over UART. It sends ERPM and brake current commands and reads telemetry (temperatures, currents, input voltage, measured ERPM, and fault codes) from the VESC.

Unlike the old `EVT_VescDriver`, this module doesn't use the VescUart library. VescUart busy-waits for up to 100 ms on every telemetry request, which blocks an RTOS task. `VescDriver` sends a request and then parses the response a byte at a time as it arrives, so it never blocks.

---

## Important Methods

There are four important methods outside of the constructor:
  1. [`update(float erpm, float brake)`](#update)
  2. [`requestTelemetry()`](#request-telemetry)
  3. [`readTelemetry()`](#read-telemetry)
  4. [`getTelemetry()`](#get-telemetry)

---

## Update

The `update()` method sends a single command to the VESC. Both values are clamped to the limits in `ControlConstants`.
  - If `brake` is greater than zero, a brake current command is sent and the ERPM is ignored
  - Otherwise, if `erpm` isn't zero, an ERPM command is sent
  - If both are zero, a zero current command is sent so the motor coasts

`setERPM()` and `setBrakeCurrent()` are shortcuts for `update(erpm, 0)` and `update(0, brake)`.

The VESC releases the motor if it doesn't receive a command within its UART timeout (App Settings > General > Timeout in VESC Tool), so `update()` must be called continuously.

---

## Request Telemetry

The `requestTelemetry()` method asks the VESC for its values with `COMM_GET_VALUES`. The VESC only sends telemetry when asked.

---

## Read Telemetry

The `readTelemetry()` method parses every byte waiting in the UART buffer and returns `true` when a complete frame with a valid CRC has been received. The constructor adds 256 bytes to the UART RX buffer so a full telemetry frame fits between calls.

---

## Get Telemetry

The `getTelemetry()` method returns the latest values as a `vescTelemetry_t`. `isConnected()` returns `false` if no telemetry has been received in the last 500 ms, and `VescDriver::faultToString()` converts the fault code to a string.

---

## RTOS Integration

`vescTask` in `Tasking.hpp` runs the driver at `IOConstants::updateFrequency`:
  - ERPM and brake targets are read from `MutexValues::commands` under `commandMutex`
  - Telemetry is requested at `IOConstants::vescTelemetryFrequency` and copied into `MutexValues::vescTelemetry` under `vescMutex`
  - If `IOConstants::motorDataToSerial` is `true`, the telemetry is printed through the log queue

---

## Bench Testing

1. In VESC Tool, set the app to use UART at 115200 baud and check the motor and current limits.
2. Wire the Teensy `Serial2` pins (RX2 pin 7, TX2 pin 8) to the VESC TX and RX, and connect the grounds.
3. Set `IOConstants::vescBenchTest` and `IOConstants::motorDataToSerial` to `true`, then build and upload.
4. Open the Serial monitor and send one command per line:
    - `e 2000` sets the target ERPM to 2000
    - `b 5` sets the brake current to 5 A
    - `s` stops sending ERPM and brake, which lets the motor coast
5. Check the printed MOSFET and motor temperatures against VESC Tool.

Set both flags back to `false` when you're done. Bench mode ignores `MutexValues::commands`, and the motor keeps its last command until you send `s` or cut power.
