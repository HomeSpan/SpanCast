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

//////////////////////

void setup() {

  Serial.begin(115200);
  delay(1000);

  struct {
    float temperature=0;
    uint8_t humidity=0;
  } tempHum;

  uint8_t windSpeed=10;
  char barometerMessage[64]="No Reading";

  Serial.printf("\n\nWind Speed Sensor Ready.\n\n");

  SpanCast::configure(WIND_SPEED_DEVICE_ID,{.channelMask=SpanCast::list({5})});

  SpanCast tempHumSensor(TEMP_HUM_DEVICE_ID,sizeof(windSpeed),sizeof(tempHum));
  SpanCast barometerSensor(BAROMETER_DEVICE_ID,sizeof(windSpeed),sizeof(barometerMessage));

  uint32_t updateTime=0;

  while(1){

    if(millis()-updateTime > 8000){

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

      Serial.printf("***\n*** Current Conditions: Temperature = %0.1fC, Humidity = %d%%, Wind = %d km/hr, Barometer = %s\n***\n",tempHum.temperature,tempHum.humidity,windSpeed,barometerMessage);

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

