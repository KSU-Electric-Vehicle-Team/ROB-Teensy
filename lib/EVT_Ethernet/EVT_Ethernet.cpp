#include <EVT_Ethernet.hpp>

#include "Mutexes.hpp"
using RTOS::MutexValues;
using RTOS::Mutexes;

namespace Signals {
  EthernetEVT::EthernetEVT(IPAddress ipTeensy, IPAddress ipPanda, byte (&mac)[8]) : teensyIP(ipTeensy), pandaIP(ipPanda), teensyMac(mac) {}


  bool EthernetEVT::setup() {
    Ethernet.begin(teensyMac, teensyIP);
    udp.begin(teensyPort);

    return (Ethernet.hardwareStatus() != EthernetNoHardware) && (Ethernet.linkStatus() == LinkON);
  }


  void EthernetEVT::sendTelemetry(const char * buff) {
    if (xSemaphoreTake(Mutexes::telemetryMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      udp.beginPacket(pandaIP, pandaPort);
      udp.write(
        reinterpret_cast<const uint8_t *>(buff),
        strlen(buff)
      );
      udp.endPacket();
    }
  }


  std::string EthernetEVT::receiveTelemetry() {
    // Read the UDP data into the packet buffer
    receivedLength = udp.read(packetBuffer, sizeof(packetBuffer) - 1);

    if (receivedLength <= 0) {
      packetBuffer[0] = '\0';
      return std::string();
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

    return std::string(packetBuffer);
  }
}
