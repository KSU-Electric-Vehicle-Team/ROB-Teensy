#include <EVT_Ethernet.hpp>

#include "Mutexes.hpp"
using RTOS::MutexValues;
using RTOS::Mutexes;

namespace Signals {
  EthernetEVT::EthernetEVT(IPAddress ipTeensy, IPAddress ipPanda, byte (&mac)[6]) : teensyIP(ipTeensy), pandaIP(ipPanda), teensyMac(mac) {}


  bool EthernetEVT::setup() {
    Ethernet.begin(teensyMac, teensyIP);
    udp.begin(teensyPort);

    return (Ethernet.hardwareStatus() != EthernetNoHardware) && (Ethernet.linkStatus() == LinkON);
  }


  void EthernetEVT::sendTelemetry(const char * buff) {
      udp.beginPacket(pandaIP, pandaPort);
      udp.write(
        reinterpret_cast<const uint8_t *>(buff),
        strlen(buff)
      );
      udp.endPacket();
  }


  void EthernetEVT::receiveTelemetry() {
    // Read the UDP data into the packet buffer
    receivedLength = udp.read(packetBuffer, sizeof(packetBuffer) - 1);

    if (receivedLength > 0) {
      char * token = strtok(packetBuffer, ",");
      int index = 0;

      while (token != nullptr) {
        switch (index) {
          case 0:
            strcpy(MutexValues::pandaPacket.state, token);
            break;
          case 1:
            MutexValues::pandaPacket.rpm = atoff(token);
            break;
          case 2:
            MutexValues::pandaPacket.oDriveTarget = atoff(token);
            break;
          case 3:
            MutexValues::pandaPacket.throttle = atoi(token);
            break;
          case 4:
            MutexValues::pandaPacket.steering = atoi(token);
            break;
          case 5:
            MutexValues::pandaPacket.driveRevolutions = atoff(token);
            break;
          case 6:
            MutexValues::pandaPacket.encoderCount = atol(token);
            break;
        }
        token = strtok(nullptr, ",");
        index++;
      }
    }

    if (receivedLength <= 0) {
      packetBuffer[0] = '\0';
    }

    packetBuffer[receivedLength] = '\0';

    // Drain leftover bytes if a packet is longer than the buffer
    while (udp.available() > 0) {
      udp.read();
    }

    // Strip newline or carraige return if a sender includes them
    for (int i = 0; i < receivedLength; i++) {
      if (packetBuffer[i] == '\r' || packetBuffer[i] == '\n') {
        packetBuffer[i] = '\0';
        break;
      }
    }
  }
}
