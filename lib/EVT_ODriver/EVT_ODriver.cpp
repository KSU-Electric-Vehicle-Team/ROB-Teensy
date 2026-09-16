#include <EVT_ODriver.hpp>

namespace MotorControls {
  ODriver::ODriver(ODriveUART & odrv) : oDrive(odrv) {
    absCenterPos = 0.0f;
    calibrationFlag = false;
  }


  bool ODriver::setup() {
    TickType_t setupStart = xTaskGetTickCount();

    while ((oDrive.getState() == ODriveAxisState::AXIS_STATE_UNDEFINED) && (xTaskGetTickCount() - setupStart < pdMS_TO_TICKS(15'000))) {
      vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (oDrive.getState() == ODriveAxisState::AXIS_STATE_IDLE) {
      snprintf(
        oDrivePrintBuffer,
        sizeof(oDrivePrintBuffer),
        "Found ODrive"
      );
    } else {
      snprintf(
        oDrivePrintBuffer,
        sizeof(oDrivePrintBuffer),
        "Unable to find ODrive"
      );
    }

    Queues::logWrite(oDrivePrintBuffer);

    return oDrive.getState() == ODriveAxisState::AXIS_STATE_IDLE;
  }


  bool ODriver::calibrate() {
    oDrive.setState(ODriveAxisState::AXIS_STATE_FULL_CALIBRATION_SEQUENCE);
    waitUntilState(ODriveAxisState::AXIS_STATE_IDLE);

    oDrive.clearErrors();

    snprintf(
      oDrivePrintBuffer,
      sizeof(oDrivePrintBuffer),
      "You have %0.1f seconds to center the ODrive",
      centeringTime
    );

    Queues::logWrite(oDrivePrintBuffer);
    vTaskDelay(pdMS_TO_TICKS(centeringTime * ConversionConstants::secToMillis));

    data.lastFeedback = oDrive.getFeedback();
    absCenterPos = data.lastFeedback.pos;

    oDrive.clearErrors();
    oDrive.setState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

    waitUntilState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

    calibrationFlag = true;
    return calibrationFlag;
  }


  void ODriver::update(float targetValue, bool rateLimit) {
    targetPosition = rateLimit ? inputLimiter.calculate(targetValue) : targetValue;
    oDrive.trapezoidalMove(targetPosition);
  }


  void ODriver::reset() {
    oDrive.clearErrors();
    calibrationFlag = false;
  }


  void ODriver::waitUntilState(ODriveAxisState state, TickType_t timeout) {
    TickType_t waitStart = xTaskGetTickCount();

    while ((oDrive.getState() != state) && (xTaskGetTickCount() - waitStart < timeout)) {
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }


  bool ODriver::getCalibrationFlag() const {
    return calibrationFlag;
  }


  float ODriver::getCenterPosition() const {
    return absCenterPos;
  }


  float ODriver::getTemperature() const {
    return oDrive.getParameterAsFloat("axis0.motor.motor_thermistor.temperature");
  }


  ODriveError ODriver::getDisarmReason() {
    data.lastError = (ODriveError)oDrive.getParameterAsInt("axis0.disarm_reason");

    snprintf(
      errorBuffer,
      sizeof(errorBuffer),
      "Error code [0x%X] on ODrive",
      data.lastError
    );

    Queues::errorWrite(errorBuffer);

    return data.lastError;
  }
}
