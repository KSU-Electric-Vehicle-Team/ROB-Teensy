#ifndef EVT_HydraulicBrake
#define EVT_HydraulicBrake

#include <arduino_freertos.h>
#include <bits/stdc++.h>

namespace MotorControls{
  class HydraulicBrake {
    private:
      int currentPositionSteps = 0;

      void stepperMotor(int steps, bool direction);

      void pulseStep();

    public:
      void setup();

      void home();

      bool driveMotor(float brakePercent);
  };
}

#endif EVT_HydraulicBrake // EVT_HydraulicBrake
