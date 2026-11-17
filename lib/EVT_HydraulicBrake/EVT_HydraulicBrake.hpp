#ifndef EVT_HydraulicBrake
#define EVT_HydraulicBrake

namespace MotorControls{
  class HydraulicBrake {
    private:
      bool homed = false;
      int currentPositionSteps = 0;

      void stepperMotor(int steps, bool direction);
      void pulseStep();
      bool isLimitHit();

    public:
      void setup();

      bool home();

      bool update(float brakePercent);
  };
}

#endif EVT_HydraulicBrake // EVT_HydraulicBrake
