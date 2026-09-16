/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2026 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/SpanCast
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
#include "Arduino.h"
#include <vector>
#include <initializer_list>

#if defined(ARDUINO_ARCH_ESP8266)
  #include "SpanCast8266.h"
#elif defined(ARDUINO_ARCH_ESP32)
  #include "SpanCast32.h"
#else
  #error ERROR: SPANCAST IS ONLY AVAILABLE FOR ESP32 AND ESP8266 MICROCONTROLLERS!
#endif

[[maybe_unused]] static const char* DIAG_TAG = "SpanCast";

///////////////////////////////

class SpanCast {

  union SpAddress {
    struct {
      uint8_t firstByte=0xF2;
      uint8_t devID;
      uint16_t netID;
      uint16_t checkSum;
    };
    uint8_t mac[6];

    SpAddress(uint8_t deviceID, uint16_t networkID){
      devID=deviceID;
      netID=networkID;
      mac[4]=mac[0]^mac[2];
      mac[5]=mac[1]^mac[3];
    }

    boolean isValid() const {
      return(firstByte==0xF2 && mac[4]==mac[0]^mac[2] && mac[5]==mac[1]^mac[3]);
    }
  };

  public: 

  struct SpConfig_t {
    uint16_t network=1;
    String password="HomeSpan";
    boolean encrypt=true;
    uint16_t channelMask=0;
    uint8_t numTries=3;
  };

  private:

  int receiveSize;                            // size (in bytes) of messages to receive
  int sendSize;                               // size (in bytes) of messages to send
  esp_now_peer_info_t peerInfo;               // structure for all ESP-NOW peer data
  QueueHandle_t receiveQueue;                 // queue to store data after it is received
  boolean overwriteQueue;                     // flag to indicate whether receiving queue should be overridden
  uint32_t receiveTime=0;                     // time (in millis) of most recent data received
  boolean initialized=false;                  // flag to ensure object was properly initialized

  uint8_t lastMessageID[crypto_auth_BYTES+sizeof(uint32_t)];     // formed from last 4-byte random nonce and 32-byte HMAC to check for duplicate transmissions

  static MasterKey *mKey;
  static HMAC *localHMAC;
    
  static std::vector<SpanCast *> SpanCasts;

  static QueueHandle_t statusQueue;           // queue for communication between SpanCast::dataSend and SpanCast::send
  static SpAddress *deviceAddress;            // SpanCast Address of this device (will be used for AP Mac)
  static SpConfig_t spConf;                   // stores all configuration settings
  static boolean configured;                  // flag indicating SpanCast has been configured
 
  static void dataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);
  static uint8_t nextChannel(uint8_t channel);

  public:

  static uint16_t range(uint8_t start, uint8_t end){
    uint16_t mask=0;
    for(int i=start;i<=end;i++)
      mask|=(1<<i);
    return(mask);
  }

  static uint16_t list(std::initializer_list<uint8_t> list){
    uint16_t mask=0;
    for(uint8_t i : list)
      mask|=(1<<i);
    return(mask);
  }

  static boolean configure(uint8_t deviceID, SpConfig_t cfg=spConf);
  SpanCast(uint8_t deviceID, size_t sendSize, size_t receiveSize=0, size_t queueDepth=0);

  boolean send(const void *data);
  boolean get(void *dataBuf);

  uint32_t time(){return(millis()-receiveTime);}
};

