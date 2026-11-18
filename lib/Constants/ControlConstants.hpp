#ifndef CONTROL_CONSTANTS
#define CONTROL_CONSTANTS

namespace Constants{
  struct ControlConstants{

    // Change these values
    static constexpr int dynamicBrakeMaxSteps = 9000; // max steps from the home position to the fully extended position
    static constexpr int dynamicBrakePedalOffsetSteps = 500; // there's a gap from the home position to the point where the brake actually starts to engage, so this is the number of steps to get past that gap
    static constexpr int dynamicBrakeHomeMaxSteps = 20000; // max steps from the extended position to the home position during homing, if it passes this we know the pinion is loose
    static constexpr unsigned int dynamicBrakeStepPulseMs = 0.01;
    static constexpr unsigned int dynamicBrakeStepDelayMs = 0.01;
    static constexpr unsigned int dynamicBrakeDirSetupMs = 0.02;
    static constexpr bool dynamicBrakeForwardDirLevel = true;
    static constexpr bool dynamicBrakeLimitActiveLow = true;
  };
}

#endif CONTROL_CONSTANTS // CONTROL_CONSTANTS
