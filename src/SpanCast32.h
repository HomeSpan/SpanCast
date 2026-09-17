
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


