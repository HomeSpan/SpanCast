
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
 
#include "SpanCast.h"
#include <EEPROM.h>

///////////////////////////////

boolean SpanCast::configure(uint8_t deviceID, SpConfig_t cfg){

  if(configured){
    ESP_LOGW(DIAG_TAG,"Duplicate call to configure(%hhu,...) ignored",deviceID);
    return(false);
  }
  
  spConf=cfg;                       // save config data  
  if(spConf.numTries==0)            // if numTries was set to 0, reset it to 1
    spConf.numTries=1;

  #ifdef ARDUINO_ARCH_ESP32
    WiFi.mode(WIFI_AP_STA);
  #else
    WiFi.mode(WIFI_STA);
  #endif

  delay(10);
  deviceAddress = new SpAddress(deviceID,spConf.network);

  #ifdef ARDUINO_ARCH_ESP32
    esp_wifi_set_mac(WIFI_IF_AP, deviceAddress->mac);
  #else
    wifi_set_macaddr(STATION_IF, deviceAddress->mac);
  #endif

  esp_now_init();

  mKey = new MasterKey(spConf.password.c_str(),"SpanCast");

  uint8_t pmk[ESP_NOW_KEY_LEN];
  mKey->create("Key for PMK",pmk,ESP_NOW_KEY_LEN); 

  #ifdef ARDUINO_ARCH_ESP32
    esp_now_set_pmk(pmk);
    esp_now_register_recv_cb([](const esp_now_recv_info *info, const uint8_t *incomingData, int len){
      dataReceived(info->src_addr, incomingData, len);
    });
    esp_now_register_send_cb([](const esp_now_send_info_t *mac, esp_now_send_status_t status){    // create callback for sending data
     xQueueOverwrite(statusQueue, &status );
    });
  #else
    esp_now_set_kok(pmk,ESP_NOW_KEY_LEN);
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *incomingData, uint8_t len){
      dataReceived(mac, incomingData, len);
    });
    esp_now_register_send_cb([](uint8_t *mac, esp_now_send_status_t status){    // create callback for sending data
    xQueueOverwrite(statusQueue, &status);
    });
  #endif

  statusQueue = xQueueCreate(1,sizeof(esp_now_send_status_t));    // create statusQueue even if not needed
  localHMAC = new HMAC(mKey,deviceAddress->mac,6);                // create authentication key for the MAC of this device

  wifi_country_t country;
  esp_wifi_get_country(&country);
  spConf.channelMask=spConf.channelMask & ((1<<country.nchan)-1)<<country.schan;     // overlay country-specific mask (e.g. channels 1-11, 1-13, or 1-14 only)  

  EEPROM.begin(1);                                            // read last-saved channel using EEPROM library since it works with both ESP32 and ESP8266
  uint8_t channel=EEPROM.read(0);

  for(int i=0;i<16;i++,channel=(channel+1)%16){               // loop over all mask bits (starting with saved channel)
    if(spConf.channelMask & (1<<channel)){                    // if channel is allowed by channel mask
      esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);   // set the WiFi channel
      if(i>0){                                                // updated saved channel if needed
        EEPROM.write(0,channel);
        EEPROM.commit();
      }
      break;                                                  // break out of loop
    }
  }

  ESP_LOGI(DIAG_TAG,"Configured as DeviceID=%hhu / NetworkID=%hu / ChannelMask=0x%04X.  MAC=%02X:%02X:%02X:%02X:%02X%:%02X.  Initial Channel=%hhu",
          deviceAddress->devID,deviceAddress->netID,spConf.channelMask,
          deviceAddress->mac[0],deviceAddress->mac[1],deviceAddress->mac[2],deviceAddress->mac[3],deviceAddress->mac[4],deviceAddress->mac[5],WiFi.channel());

  configured=true;                                                // set configured to true
  return(true);
}

///////////////////////////////

