# SpanCast: Peer-to-Peer Messaging Made Simple

SpanCast is a lightweight and easy-to-use peer-to-peer messaging system for ESP32 devices packaged as a ready-to-run library under the [Arduino-ESP32 Core](https://github.com/espressif/arduino-esp32). SpanCast is based on Espressif's ESP-NOW protocol and provides bi-directional, point-to-point communication of short, fixed-size messages directly between ESP32 devices using those device's WiFi radios but *without the need for a central WiFi network*.

Key features of SpanCast include:

* **Automatic Channel Management**


  * ensures all devices sync to the same WiFi channel
  * allows users to optionally limit which channels can be used
  * provides for the simultaneous use of SpanCast for peer-to-peer communications alongside connections to a central WiFi network for broader internet access
* **Simple Network Topology**
  * allows users to configure each device with a unique SpanCast Device ID
  * allows further configuration of SpanCast into distinct networks using an optional unique Network ID
  * users **do not** need know the MAC addresses of any device or hardcode MAC addresses into any sketch
* **Fully-Managed Transmissions**
  * built-in data queues with user-specified depth
  * allows use of native ESP-NOW message encryption
  * enhances security by further authenticating messages even when not encrypyted
  * provides anti-duplication logic so that re-transmissions of unacknowledged messages do not re-trigger multiple alerts
* **Full Support for legacy ESP8266 devices when run under the [Arduino-ESP8266 Core](https://github.com/esp8266/arduino)**

AND...

* **SpanCast has been optimized to work seamlessly with [HomeSpan](https://github.com/HomeSpan/HomeSpan)**

  * enables users to create custom, standalone ESP32-based sensors and controls that connect directly Apple HomeKit
  * low-power requirements of ESP-NOW mean such devices can be *battery-powered*

## General Overview

SpanCast is implemented as a single C++ *class* providing both static functions and non-static methods.  To access SpanCast, simply add `#include "SpanCast.h"` to the top of your sketch.

Each instantiation of a SpanCast object represents a bi-directional link between your device and another device, which also must be running SpanCast.  However, before creating any SpanCast objects you first need to configure SpanCast by calling the static function `SpanCast::configure()`, typically from with the `setup()` portion of your sketch.  This function allows you to set a SpanCast *Device ID* for your device, in the range of 0 through 255.  The *Device ID* needs to be unique for each device running SpanCast, though there are optional arguments to `configure()` (described further below) that allow you to partition your SpanCast devices into distinct "networks" with a separate SpanCast *Network ID* ranging from 0 through 65535.  SpanCast devices that share the same *Network ID* can send and receive messages to each other independently of any devices that may have the same *Device ID* but a different *Network ID*.

Once SpanCast is configured, you can then instantiate SpanCast objects using the SpanCast constructor.  The constructor requires three arguments: the *Device ID* of the *remote* device (also running SpanCast) that you wish to communicate with; the size (in bytes) of the messages to be transmitted from your device to the remote device; and the size (in bytes) of the messages your device expects to receive from the remote device.  Either of the last two arguments may be zero if messages are only being either transmitted or received, but not both.  The SpanCast constructor also includes a number of optional arguments (described further below) that allow you to fine-tune various message transmission parameters.

After you have created a SpanCast object, you then simply call the `send()` method to transmit your message.  This method takes a pointer to another variable containing your message as its only argument.  Your message can represent a single number, a structure of numbers, a data buffer, or even an actual text message.  Anything that can be converted to a `(void *)` pointer can be sent.

Receiving is just as easy.  Simply call the `get()` method.  This method also takes a `(void *)` pointer as its only argument, though in this case the pointer will be filled with the received message, *if it exists*.  The `get()` method returns `true` if data is found, else it returns `false`.  Repeatedly calling `get()` from with a loop is an easy way to "poll" your remote devices for incoming messages.

### WiFi Channels

WiFi networks operating at 2.4 GHz provide fourteen overlapping channels numbered 1-14 with regional limitations: in North America channels 1-11 are generally allowed; outside of North America channels 1-13 can be used; and in Japan channel 14 is also available for use in certain circumstances.  In order for WiFi transmissions from one device to be reliably received by another device, the WiFi radios on both devices must be set to the same channel.[^wifi]

[^wifi]: If two devices are in close proximity, it is possible for messages to be successfully received even if the WiFi channels of the devices differ by one or two (e.g. channels 5 and 6).  However, reception will degraded.  To ensure the best fidelity and coverage, the devices should always be set to the same channel.

By default, SpanCast will refrain from making any changes to the WiFi channel used by the device.  Instead, SpanCast will transmit data on, and listen for data from, whatever WiFi channel happens to be set when the member functions `send()` and `get()` are called.  This is a proper setting for any SpanCast device that will be connected to a central WiFi network (e.g. using `WiFi.begin()`), since the WiFi library itself takes care of setting the WiFi channel to ensure it stays synchronized with whatever channel is being used by your central network's WiFi router.  In such cases SpanCast simply uses whatever channel the WiFi library has selected.

However, SpanCast also includes a user-configurable option that dynamically changes the channel of the device's WiFi radio whenever the member function `send()` is called in an attempt to match the WiFi channel used by the receiving device.  Usage of this option is the proper setting for any SpanCast device that is not itself connected to a central WiFi network, but that is transmitting to another SpanCast device that is connected to a central WiFi network.

Invoking this option is done during the call to `configure()` by setting the optional *channelMask* parameter to a non-zero value.  The *channelMask* parameter is a 16-bit mask allowing you to specify which channels SpanCast should utilize when transmitting SpanCast messages.  For each bit N in *channelMask*, where N ranges from 1 to 14, if the bit is set SpanCast will attempt transmissions using WiFi channel N.  If bit N is not set SpanCast will not use that channel.  For example, setting the *channelMask* to 64 (0x0040) tells SpanCast to use only WiFi channel 6.  Setting the *channelMask* instead to 4094 (0x0FFE) tells SpanCast to use any channel from 1-11. Note that since there are no WiFi Channels 0 or 15, bit 0 and bit 15 of *channelMask* do not matter and are typically left as zero.

When `send()` is called, SpanCast first tries to transmit the message using whatever WiFi channel is currently set.  If the transmission is successful `send()` exits and returns `true`.  If the transmission instead fails, SpanCast tries again.  SpanCast will continue re-transmitting the message until either it succeeds or the number of tries equals a preset maximum number of tries.  This maximum defaults to 3 but can be changed to any number by setting the optional *numTries* parameter during the call to `configure()`.

If SpanCast still fails to transmit successfully after *numTries* attempts, it changes the WiFi radio to next allowed channel based on the *channelMask* parameter, and repeats the above process.  In this fashion SpanCast will loop through WiFi channels allowed by *channelMask* and attempt to transmit until either it succeeds or it has tried every WiFi channel allowed at least once.  For example, if the *channelMask* is set to allow only channels {1,3,5,6,9} and the WiFi channel happens to be set to 5 when `send()` is called, SpanCast will first try channel 5, followed by 6, 9, 1, and finally 3.  If at any point the transmission succeeds, `send()` saves the current channel in long-term memory, exits, and returns `true`.  This ensures that `send()` always uses the most recent successful channel when it is next called, even if power is cycled.  Unless the receiving device changes the channel it is using, `send()` will generally not have to probe different channels on subsequent calls.

However, if after trying channel 3 (the last channel allowed by *channelMask* in this example) the transmission is still unsuccessful, SpanCast will reset the channel back 5 (where it started), terminate `send()` and return `false`.


### Example Sketches

Below is a simple example showing the sketches for one device transmitting a simple counting variable to another device.  For those new to C++, note that in the first sketch we kept all the logic in the Arduino `setup()` function and were able to instantiate a local SpanCast object.  But in the second sketch we placed the SpanCast polling logic in the Arduino `loop()` function, which required the use of a global SpanCast pointer that is instantiated in the `setup()` function using `new`.  The reason for these differences is purely for demonstration purposes.  Both sketches could have been structured in the same manner.






