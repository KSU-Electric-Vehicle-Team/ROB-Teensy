#ifndef CONTROL_CONSTANTS
#define CONTROL_CONSTANTS

/*-----------------------------------------------------------------------------*/
/**
 * @file   ControlConstants.hpp
 * @brief  Header for ControlConstants struct
 *
 * The ControlConstants struct is used for the definition of various constants
 * that are used for system behaviors. For example, the values for PID gains
 * or slewrates can be found here.
 *
 * @author Lilia Turbeville
 * @date   March 18, 2026
*//*---------------------------------------------------------------------------*/

#include <stdint.h>

namespace Constants {
  /**
   * @brief Constants used for control of the system's behavior
   */
  struct ControlConstants {
    static constexpr float steeringMaxTurns = 2.1f;            ///< Steering motor max turns from the center in either direction
    static constexpr float steeringVelLimit = 140.0f;          ///< Steering motor velocity limit in turns per second
    static constexpr float steeringAccelLimit = 400.0f;        ///< Steering motor acceleration limit in turns per second squared

    static constexpr float steeringPosGain = 100.0f;           ///< Position gain for the steering trapezoidal trajectory
    static constexpr float steeringVelGain = 0.05f;            ///< Velocity gain for the steering trapezoidal trajectory
    static constexpr float steeringIntegratorGain = 0.0f;      ///< Integrator gain for the steering trapezoidal trajectory

    static constexpr float steeringSoftCurrentMax = 30.0f;     ///< Current in amps for steering motor standard operation
    static constexpr float steeringHardCurrentMax = 80.0f;     ///< Current in amps for an ODrive overcurrent error in the steering motor

    static constexpr float steeringMechanicalThreshold = -100; ///< Mechanical power threshold in Watts for the steering motor
    static constexpr float steeringElectricalThreshold = 100;  ///< Electrical power threshold in Watts for the steering motor

    static constexpr float oDriveInputLimit = 50.0f;           ///< Steering rate limit for the ODrive input

    static constexpr float steeringGearRatio = (1.0 / 20.0);   ///< Gear ratio on the steering motor
  };
}

#endif // CONTROL_CONSTANTS
