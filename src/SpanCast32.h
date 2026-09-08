
#if !(defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C5))
  #error ERROR: SELECTED MICROCONTROLLER NOT SUPPORTED. SPANCAST FOR THE ESP32 SUPPORTS THE FOLLOWING CHIPS: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C5, AND ESP32-C6
#endif

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <sodium.h>

///////////////////////////////

class MasterKey {

  private:
  
  uint8_t masterKey[crypto_kdf_hkdf_sha256_KEYBYTES];

  public:

  MasterKey(const char *password, const char *salt){

    crypto_kdf_hkdf_sha256_extract(masterKey,(unsigned char *)salt,strlen(salt),(unsigned char *)password,strlen(password));
  }

  void create(const char *keyInfo, uint8_t *newKey, size_t newKeyLen){

    create(keyInfo,strlen(keyInfo),newKey,newKeyLen);   
  }

  void create(const char *keyInfo, size_t keyInfoLen, uint8_t *newKey, size_t newKeyLen){

    crypto_kdf_hkdf_sha256_expand(newKey,newKeyLen,keyInfo,keyInfoLen,masterKey);
  }  
};

///////////////////////////////

class HMAC {

  private:

  uint8_t authKey[crypto_auth_KEYBYTES];

  public:
  
  HMAC(MasterKey *masterKey, const void *keyInfo, size_t keyInfoLen){

    masterKey->create((char *)keyInfo,keyInfoLen,authKey,crypto_auth_KEYBYTES);
  }

  void create(const void *data, size_t dataLen, uint8_t *hmac){

    crypto_auth_hmacsha256(hmac, (unsigned char *)data, dataLen, authKey);
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


