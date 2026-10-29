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


///////////////////////////////////////////////////
//                                               //
//    SpanCast: Basic Receiver Example           //
//    -----------------------------------        //
//                                               //
//    * polls for a message containing a simple  //
//      counting integer from another device     //
//                                               //
///////////////////////////////////////////////////

#include "SpanCast.h"

void setup(){

  Serial.begin(115200);
  delay(2000);

  Serial.printf("\n\nSpanCast Basic Receiver Example\n\n");

  int counter;                                  // define a simple integer counter (no need to initialize)
  SpanCast::configure(5);                       // configure SpanCast and set Device ID=5
  SpanCast myLink(3, 0, sizeof(counter));       // construct SpanCast object with link to device with Device ID=3

  do {
    if( myLink.get(&counter) )                                              // check if a message has been received from the other device     
      Serial.printf("Received counter=%d from other device...",counter);    // if so, print it
  } while(1);                                   // repeat indefinitely
}

void loop() {}                                  // nothing is needed in the loop() function
