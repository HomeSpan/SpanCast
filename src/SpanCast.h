
#include "Arduino.h"
#include <vector>

#define LOG2(format,...) Serial.print ##__VA_OPT__(f)(format __VA_OPT__(,) __VA_ARGS__);

#if defined(ARDUINO_ARCH_ESP8266)
  #include "SpanCast8266.h"
#elif defined(ARDUINO_ARCH_ESP32)
  #include "SpanCast32.h"
#else
  #error ERROR: SPANCAST IS ONLY AVAILABLE FOR ESP32 AND ESP8266 MICROCONTROLLERS!
#endif

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

  struct SpConfig_t {
    uint16_t network=1;
    String password="HomeSpan";
    boolean encrypt=true;
    uint16_t channelMask=0x0FFE;
  };

  int receiveSize;                            // size (in bytes) of messages to receive
  int sendSize;                               // size (in bytes) of messages to send
  esp_now_peer_info_t peerInfo;               // structure for all ESP-NOW peer data
  QueueHandle_t receiveQueue;                 // queue to store data after it is received
  boolean overwriteQueue;                     // flag to indicate whether receiving queue should be overridden
  uint32_t receiveTime=0;                     // time (in millis) of most recent data received

  static MasterKey *mKey;
  static HMAC *localHMAC;
//  static nvs_handle pointNVS;                 // NVS storage for channel number (only used for remote devices)
    
  static std::vector<SpanCast *> SpanCasts;

  static QueueHandle_t statusQueue;           // queue for communication between SpanCast::dataSend and SpanCast::send
  static SpAddress *deviceAddress;            // SpanCast Address of this device (will be used for AP Mac)
  static SpConfig_t spConf;                   // stores all configuration settings
  static boolean configured;                  // flag indicating SpanCast has been configured
 
  static void dataReceived(const uint8_t *mac, const uint8_t *incomingData, int len);
  static uint8_t nextChannel(uint8_t channel);
  static void initializeChannels();
 
  public:

  static void configure(uint8_t deviceID, SpConfig_t cfg=spConf);
  SpanCast(uint8_t deviceID, size_t sendSize, size_t receiveSize=0, size_t queueDepth=0);

  boolean send(const void *data);
  boolean get(void *dataBuf);

  uint32_t time(){return(millis()-receiveTime);}
};

