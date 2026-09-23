#ifndef EVT_ETHERNET
#define EVT_ETHERNET

/*-----------------------------------------------------------------------------*/
/**
 * @file   EVT_Ethernet.hpp
 * @brief  Header for EthernetEVT class
 *
 * The EthernetEVT class is used to send and recive UDP packets over Ethernet
 * with a Teensy 4.1. Specifically, this communicates between the
 *
 * @author Lilia Turbeville
 * @date   September 16, 2026
*//*---------------------------------------------------------------------------*/

#include "arduino_freertos.h"
#include <NativeEthernet.h>
#include <string>

namespace Signals {
  /**
   * @brief Class used for communications of UDP packets
   */
  class EthernetEVT {
    private:
      EthernetUDP udp;            // UDP socket used for communications

      IPAddress & teensyIP;       // IPAddress reference for the Teensy 4.1
      IPAddress & pandaIP;        // IPAddress reference for the Latte Panda Sigma

      byte (&teensyMac)[8];       // Mac address for the Teensy 4.1

      char packetBuffer[128];     // UDP packet buffer

      uint16_t receivedLength;    // Length of the received packet

      uint16_t teensyPort = 8888; // Autonomous command RX port
      uint16_t pandaPort = 5005;  // Telemetry TX port
    public:
      /**
       * @brief Define a new EthernetEVT instance given the Teensy IP, Panda IP, and Teensy Mac address
       *
       * @param ipTeensy IP address of the Teensy 4.1
       * @param ipPanda IP address of the Latte Panda Sigma
       * @param mac Mac address of the Teensy 4.1
       */
      EthernetEVT(IPAddress ipTeensy, IPAddress ipPanda, byte (&mac)[8]);


      /**
       * @brief Sets up the ethernet connections
       */
      bool setup();


      /**
       * @brief Sends the formatted telemetry packet to the Latte Panda Sigma
       *
       * @param buff Ethernet message buffer to send
       */
      void sendTelemetry(const char * buff);


      /**
       * @brief Receives the formatted telemetry packet from the Latte Panda Sigma
       */
      std::string receiveTelemetry();
  };
}

#endif //EVT_ETHERNET
