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


/////////////////////////////////////////////////
//                                             //
//    SpanCast: Basic Transmitter Example      //
//    -----------------------------------      //
//                                             //
//    * transmits a simple counting integer    //
//      from this device to another device     //
//      every 5 seconds                        //
//                                             //
/////////////////////////////////////////////////

#include "SpanCast.h"

void setup(){

  Serial.begin(115200);
  delay(2000);

  Serial.printf("\n\nSpanCast Basic Transmitter Example\n\n");

  int counter=0;                                // define and initialize a simple integer counter
  SpanCast::configure(3);                       // configure SpanCast and set Device ID=3
  SpanCast myLink(5, sizeof(counter), 0);       // construct SpanCast object with link to device with Device ID=5

  do {
    Serial.printf("Transmitting counter=%d to other device...",counter);

    boolean status = myLink.send(&counter);   // transmit counter and save the returned status

    if(status)                                // indicate whether transmission succeeded or failed
      Serial.printf("Success!\n");
    else
      Serial.printf("Failed!\n");

    counter++;                                // increment counter
    delay(5000);                              // delay 5 seconds
  } while(1);                                 // repeat indefinitely
}

void loop() {}                                          // nothing is needed in the loop() function
