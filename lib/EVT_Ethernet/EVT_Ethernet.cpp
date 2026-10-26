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

    Entropy.Initialize();
    MutexValues::bootId = Entropy.random();

    // fill in info
    MutexValues::info.encoder_counts_per_rev = 2400;

    return (Ethernet.hardwareStatus() != EthernetNoHardware) && (Ethernet.linkStatus() == LinkON);
  }


  void EthernetEVT::sendPacket(const uint8_t * data, size_t len) {
      udp.beginPacket(pandaIP, pandaPort);
      udp.write(
        reinterpret_cast<const uint8_t *>(data),
        len
      );
      udp.endPacket();
  }


  void EthernetEVT::receiveCommand() {
    int size;
    while ((size = udp.parsePacket()) > 0) {
      uint8_t packetBuffer[64];
      int len = udp.read(packetBuffer, sizeof(packetBuffer));

      Networking::Packet<Networking::CommandPayload> cmd;
      auto reject = checkCommand(packetBuffer, len, MutexValues::telemetry.state,
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
