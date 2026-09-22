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
  
#define TEMP_HUM_DEVICE_ID      1   // SpanCast Device ID for the Temperature+Humidity Sensor
#define WIND_SPEED_DEVICE_ID    2   // SpanCast Device ID for the Wind Speed Sensor
#define BAROMETER_DEVICE_ID     3   // SpanCast Device ID for the Barometric Pressure Sensor

#define UPDATE_FREQUENCY     8000   // how frequently (in millseconds) for this device to send out updates

//////////////////////

void setup() {

  Serial.begin(115200);
  delay(1000);

  struct {
    float temperature;
    uint8_t humidity;
  } tempHum;

  uint8_t windSpeed=10;
  char barometerMessage[64];

  Serial.printf("\n\nWind Speed Sensor Ready.\n\n");

  SpanCast::configure(WIND_SPEED_DEVICE_ID,{.encrypt=false,.channelMask=SpanCast::list({5})});

  WiFi.enableSTA(true);


  SpanCast tempHumSensor(TEMP_HUM_DEVICE_ID,sizeof(windSpeed),sizeof(tempHum),{.encrypt=true});
  SpanCast barometerSensor(BAROMETER_DEVICE_ID,sizeof(windSpeed),sizeof(barometerMessage));

  uint32_t updateTime=0;

  while(1){

    if(millis()-updateTime > UPDATE_FREQUENCY){

      windSpeed++;
      if(windSpeed>30)
        windSpeed=10;

      Serial.printf("Sending to Temp/Hum Sensor -> Lastest Wind Speed Reading ... ");

      if(tempHumSensor.send(&windSpeed))
        Serial.printf("Succeeded\n");
      else
        Serial.printf("Failed\n");

      Serial.printf("Sending to Barometer Sensor -> Lastest Wind Speed Reading ... ");

      if(barometerSensor.send(&windSpeed))
        Serial.printf("Succeeded\n");
      else
        Serial.printf("Failed\n");

      Serial.printf("\n---------------------------\n");
      if(tempHumSensor.isActive()){
        Serial.printf("Temperature:  %0.1fC\n",tempHum.temperature);
        Serial.printf("Humidity:     %d%%\n",tempHum.humidity);
      } else {
        Serial.printf("Temperature:  ---\n");
        Serial.printf("Humidity:     ---\n");
      }
      Serial.printf("Wind Speed:   %d km/hr\n",windSpeed);
      if(barometerSensor.isActive())
        Serial.printf("Barometer:    %s\n",barometerMessage);
      else
        Serial.printf("Barometer:    ---\n");
      Serial.printf("---------------------------\n\n");

      updateTime=millis();        
    }

    if(tempHumSensor.get(&tempHum))
      Serial.printf("Received from Temp/Hum Sensor: Temp=%0.1f, Hum=%d\n",tempHum.temperature,tempHum.humidity);

    if(barometerSensor.get(barometerMessage))
      Serial.printf("Received from Barometer Sensor: Bar=%s\n",barometerMessage);

    delay(1);
  }
}

//////////////////////

void loop() {
}

//////////////////////
