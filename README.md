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

WiFi networks operating at 2.4 GHz provide fourteen overlapping channels numbered 1-14 with regional limitations: in North America channels 1-11 are generally allowed; outside of North America channels 1-13 can be used; and in Japan channel 14 is also available for use in certain circumstances.

In order for WiFi transmissions from one device to be successfully received by another device, the WiFi radios on both devices should be set to the same channel.

By default, SpanCast will refrain from making any changes to the WiFi channel used by the device.  This is a proper setting for any device that will be connected to a central WiFi network (e.g. using `WiFi.begin()`), since the WiFi library itself takes care of setting the WiFi channel to ensure it stays synchronized with whatever channel is being used by your central network's WiFi router.



Depending on whether your device is also connected to a central WiFi network, you can instruct SpanCast to take control of the channel selection, or you SpanCast can allow allows you to optionally specify a channel as part of the `configure()` function, which works fine for if your devices only connect to each other via SpanCast.

However, things get more complicated if one or more of your devices using SpanCast *also* connects to your central WiFi network, as is quite typical.  When a device connects to a central WiFi network, that network's router chooses the WiFi channel to use, and upon connecting, the device will adopt that channel.  More so, though it is possible to preset the WiFI channel used by WiFi router to a specific value, it is much more common to allow the WiFi router to dynamically select and periodically update the WiFi channel it uses based on current radio conditions and interference.  Though this helps keep home networks optimized, it means the WiFi channel used by any SpanCast device that is *not* connected to the central WiFi network (such as remote-sensor devices, battery-operated pushbutton devices, etc.) will quickly get out of sync with the WiFi channel used by devices that are connected to the central network.

SpanCast has been designed to automatically solve this problem through the use of an optional *channelMask* parameter to the `configure()` function.  This parameter acts as a 16-bit mask allowing you to specify which channels the device should utilize when transmitting SpanCast messages.  If bit N is set, that means channel N is allowed, else it is not.  Note that since there are no WiFi Channels 0 or 15, those bits do not matter and are typically left as zero.  For example, setting the *channelMask* to 64 (0x0040) tells SpanCast to use only WiFi Channel 6.  Setting the *channelMask* instead to 4094 (0x0FFE) tells SpanCast to use any channel from 1-11.

Setting the *channelMask* exactly to zero has an important and special meaning:  It tells SpanCast *not* to perform any channel management and to simply transmit messages using whatever channel is already set.  This is the setting to use if your device is also connecting to a central WiFi network, as it allows the WiFi connection logic to manage the channel settings according to the central network without interference from SpanCast.


[^wifi]: If two devices are in close proximity, it is possible for messages to be successfully received even if the WiFi channels of the devices differ by one or two (e.g. channels 5 and 6).  However, reception will degraded.  To ensure the best fidelity and coverage, the devices should always be set to the same channel.

### Example Sketches

Below is a simple example showing the sketches for one device transmitting a simple counting variable to another device.  For those new to C++, note that in the first sketch we kept all the logic in the Arduino `setup()` function and were able to instantiate a local SpanCast variable.  But in the second sketch we placed the SpanCast polling logic in the Arduino `loop()` function, which required the use of a global SpanCast variable that is instantiated in the `setup()` function using `new`.  






