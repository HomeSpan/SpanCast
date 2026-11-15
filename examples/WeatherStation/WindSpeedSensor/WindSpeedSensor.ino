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

#define UPDATE_FREQUENCY     8000   // how frequently (in millseconds) for this device to send out updates

// Create global pointers for SpanCast objects so they can be created in setup() and used in loop()

SpanCast *barometerSensor;
SpanCast *tempHumSensor;

// Create a single structure to hold all weather data from this sensor as well as the remote sensors
// Fill with default starting data but set Active flag to false so data is not yet displayed

struct WeatherData {
  struct {
    float temperature=-10;
    uint8_t humidity=30;
  } tempHum;
  boolean tempHumActive=false;
  
  uint8_t windSpeed=10;
  boolean windSpeedActive=false;

  char barometerMessage[64]="Steady";
  boolean barometerMessageActive=false;
} weatherData;

// Create global time counter to keep track of when to next send data

uint32_t updateTime=0;

//////////////////////

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.printf("\n\nSpanCast Version: %s\n",SpanCast::version());

  // Configure SpanCast for this device with the following optional settings:
  //  * Network ID set to 4
  //  * Global enryption disabled
  //  * Channel Mask set to fixed value of channel 5

  SpanCast::configure(WIND_SPEED_DEVICE_ID,{.network=4,.encrypt=false,.channelMask=SpanCast::list({5})});

  // Create SpanCast objects for the remote sensors.  Note encryption has been re-enabled for the Temp/Hum sensor

  tempHumSensor = new SpanCast(TEMP_HUM_DEVICE_ID,sizeof(weatherData.windSpeed),sizeof(weatherData.tempHum),{.encrypt=true});
  barometerSensor = new SpanCast(BAROMETER_DEVICE_ID,sizeof(weatherData.windSpeed),sizeof(weatherData.barometerMessage));

  // Set Active flag to true for data from this device

  weatherData.windSpeedActive=true;

  Serial.printf("Wind Speed Sensor Ready.\n\n");
}

//////////////////////

void loop() {

  // Check if there is any data available from remote sensors.  If yes, print message and then call display() to show all weather data

  if(barometerSensor->get(weatherData.barometerMessage)){
    Serial.printf("Received from Barometer Sensor: Barometer=%s\n",weatherData.barometerMessage);
    weatherData.barometerMessageActive=true;
    displayData();
  }

  if(tempHumSensor->get(&weatherData.tempHum)){
    Serial.printf("Received from Temp/Hum Sensor: Temp=%0.1f, Hum=%d\n",weatherData.tempHum.temperature,weatherData.tempHum.humidity);
    weatherData.tempHumActive=true;
    displayData();
  }

  // If it is time to update sensor reading(s) on this device...

  if(millis()-updateTime > UPDATE_FREQUENCY){

    // Update data on this device and print message

    if(++weatherData.windSpeed>30)
      weatherData.windSpeed=10;

    Serial.printf("New Wind Speed Reading: %d\n",weatherData.windSpeed);

    // Send update to other sensors

    Serial.printf("Sending Wind Speed Reading to Temp/Hum Sensor... ");
    if(tempHumSensor->send(&weatherData.windSpeed))
      Serial.printf("Succeeded\n");
    else
      Serial.printf("Failed\n");

    Serial.printf("Sending Wind Speed Reading to Barometer Sensor... ");
    if(barometerSensor->send(&weatherData.windSpeed))
      Serial.printf("Succeeded\n");
    else
      Serial.printf("Failed\n");

    // Display updated weather data
  
    displayData();

    // Update timer

    updateTime=millis();        
  }

  // Check to see if a remote sensor has been flagged as Active, but new data from that sensor has not been received in a while.  If so, flag as Inactive

  if(weatherData.barometerMessageActive && !barometerSensor->isActive()){
    Serial.printf("Have not received data from Barometer Sensor in quite a while!\n");
    weatherData.barometerMessageActive=false;
  }

  if(weatherData.tempHumActive && !tempHumSensor->isActive()){
    Serial.printf("Have not received data from Temp/Hum Sensor in quite a while!\n");
    weatherData.tempHumActive=false;
  }
}

//////////////////////

void displayData(){

  // Display all weather data except replace values with "--" for sensors flagged as Inactive

  Serial.printf("\n---------------------------\n");

  if(weatherData.tempHumActive){
    Serial.printf("Temperature:  %0.1fC\n",weatherData.tempHum.temperature);
    Serial.printf("Humidity:     %d%%\n",weatherData.tempHum.humidity);
  } else {
    Serial.printf("Temperature:  ---\n");
    Serial.printf("Humidity:     ---\n");
  }

  if(weatherData.windSpeedActive)
    Serial.printf("Wind Speed:   %d km/hr\n",weatherData.windSpeed);
  else
    Serial.printf("Wind Speed:   ---\n");

  if(weatherData.barometerMessageActive)
    Serial.printf("Barometer:    %s\n",weatherData.barometerMessage);
  else
    Serial.printf("Barometer:    ---\n");

  Serial.printf("---------------------------\n\n");
}

//////////////////////

