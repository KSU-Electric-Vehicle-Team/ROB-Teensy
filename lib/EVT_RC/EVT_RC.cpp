#include <EVT_RC.hpp>

namespace Signals {
  ControlRC::ControlRC() {
    for (int i = 0; i < TransmitterConstants::numChannels; i++) {
      channelVal[i] = TransmitterConstants::midRC;
    }

    sbusRX.Begin();
  }


  void ControlRC::setMapping(const uint16_t mapArray[2], mapType mappingType) {
    switch (mappingType) {
      case (mapType::JOYSTICK):
        joystickMap[0] = mapArray[0];
        joystickMap[1] = mapArray[1];
        
        break;
      case (mapType::SWITCH):
        switchMap[0] = mapArray[0];
        switchMap[1] = mapArray[1];

        break;
      case (mapType::TRI_SWITCH):
        triSwitchMap[0] = mapArray[0];
        triSwitchMap[1] = mapArray[1];
        triSwitchMap[2] = mapArray[2];

        break;
      case (mapType::KNOB):
        knobMap[0] = mapArray[0];
        knobMap[1] = mapArray[1];

        break;
    }
  }


  bool ControlRC::update() {
    if (sbusRX.Read()) {
      data = sbusRX.data();

      if (data.failsafe) {
        hasValidFrame = false;
        return false;
      }

      if (!data.lost_frame) {
        for (int i = 0; i < TransmitterConstants::numChannels; i++) {
          channelVal[i] = data.ch[i];
        }

        hasValidFrame = true;
        lastValidFrame = millis();
        return true;
      }
    }

    return hasValidFrame && ((millis() - lastValidFrame) <= validFrameTimeout);
  } 


  void ControlRC::printChannel(ChannelRC channel, bool isMapped) {
    snprintf(
      channelPrint,
      sizeof(channelPrint),
      "Ch[%s%u] - %u",
      int(channel + 1) < 10 ? "0" : "",
      int(channel + 1),
      getChannelValue(channel, isMapped)
    );

    Queues::logWrite(channelPrint);
  }


  uint16_t ControlRC::getChannelValue(ChannelRC channel, bool mapChannel) {
    if (mapChannel) {
      switch (channel) {
        case (ChannelRC::LEFT_X):
        case (ChannelRC::RIGHT_X):
        case (ChannelRC::RIGHT_Y):
        case (ChannelRC::LEFT_Y):
          return map(
            channelVal[channel], 
            TransmitterConstants::minRC, 
            TransmitterConstants::maxRC, 
            joystickMap[0], 
            joystickMap[1]
          );
        case (ChannelRC::SWA):
        case (ChannelRC::SWD):
        case (ChannelRC::SWF):
        case (ChannelRC::SWH):
          return map(
            channelVal[channel], 
            TransmitterConstants::minRC, 
            TransmitterConstants::maxRC, 
            switchMap[0], 
            switchMap[1]
          );
        case (ChannelRC::SWB):
        case (ChannelRC::SWC):
        case (ChannelRC::SWE):
        case (ChannelRC::SWG):
          if (channelVal[channel] == TransmitterConstants::minRC) {
            return triSwitchMap[0];
          } else if (channelVal[channel] == (TransmitterConstants::midRC)) {
            return triSwitchMap[1];
          } else {
            return triSwitchMap[2];
          }
        case (ChannelRC::VRA):
        case (ChannelRC::VRB):
        case (ChannelRC::VRC):
        case (ChannelRC::VRD):
          return map(
            channelVal[channel], 
            TransmitterConstants::minRC, 
            TransmitterConstants::maxRC, 
            knobMap[0], 
            knobMap[1]
          );
        default:
          return channelVal[channel];
      }
    } 
    
    return channelVal[channel];
  }


  SbusData ControlRC::getData() const {
    return data;
  }


  bool ControlRC::mapSwitches(uint16_t val) {
    return (val >= (0.90 * TransmitterConstants::maxRC));
  }


  template <class T>
  T ControlRC::mapToPercent(T value, const T (&mapArray)[2]) {
    return map(
      value,
      mapArray[0],
      mapArray[1],
      0,
      100
    );
  }
}