/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2026 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
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

#define UPDATE_FREQUENCY     5000   // how frequently (in millseconds) for this device to send out updates

//////////////////////

void setup() {

  Serial.begin(115200);
  delay(1000);

  struct {
    float temperature=-10;
    uint8_t humidity=30;
  } tempHum;

  uint8_t windSpeed;
  char barometerMessage[64];

  Serial.printf("\n\nTemperature and Humidity Sensor Ready.\n\n");

  SpanCast::configure(TEMP_HUM_DEVICE_ID,{.channelMask=SpanCast::range(1,11)});

  SpanCast windSpeedSensor(WIND_SPEED_DEVICE_ID,sizeof(tempHum),sizeof(windSpeed));
  SpanCast barometerSensor(BAROMETER_DEVICE_ID,sizeof(tempHum),sizeof(barometerMessage));

  uint32_t updateTime=0;

  while(1){

    if(millis()-updateTime > UPDATE_FREQUENCY){

      tempHum.temperature+=0.6;
      tempHum.humidity++;

      if(tempHum.temperature>30){
        tempHum.temperature=-10;
        tempHum.humidity=30;
      }

      Serial.printf("Sending to Wind Speed Sensor -> Lastest Temperature and Humidity Reading ... ");

      if(windSpeedSensor.send(&tempHum))
        Serial.printf("Succeeded\n");
      else
        Serial.printf("Failed\n");

      Serial.printf("Sending to Barometer Sensor -> Lastest Temperature and Humidity Reading ... ");

      if(barometerSensor.send(&tempHum))
        Serial.printf("Succeeded\n");
      else
        Serial.printf("Failed\n");

      Serial.printf("\n---------------------------\n");
      Serial.printf("Temperature:  %0.1fC\n",tempHum.temperature);
      Serial.printf("Humidity:     %d%%\n",tempHum.humidity);
      if(windSpeedSensor.isActive())
        Serial.printf("Wind Speed:   %d km/hr\n",windSpeed);
      else
        Serial.printf("Wind Speed:   ---\n");
      if(barometerSensor.isActive())
        Serial.printf("Barometer:    %s\n",barometerMessage);
      else
        Serial.printf("Barometer:    ---\n");
      Serial.printf("---------------------------\n\n");

      updateTime=millis();        
    }

    if(windSpeedSensor.get(&windSpeed))
      Serial.printf("Received from Wind Speed Sensor: Wind=%d\n",windSpeed);

    if(barometerSensor.get(barometerMessage))
      Serial.printf("Received from Barometer Sensor: Bar=%s\n",barometerMessage);

    delay(1);
  }
}

//////////////////////

void loop() {
}

//////////////////////

