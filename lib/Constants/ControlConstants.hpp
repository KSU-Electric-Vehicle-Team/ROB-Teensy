#ifndef CONTROL_CONSTANTS
#define CONTROL_CONSTANTS

namespace Constants{
  struct ControlConstants{

    // Change these values
    static constexpr int dynamicBrakeMaxSteps = 0; // max steps from the home position to the fully extended position
    static constexpr int dynamicBrakePedalOffsetSteps = 0; // there's a gap from the home position to the point where the brake actually starts to engage, so this is the number of steps to get past that gap
    static constexpr int dynamicBrakeHomeMaxSteps = 0; // max steps from the extended position to the home position during homing, if it passes this we know the pinion is loose
    static constexpr unsigned int dynamicBrakeStepPulseUs = 10;
    static constexpr unsigned int dynamicBrakeStepDelayUs = 10;
    static constexpr unsigned int dynamicBrakeDirSetupUs = 20;
    static constexpr bool dynamicBrakeForwardDirLevel = true;
    static constexpr bool dynamicBrakeLimitActiveLow = true;
  };
}

#endif CONTROL_CONSTANTS // CONTROL_CONSTANTS
