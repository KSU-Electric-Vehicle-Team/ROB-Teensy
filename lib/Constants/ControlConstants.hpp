#ifndef CONTROL_CONSTANTS
#define CONTROL_CONSTANTS

/*-----------------------------------------------------------------------------*/
/**
 * @file   ControlConstants.hpp
 * @brief  Header for ControlConstants struct
 *
 * The ControlConstants struct is used for the definition of various constants
 * that are used for system behaviors. For example, the limits applied to the
 * commands sent to the VESC can be found here.
 *
 * @author FluenceCodez
 * @date   October 28, 2026
*//*---------------------------------------------------------------------------*/

#include <stdint.h>

namespace Constants {
  /**
   * @brief Constants used for control of the system's behavior
   */
  struct ControlConstants {
    static constexpr float vescMaxERPM = 10'000.0f;       ///< Maximum forward ERPM command sent to the VESC
    static constexpr float vescMaxReverseERPM = 2'000.0f; ///< Maximum reverse ERPM command sent to the VESC (as a positive value)
    static constexpr float vescMinERPM = 900.0f;          ///< Smallest ERPM sent as a speed command (match VESC Tool's Speed PID Minimum ERPM)
    static constexpr float vescMaxBrake = 20.0f;          ///< Maximum brake current in amps sent to the VESC
  };
}

#endif // CONTROL_CONSTANTS
