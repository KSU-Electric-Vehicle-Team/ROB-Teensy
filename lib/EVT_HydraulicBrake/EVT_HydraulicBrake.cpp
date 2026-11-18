#include <EVT_HydraulicBrake.hpp>

#include <ControlConstants.hpp>
#include <IOConstants.hpp>
#include <TransmitterConstants.hpp>

namespace MotorControls{

  void HydraulicBrake::setup() {
    pinMode(Constants::IOConstants::dynamicBrakePulsePin, arduino::OUTPUT);
    pinMode(Constants::IOConstants::dynamicBrakeDirPin, arduino::OUTPUT);
    pinMode(Constants::IOConstants::dynamicBrakeLimitSwitchPin, arduino::INPUT_PULLUP);

    digitalWrite(Constants::IOConstants::dynamicBrakePulsePin, arduino::LOW);
    digitalWrite(Constants::IOConstants::dynamicBrakeDirPin, arduino::LOW);
  }


  void HydraulicBrake::pulseStep() {
    digitalWrite(Constants::IOConstants::dynamicBrakePulsePin, arduino::HIGH);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeStepPulseMs));
    digitalWrite(Constants::IOConstants::dynamicBrakePulsePin, arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeStepDelayMs));
  }


  void HydraulicBrake::stepperMotor(int steps, bool directionForward) {
    if (steps <= 0) {
      return;
    }

    const bool dirLevel = directionForward
      ? Constants::ControlConstants::dynamicBrakeForwardDirLevel
      : !Constants::ControlConstants::dynamicBrakeForwardDirLevel;

    digitalWrite(Constants::IOConstants::dynamicBrakeDirPin, dirLevel ? arduino::HIGH : arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeDirSetupMs));

    for (int i = 0; i < steps; i++) {
      if (!directionForward && isLimitHit()) {
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


  bool HydraulicBrake::isLimitHit() const {
    const int state = digitalRead(Constants::IOConstants::dynamicBrakeLimitSwitchPin);
    return Constants::ControlConstants::dynamicBrakeLimitActiveLow ? state == arduino::LOW : state == arduino::HIGH;
  }


  bool HydraulicBrake::home() {
    digitalWrite(Constants::IOConstants::dynamicBrakeDirPin, Constants::ControlConstants::dynamicBrakeForwardDirLevel ? arduino::LOW : arduino::HIGH);
    vTaskDelay(pdMS_TO_TICKS(Constants::ControlConstants::dynamicBrakeDirSetupMs));

    for (int i = 0; i < Constants::ControlConstants::dynamicBrakeHomeMaxSteps; i++) {
      if (isLimitHit()) {
        currentPositionSteps = 0;
        stepperMotor(Constants::ControlConstants::dynamicBrakePedalOffsetSteps, true);
        homed = true;
        return true;
      }

      pulseStep();
    }

    homed = false;
    return false;
  }


  bool HydraulicBrake::driveMotor(float brakePercent) {
    brakePercent = std::clamp(brakePercent, 0.0f, 1.0f);

    if (!homed && !home()) {
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
      stepperMotor(deltaSteps, true);
    } else if (deltaSteps < 0) {
      stepperMotor(-deltaSteps, false);
    }

    return true;
  }
}