SpanCast::SpanCast(uint8_t deviceID, size_t sendSize, size_t receiveSize, SpCast_t settings){

  SpAddress destAddress(deviceID, deviceAddress->netID);
  memcpy(peerInfo.peer_addr,destAddress.mac,6);
  
  if(!configured){
    ESP_LOGE(DIAG_TAG,"Can't initialize new SpanCast(%d,%d,%d...) object - SpanCast not yet configured",deviceID,sendSize,receiveSize);
    return;
  }

  if(deviceID==deviceAddress->devID || esp_now_is_peer_exist(destAddress.mac)){
    ESP_LOGE(DIAG_TAG,"Can't initialize new SpanCast(%d,%d,%d...) object - deviceID already used",deviceID,sendSize,receiveSize);
    return;
  }

  if(sendSize>MAX_MESSAGE_SIZE || receiveSize>MAX_MESSAGE_SIZE){
    ESP_LOGE(DIAG_TAG,"Can't initialize new SpanCast(%d,%d,%d) object - either send or receive size exceeds %d-byte maximum",deviceID,sendSize,receiveSize,MAX_MESSAGE_SIZE);
    return;
  }

  if(sendSize==0 && receiveSize==0){
    ESP_LOGE(DIAG_TAG,"Can't initialize new SpanCast(%d,%d,%d...) object - both send and receive size are zero",deviceID,sendSize,receiveSize);
    return;
  }  
  
  this->sendSize=sendSize;
  this->receiveSize=receiveSize;

  spCast=settings;                        // save settings data
  spCast.encrypt|=spConf.encrypt;         // overlay class-level encryption requirement

  if(spCast.encrypt || sendSize>0) {      // if encryption is on, or it's not but sending is configured, create a peer
    uint8_t lmk[ESP_NOW_KEY_LEN];
    char *keyContext;
    asprintf(&keyContext,"Key for LMK: NetID=%hu DevID1=%hhu DevID2=%hhu",deviceAddress->netID,
              deviceID<(deviceAddress->devID)?deviceID:deviceAddress->devID,
              deviceID>(deviceAddress->devID)?deviceID:deviceAddress->devID);

    SpanCast::mKey->create(keyContext,lmk,ESP_NOW_KEY_LEN);
    free(keyContext);

    int status;

    #ifdef ARDUINO_ARCH_ESP32
      peerInfo.channel=0;                             // 0 = matches current WiFi channel
      peerInfo.ifidx=WIFI_IF_AP;                      // specify interface as AP
      peerInfo.encrypt=spCast.encrypt;                // set encryption for this peer
      memcpy(peerInfo.lmk,lmk,ESP_NOW_KEY_LEN);       // set LMK for this peer
      status=esp_now_add_peer(&peerInfo);             // add peer to ESP-NOW
    #else
      if(spCast.encrypt)
        status=esp_now_add_peer(peerInfo.peer_addr, ESP_NOW_ROLE_COMBO, 0, lmk, ESP_NOW_KEY_LEN);
      else
        status=esp_now_add_peer(peerInfo.peer_addr, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
    #endif

    if(status!=0){
      ESP_LOGE(DIAG_TAG,"Can't initialize new SpanCast(%d,%d,%d...) object - failed to create ESP-NOW peer",deviceID,sendSize,receiveSize);
      return;
    }
  }

  if(receiveSize>0){
    receiveQueue = xQueueCreate(spCast.queueDepth>0?spCast.queueDepth:1,receiveSize);
    overwriteQueue=(spCast.queueDepth==0);
  }

  initialized=true;
  SpanCasts.push_back(this);

  ESP_LOGI(DIAG_TAG,"Initialized new SpanCast object with DeviceID=%hhu / SendSize=%d / ReceiveSize=%d / QueueDepth=%d / Encryption=%s.  MAC=%02X:%02X:%02X:%02X:%02X%:%02X",deviceID,sendSize,receiveSize,spCast.queueDepth,
          spCast.encrypt?"TRUE":"False",destAddress.mac[0],destAddress.mac[1],destAddress.mac[2],destAddress.mac[3],destAddress.mac[4],destAddress.mac[5]);
}

///////////////////////////////

boolean SpanCast::send(const void *data){

  const SpAddress *destAddress = (SpAddress *)peerInfo.peer_addr;

  if(!initialized){
    ESP_LOGE(DIAG_TAG,"Can't send to DeviceID=%hhu - SpanCast object not initialized",destAddress->devID);
    return(false);
  }

  if(sendSize==0){
    ESP_LOGE(DIAG_TAG,"Can't send to DeviceID=%hhu - SpanCast object not configured for sending",destAddress->devID);
    return(false);
  }
  
  uint8_t channel = WiFi.channel();
  uint8_t startingChannel=channel;              // set starting channel to current channel

  uint32_t nonce=esp_random();                                                   // create random 4-byte nonce to allow rejection of duplicate messages received but not acknowlegded
  size_t msgSize=sendSize+crypto_auth_BYTES+sizeof(nonce);                       // size of message with HMAC + nonce
  uint8_t *msg=(uint8_t *)malloc(msgSize);                                       // allocate new memory reflecting large size
  memcpy(msg,data,sendSize);                                                     // copy data into first part of memory block
  memcpy(msg+sendSize,&nonce,sizeof(nonce));                                     // copy nonce into second part of memory block
  localHMAC->create(msg,sendSize+sizeof(nonce),msg+sendSize+sizeof(nonce));      // create HMAC from authKey and load into third part of memory block

  esp_now_send_status_t status = ESP_NOW_SEND_FAIL;

  do {
    for(int i=0; status!=ESP_NOW_SEND_SUCCESS && i<spConf.numTries; i++){      
      esp_now_send(peerInfo.peer_addr, msg, msgSize);
      xQueueReceive(statusQueue, &status, pdMS_TO_TICKS(2000));
      ESP_LOGI(DIAG_TAG,"Sent %d bytes from DeviceID=%hhu to DeviceID=%hhu using WiFi channel %hhu - %s",sendSize,deviceAddress->devID,destAddress->devID,channel,status==ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
      delay(10);
    }    
  } while(status!=ESP_NOW_SEND_SUCCESS && (channel=nextChannel(channel))!=startingChannel);

  if(status!=ESP_NOW_SEND_SUCCESS)
    ESP_LOGW(DIAG_TAG,"DeviceID=%hhu on NetworkID=%hu unreachable",destAddress->devID,deviceAddress->netID);

  free(msg);

  return(status==ESP_NOW_SEND_SUCCESS);
}

///////////////////////////////

boolean SpanCast::get(void *dataBuf){

  if(!initialized)
    return(false);

  if(receiveSize==0)
    return(false);

  return(xQueueReceive(receiveQueue, dataBuf, 0));
}

///////////////////////////////

void SpanCast::dataReceived(const uint8_t *mac, const uint8_t *incomingData, int len){

  const SpAddress *srcAddress = (SpAddress *)mac;

  if(!srcAddress->isValid()){
    ESP_LOGW(DIAG_TAG,"Ignoring %d-byte message received from invalid SpanCast MAC Address %02X:%02X:%02X:%02X:%02X:%02X",len,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return;
  }

  HMAC remoteHMAC(SpanCast::mKey,mac,6);
  if(!remoteHMAC.verify(incomingData, len) || len<=sizeof(lastMessageID)){
    ESP_LOGW(DIAG_TAG,"Ignoring unverifiable %d-byte message received from DeviceID=%hhu",len,srcAddress->devID);
    return;
  }

  len-=sizeof(lastMessageID);

  auto it=SpanCasts.begin();
  for(;it!=SpanCasts.end() && memcmp((*it)->peerInfo.peer_addr,mac,6)!=0; it++);
  
  if(it==SpanCasts.end()){
    ESP_LOGW(DIAG_TAG,"Received %d verified bytes from DeviceID=%hhu but no matching SpanCast object to receive data",len,srcAddress->devID);
    return;
  }

  if((*it)->receiveSize==0){
    ESP_LOGW(DIAG_TAG,"Received %d verified bytes from DeviceID=%hhu but matching SpanCast object is not configured to receive data",len,srcAddress->devID);
    return;
  }

  if(len!=(*it)->receiveSize){
    ESP_LOGW(DIAG_TAG,"Received %d verified bytes from DeviceID=%hhu but matching SpanCast object expects %d bytes",len,srcAddress->devID,(*it)->receiveSize);
    return;
  }

  if(!memcmp(incomingData+len,(*it)->lastMessageID,sizeof(lastMessageID))){
    ESP_LOGI(DIAG_TAG,"Ignoring duplicate message of %d verified bytes from DeviceID=%hhu",len,srcAddress->devID);
    return;
  }

  memcpy((*it)->lastMessageID,incomingData+len,sizeof(lastMessageID));

  if( ((*it)->overwriteQueue && xQueueOverwrite((*it)->receiveQueue, incomingData)) || xQueueSend((*it)->receiveQueue, incomingData, 0) ){       // overwrite or send to queue immediately
    ESP_LOGI(DIAG_TAG,"Received %d verified bytes from DeviceID=%hhu - Queue updated",len,srcAddress->devID);        
    (*it)->receiveTime=millis();
    (*it)->active=true;
  } else {
    ESP_LOGW(DIAG_TAG,"Received %d verified bytes from DeviceID=%hhu but Queue is already full",len,srcAddress->devID);        
  }
}

///////////////////////////////

uint8_t SpanCast::nextChannel(uint8_t channel){

  // do NOT change channel if channel mask is set to zero or channel mask does not allow for any other channels

  if((spConf.channelMask==0) || spConf.channelMask==(1<<channel))
    return(channel);

  do {
    channel=(channel<13)?channel+1:1;              // advance to next channel
  } while(!(spConf.channelMask & (1<<channel)));   // until we find next valid one

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);   // set the WiFi channel
  EEPROM.write(0,channel);
  EEPROM.commit();  
     
  return(channel);  
}

///////////////////////////////

boolean SpanCast::isActive(){

  if(active && (millis()-receiveTime) > spCast.timeout)
    active=false;
  return(active);
}

///////////////////////////////

uint16_t SpanCast::range(uint8_t start, uint8_t end){
  uint16_t mask=0;
  for(int i=start;i<=end;i++)
    mask|=(1<<i);
  return(mask);
}

///////////////////////////////

uint16_t SpanCast::list(std::initializer_list<uint8_t> list){
  uint16_t mask=0;
  for(uint8_t i : list)
    mask|=(1<<i);
  return(mask);
}

///////////////////////////////

std::vector<SpanCast *> SpanCast::SpanCasts;
QueueHandle_t SpanCast::statusQueue;
SpanCast::SpAddress *SpanCast::deviceAddress=NULL;
SpanCast::SpConfig_t SpanCast::spConf;
boolean SpanCast::configured=false;
MasterKey *SpanCast::mKey;
HMAC *SpanCast::localHMAC;

///////////////////////////////


