
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
 
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <bearssl/bearssl.h>

///////////////////////////////

#define pdMS_TO_TICKS(N) (N)

static const int ESP_NOW_KEY_LEN          = 16;
static const int crypto_auth_BYTES        = 32;
static const int ESP_NOW_MAX_DATA_LEN_V2  = 250;

enum {
  ESP_NOW_SEND_SUCCESS,
  ESP_NOW_SEND_FAIL
};

struct esp_now_peer_info_t {
  uint8_t peer_addr[6];
};

using esp_now_send_status_t = uint8_t;

#define esp_wifi_get_country(country)  (wifi_get_country(country))
#define WIFI_SECOND_CHAN_NONE 0
#define esp_wifi_set_channel(channel,channel2)  {wifi_promiscuous_enable(true);wifi_set_channel(channel);wifi_promiscuous_enable(false);}
#define esp_random() (RANDOM_REG32)

#ifdef DEBUG_ESP_WIFI
  #define ESP_LOGE(TAG,format,...) {Serial.printf("[%6d][E][%s:%d] %s(): [%s] ",millis(),__FILE__,__LINE__,__FUNCTION__,TAG);Serial.printf(format "\n" __VA_OPT__(,) __VA_ARGS__);}
  #define ESP_LOGW(TAG,format,...) {Serial.printf("[%6d][W][%s:%d] %s(): [%s] ",millis(),__FILE__,__LINE__,__FUNCTION__,TAG);Serial.printf(format "\n" __VA_OPT__(,) __VA_ARGS__);}
  #define ESP_LOGI(TAG,format,...) {Serial.printf("[%6d][I][%s:%d] %s(): [%s] ",millis(),__FILE__,__LINE__,__FUNCTION__,TAG);Serial.printf(format "\n" __VA_OPT__(,) __VA_ARGS__);}
  #define ESP_LOGD(TAG,format,...) {Serial.printf("[%6d][D][%s:%d] %s(): [%s] ",millis(),__FILE__,__LINE__,__FUNCTION__,TAG);Serial.printf(format "\n" __VA_OPT__(,) __VA_ARGS__);}
  #define ESP_LOGV(TAG,format,...) {Serial.printf("[%6d][V][%s:%d] %s(): [%s] ",millis(),__FILE__,__LINE__,__FUNCTION__,TAG);Serial.printf(format "\n" __VA_OPT__(,) __VA_ARGS__);}
#else
  #define ESP_LOGE(TAG,format,...) { (void)0; }
  #define ESP_LOGW(TAG,format,...) { (void)0; }
  #define ESP_LOGI(TAG,format,...) { (void)0; }
  #define ESP_LOGD(TAG,format,...) { (void)0; }
  #define ESP_LOGV(TAG,format,...) { (void)0; }
#endif

///////////////////////////////

class MasterKey {

  private:
  
  br_hkdf_context masterContext;

  public:

  MasterKey(const char *password, const char *salt){

	  br_hkdf_init(&masterContext, &br_sha256_vtable, salt, strlen(salt));
    br_hkdf_inject(&masterContext, password, strlen(password));
    br_hkdf_flip(&masterContext);
  }

  void create(const char *keyInfo, uint8_t *newKey, size_t newKeyLen){

    create(keyInfo,strlen(keyInfo),newKey,newKeyLen);   
  }

  void create(const void *keyInfo, size_t keyInfoLen, uint8_t *newKey, size_t newKeyLen){
  
    br_hkdf_context tempContext=masterContext;  
    br_hkdf_produce(&tempContext, keyInfo, keyInfoLen, newKey, newKeyLen);    
  }  
};

///////////////////////////////

class HMAC {

  private:

  br_hmac_key_context kc;

  public:
  
  HMAC(MasterKey *masterKey, const void *keyInfo, size_t keyInfoLen){

    uint8_t authKey[32];
    masterKey->create(keyInfo,keyInfoLen,authKey,32);
    br_hmac_key_init(&kc, &br_sha256_vtable, authKey, 32);
  }

  void create(const void *data, size_t dataLen, uint8_t *hmac){

    br_hmac_context mc;
    br_hmac_init(&mc, &kc, 32);
    br_hmac_update(&mc, data, dataLen);
    br_hmac_out(&mc, hmac);
  }

  boolean verify(const uint8_t *data,  size_t dataLen){

    if(dataLen<33)
      return(false);
    
    dataLen-=32;
    uint8_t hmac[32];
    create(data,dataLen,hmac);
    return(memcmp(data+dataLen,hmac,32)==0);
  }
};

///////////////////////////////

class SimpleQueue {

  size_t depth;
  size_t nBytes;
  uint8_t **queue;
  volatile int index=0;
  volatile int nEntries=0;
  
  public:

  SimpleQueue(size_t depth, size_t nBytes){

    this->depth=depth;
    this->nBytes=nBytes;
    
    queue=(uint8_t **)calloc(depth,sizeof(uint8_t *));
    for(int i=0;i<depth;i++)
      queue[i]=(uint8_t *)calloc(nBytes,sizeof(uint8_t));
  }

  boolean send(const void *data, boolean overWrite){

    if(nEntries<depth){
      memcpy(queue[index],data,nBytes);
      nEntries++;
      index=(index+1)%depth;
      return(true);
    } else if(depth==1 && overWrite) {
      memcpy(queue[index],data,nBytes);
      return(true);
    }

    return(false);
  }

  boolean receive(void *data, uint32_t waitTime){

    uint32_t t=millis();

    while(nEntries==0 && millis()-t<waitTime)
      delay(1);

    if(nEntries==0)
      return(false);

    memcpy(data,queue[(index-nEntries+depth)%depth],nBytes);
    nEntries--;

    return(true);
  }
};

using QueueHandle_t = SimpleQueue*;

#define xQueueCreate(depth, nBytes) new SimpleQueue(depth, nBytes)
#define xQueueSend(queue, data, unused_waitTime) queue->send(data,false)
#define xQueueOverwrite(queue, data) queue->send(data,true)
#define xQueueReceive(queue, data, waitTime) queue->receive(data,waitTime)

