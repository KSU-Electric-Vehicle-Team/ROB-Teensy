#ifndef CONVERSION_CONSTANTS
#define CONVERSION_CONSTANTS

/*-----------------------------------------------------------------------------*/
/**
 * @file   ConversionConstants.hpp
 * @brief  Header for ConversionConstants struct
 *
 * The ConversionConstants struct is used for the definition of various constants
 * that are used for converting between units. For example, this file contains
 * conversion factors for degrees to radians, seconds to milliseconds, and more
 *
 * @author Lilia Turbeville
 * @date   March 18, 2026
*//*---------------------------------------------------------------------------*/

#include "arduino_freertos.h"

namespace Constants {
  /**
   * @brief Constants used for various conversion ratio
   */
  struct ConversionConstants {
    static constexpr double secToMillis = 1'000.0;           ///< Conversion factor for seconds to milliseconds
    static constexpr double millisToSec = (1.0 / 1'000.0);   ///< Conversion factor for milliseconds to seconds

    static constexpr double degToRad = (PI / 180.0);         ///< Conversion factor for degrees to radians
    static constexpr double radToDeg = (180.0 / PI);         ///< Conversion factor for radians to degrees

    static constexpr double radToTurns = (1.0 / (2.0 * PI)); ///< Conversion factor for radians to turns
    static constexpr double turnsToRad = (2.0 * PI);         ///< Conversion factor for turns to radians

    static constexpr double degToTurns = (1.0 / 360.0);      ///< Conversion factor for degrees to turns
    static constexpr double turnsToDeg = 360.0;              ///< Conversion factor for turns to degrees
  };
}

#endif // CONVERSION_CONSTANTS
