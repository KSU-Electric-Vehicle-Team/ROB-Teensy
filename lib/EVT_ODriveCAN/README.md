# ODriver class

## Overview 

The `ODriver` module is used for controlling ODrive motor controllers over CAN bus. It allows for sending position targets, calibrating, resetting, and recieving values from any ODrive on the CAN bus.

---

## Important Methods

Including the constructor, there are three main important methods.
  1. [`ODriver()`](#constructor)
  2. [`calibrate()`](#calibration)
  3. [`update()`](#update)

In addition to those, there are also various methods used to get important values. Those methods are outlined in [getters](#getter-methods).

---

## Constructor 

The constructor for the ODriver class takes in two parameters, an `ODriveCAN` reference and a `uint8_t` for the CAN node ID. With these, an instance can be defined in a way that allows proper setup of callback methods needed for CAN communications with the ODriveCAN library. 

---

## Calibration

The `calibrate()` method is used to run the calibration sequence of the ODriveCAN reference. It's needed not only when the system first initialized, but also when an error occurs within the ODrive. After running the full calibration sequence, time is given for the ODrive motor to be manually centered. Then, the ODrive is set to closed loop control and the method returns the procedure result as a boolean.

---

## Update

The `update()` method is a simple method used to update the position target of the ODrive instance. It has two parameters: `targetValue` and `rateLimit`. The target value parameter is a float used to set the position setpoint of the ODriver. The rate limit parameter is an optional parameter which is a boolean used to determine whether or not to apply an input rate limit to the ODrive position input.

---

## Getter Methods

There are four important getter methods within the ODriver class.
  1. `getCalibrationFlag()`
      - Gets the flag which denotes whether or not the given ODrive has been calibrated or not
  2. `getCenterPosition()`
      - Gets the absolute center position of the ODrive in turns 
  3. `getTemperature()`
      - Gets the temperature of the ODrive in degrees celcius
  4. `getDisarmReason()`
      - Gets the disarm reason from the ODrive as an ODriveError enum