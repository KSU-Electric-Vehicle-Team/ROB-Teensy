# KSU EVT firmware review instructions

- Treat code that changes relays, pins, serial buses, CAN messages, motor
  controllers, timing, or state transitions as hardware-affecting.
- Flag buffer bounds, blocking calls in time-sensitive paths, race conditions,
  unsafe state transitions, unit mismatches, and missing error handling.
- Check changes against the Teensy 4.1 and PlatformIO configuration in
  `platformio.ini`.
- Ask for a test plan when a change cannot be covered by compilation or unit
  tests.
- Do not claim a successful build proves safe vehicle behavior.
- Prefer a focused fix over unrelated cleanup in the same pull request.

