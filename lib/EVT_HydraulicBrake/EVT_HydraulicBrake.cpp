#include <EVT_HydraulicBrake.hpp>

#include <ControlConstants.hpp>
#include <ConversionConstants.hpp>
#include <IOConstants.hpp>
#include <TransmitterConstants.hpp>

#include "Mutexes.hpp"
using RTOS::MutexValues;

namespace MotorControls{

  void HydraulicBrake::setup() {
    pinMode(Constants::IOConstants::dynamicBrakePulsePin, arduino::OUTPUT);
    pinMode(Constants::IOConstants::dynamicBrakeDirPin, arduino::OUTPUT);
    pinMode(Constants::IOConstants::dynamicBrakeLimitSwitchPin, arduino::INPUT_PULLUP);

    digitalWrite(Constants::IOConstants::dynamicBrakePulsePin, arduino::LOW);
    digitalWrite(Constants::IOConstants::dynamicBrakeDirPin, arduino::LOW);

    home();
  }


  void HydraulicBrake::pulseStep() {
    digitalWrite(Constants::IOConstants::dynamicBrakePulsePin, arduino::HIGH);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeStepPulseUs * Constants::ConversionConstants::microsToSec * Constants::ConversionConstants::secToMillis));
    digitalWrite(Constants::IOConstants::dynamicBrakePulsePin, arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeStepDelayUs * Constants::ConversionConstants::microsToSec * Constants::ConversionConstants::secToMillis));
  }


  void HydraulicBrake::moveSteps(int steps, bool directionForward) {
    if (steps <= 0) {
      return;
    }

    const bool dirLevel = directionForward
      ? Constants::ControlConstants::dynamicBrakeForwardDirLevel
      : !Constants::ControlConstants::dynamicBrakeForwardDirLevel;

    digitalWrite(Constants::IOConstants::dynamicBrakeDirPin, dirLevel ? arduino::HIGH : arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeDirSetupUs * Constants::ConversionConstants::microsToSec * Constants::ConversionConstants::secToMillis));

    for (int i = 0; i < steps; i++) {
      if (!directionForward && MutexValues::hydraulicBrakeHomed) {
        currentPositionSteps = 0;
        break;
      }

      pulseStep();
      currentPositionSteps += directionForward ? 1 : -1;

      if (currentPositionSteps < 0) {
        currentPositionSteps = 0;
      }
    }
  }

  void HydraulicBrake::home() {
    digitalWrite(Constants::IOConstants::dynamicBrakeDirPin, Constants::ControlConstants::dynamicBrakeForwardDirLevel ? arduino::LOW : arduino::HIGH);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeDirSetupUs * Constants::ConversionConstants::microsToSec * Constants::ConversionConstants::secToMillis));

    for (int i = 0; i < Constants::ControlConstants::dynamicBrakeHomeMaxSteps; i++) {
      if (MutexValues::hydraulicBrakeHomed) {
        currentPositionSteps = 0;
        moveSteps(Constants::ControlConstants::dynamicBrakePedalOffsetSteps, true);

        return; // exit function
      }

      pulseStep();
    }
  }


  bool HydraulicBrake::driveMotor(float brakePercent) {
    brakePercent = std::clamp(brakePercent, 0.0f, 1.0f);

    if (!MutexValues::hydraulicBrakeHomed) {
      if (Serial) {
        Serial.println("Dynamic brake failed to home");
      }

      return false;
    }

    const int targetSteps = Constants::ControlConstants::dynamicBrakePedalOffsetSteps
      + static_cast<int>(
        brakePercent * (
          Constants::ControlConstants::dynamicBrakeMaxSteps
          - Constants::ControlConstants::dynamicBrakePedalOffsetSteps
        )
      );
    const int deltaSteps = targetSteps - currentPositionSteps;

    if (deltaSteps > 0) {
      moveSteps(deltaSteps, true);
    } else if (deltaSteps < 0) {
      moveSteps(-deltaSteps, false);
    }

    return true;
  }
}
