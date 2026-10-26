#include <EVT_Ethernet.hpp>

#include "Mutexes.hpp"
#include "EthernetPacket.hpp"
#include "CheckCommand.hpp"
using RTOS::MutexValues;
using RTOS::Mutexes;

namespace Networking {
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


  void EthernetEVT::receiveCommand() {
    int size;
    while ((size = udp.parsePacket()) > 0) {
      uint8_t buf[64];
      int len = udp.read(buf, sizeof(buf));

      Networking::Packet<Networking::CommandPayload> cmd;
      auto reject = checkCommand(buf, len, MutexValues::telemetry.state,
                              MutexValues::telemetry.last_cmd_boot_id,
                              MutexValues::telemetry.last_cmd_sequence, cmd);  // match the real signature in CheckCommand.hpp

      if (reject == RejectReason::NONE) {
        stamp(MutexValues::telemetry.last_cmd);

        // Update command values
        if (xSemaphoreTake(Mutexes::commandMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
          MutexValues::commands.brake = cmd.payload.brake_command;
          MutexValues::commands.erpm = cmd.payload.motor_setpoint;
          MutexValues::commands.steering = cmd.payload.steering_target_rad;
          xSemaphoreGive(Mutexes::commandMutex);
        }

        MutexValues::telemetry.last_cmd_boot_id = cmd.header.boot_id;
        MutexValues::telemetry.last_cmd_sequence = cmd.header.sequence;

      } else {
        MutexValues::telemetry.cmd_reject_count++;
        MutexValues::telemetry.last_reject_reason = reject;
      }
    }
  }
}
