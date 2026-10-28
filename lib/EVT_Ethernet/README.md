# EthernetEVT Class

## Overview

The `EthernetEVT` module is used to send and receive values over Ethernet connections using UDP packets

---

## Important Methods

There are two important methods outside of the constructor:
  1. [`sendTelemetry(const char * buff)`](#send-telemetry)
  2. [`receiveTelemetry()`](#receive-telemetry)

In the most basic sense, they calculate the new value and set the maximum change rates, respectively.

---

## Send Telemetry

The `sendTelemtetry()` method sends a telemetry buffer to the IP address associated with the instance of EthernetEVT

---

## Receive Telemetry

The `receiveTelemetry()` method receieves a telemetry buffer from the given IP address associated with the instance of EthernetEVT and saves it into the shared MutexValues

---
