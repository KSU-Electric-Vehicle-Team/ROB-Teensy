# ControlRC Class

## Overview

The `ControlRC` module is used for accessing the values received over sBus transmitter. The module also maps the values recieved from each channel, and has options to map to data types other than `uint16_t`

---

## Important Methods

There are four important methods, which don't include the constructor 
  1. [`update()`](#update)
  2. [`setMapping()`](#set-mapping)
  3. [`getChannelValue()`](#get-channel-value)
  4. [`ControlRC::mapSwitches()`](#map-switches)

---

## Update 

The `update()` method is quite simple in both application and function. It uses predefined references for each channel value and sets the values according to the received input. 

---

## Set Mapping

The `setMapping()` method is extremely useful for situations where you need the value received from the RC transmitter to be used for inputs for other systems. The method takes in an array used for the mapping function and a `mapType` (see [additions](#important-additions)) to set which channel is being mapped. The array is in the form of `{min, max}` and is used directly to map the values received over sBus.

---

## Get Channel Value

The get channel value method is used mostly for debugging. It allows for the direct retrieval of the value of one of the channels from the RC transmitter. The method gets the value of the channel using the predefined mapping set up with `setMapping()` method. Additionally, you can receive the raw data by passing `mapChannel` as `false`. 

---

## Map Switches

The static method, `ControlRC::mapSwitches(uint16_t val)` is a method used to map the values received from the RC transmitter to a boolean. Essentially, it compares the value to the minimum or maximum, and returns `true` or `false` accordingly. 

Effectively, this can be represented by an equation defined as

```math
\begin{equation}
  f(n) =
    \left\{
      \begin{array}{lr}
        0, & n \lt 0.90 * max_{RC} \\
        1, & n \ge 0.90 * max_{RC}
      \end{array}
    \right.
 \end{equation}
```

given that $max_{RC}$ is the maximum value from the RC transmitter

---

## Important Additions

In addition to the important methods, there are also two important enums used for the `ControlRC` module:
  1. `ChannelRC`
      - Used to denote which channel to receive value from or which channel to map
  2. `ControlRC::mapType`
      - Used to denote what type of input is being mapped (e.g. switches, joysticks, or knobs)

---