#include <EVT_ODriveCAN.hpp>

namespace MotorControls {
  ODriver::ODriver(ODriveCAN & odrv, uint8_t node) : oDrive(odrv), nodeID(node) {
    oDrives[nodeID] = &odrv;

    odrv.onStatus(onHeartbeat, &nodeData);
    odrv.onFeedback(onFeedback, &nodeData);
    odrv.onError(onError, &nodeData);
    odrv.onBusVI(onVoltageCurrent, &nodeData);
    odrv.onTemperature(onTemperature, &nodeData);

    absCenterPos = 0.0f;
    calibrationFlag = false;
  }


  bool ODriver::setup() {
    TickType_t setupStart = xTaskGetTickCount();

    while ((!nodeData.receivedHeartbeat) && (xTaskGetTickCount() - setupStart < pdMS_TO_TICKS(15'000))) {
      pumpEvents(MutexValues::canBus);
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (nodeData.receivedHeartbeat) {
      snprintf(
        oDrivePrintBuffer,
        sizeof(oDrivePrintBuffer),
        nodeID >= 10 ? "Found ODrive on node [%u]" : "Found ODrive on node [0%u]",
        nodeID
      );
    } else {
      snprintf(
        oDrivePrintBuffer,
        sizeof(oDrivePrintBuffer),
        nodeID >= 10 ? "Unable to find ODrive on node [%u]" : "Unable to find ODrive on node [0%u]",
        nodeID
      );
    }

    Queues::logWrite(oDrivePrintBuffer);

    return nodeData.receivedHeartbeat;
  }


  bool ODriver::calibrate() {
    oDrive.setState(ODriveAxisState::AXIS_STATE_FULL_CALIBRATION_SEQUENCE);
    waitUntilState(ODriveAxisState::AXIS_STATE_IDLE);

    oDrive.clearErrors();

    sniprintf(
      oDrivePrintBuffer,
      sizeof(oDrivePrintBuffer),
      nodeID >= 10 ? "You have %0.1f seconds to center ODrive[%u]" : "You have %0.1f seconds to center ODrive[0%u]",
      centeringTime,
      nodeID
    );

    Queues::logWrite(oDrivePrintBuffer);
    vTaskDelay(pdMS_TO_TICKS(centeringTime * ConversionConstants::secToMillis));

    absCenterPos = nodeData.lastFeedback.Pos_Estimate;
    nodeData.receivedFeedback = false;

    oDrive.clearErrors();
    oDrive.setState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

    waitUntilState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

    calibrationFlag = nodeData.lastHeartbeat.Procedure_Result == ODriveProcedureResult::PROCEDURE_RESULT_SUCCESS;
    nodeData.receivedHeartbeat = false;
    return calibrationFlag;
  }


  void ODriver::update(float targetValue, bool rateLimit) {
    pumpEvents(MutexValues::canBus);

    targetPosition = rateLimit ? inputLimiter.calculate(targetValue) : targetValue;
    oDrive.setPosition(targetPosition);
  }


  void ODriver::reset() {
    oDrive.clearErrors();
    calibrationFlag = false;
  }


  void ODriver::waitUntilState(ODriveAxisState state, TickType_t timeout) {
    TickType_t waitStart = xTaskGetTickCount();

    while ((nodeData.lastHeartbeat.Axis_State != state) && (xTaskGetTickCount() - waitStart < timeout)) {
      nodeData.receivedHeartbeat = false;
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }


  void ODriver::setInputMode(ODriveInputMode mode) {
    oDrive.setControllerMode(ODriveControlMode::CONTROL_MODE_POSITION_CONTROL, mode);
  }


  bool ODriver::getCalibrationFlag() const {
    return calibrationFlag;
  }


  float ODriver::getCenterPosition() const {
    return absCenterPos;
  }


  float ODriver::getTemperature() {
    nodeData.receivedTemp = false;
    return nodeData.lastTemperature.FET_Temperature;
  }


  ODriveError ODriver::getDisarmReason() const {
    snprintf(
      errorBuffer,
      sizeof(errorBuffer),
      nodeID >= 10 ? "Error code [0x%X] on ODrive[%u]" : "Error code [0x%X] on ODrive[0%u]",
      (ODriveError)nodeData.lastError.Disarm_Reason,
      nodeID
    );

    Queues::errorWrite(errorBuffer);

    return (ODriveError)nodeData.lastError.Disarm_Reason;
  }


  std::map<uint8_t, ODriveCAN*> ODriver::getODrives() {
    return oDrives;
  }


  void ODriver::onHeartbeat(Heartbeat_msg_t & msg, void * userData) {
    ODriveUserData * data = static_cast<ODriveUserData*>(userData);
    data->lastHeartbeat = msg;
    data->receivedHeartbeat = true;
  }


  void ODriver::onFeedback(Get_Encoder_Estimates_msg_t & msg, void * userData) {
    ODriveUserData * data = static_cast<ODriveUserData*>(userData);
    data->lastFeedback = msg;
    data->receivedFeedback = true;
  }


  void ODriver::onError(Get_Error_msg_t & err, void * userData) {
    ODriveUserData * data = static_cast<ODriveUserData*>(userData);
    data->lastError = err;
    data->receivedError = true;
  }


  void ODriver::onVoltageCurrent(Get_Bus_Voltage_Current_msg_t & VI, void * userData) {
    ODriveUserData * data = static_cast<ODriveUserData*>(userData);
    data->lastVoltageCurrent = VI;
    data->receivedVI = true;
  }


  void ODriver::onTemperature(Get_Temperature_msg_t & temp, void * userData) {
    ODriveUserData * data = static_cast<ODriveUserData *>(userData);
    data->lastTemperature = temp;
    data->receivedTemp = true;
  }
}