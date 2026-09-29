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

### Example Sketches

Below is a simple example showing the sketches for one device transmitting a simple counting variable to another device.  For those new to C++, note that in the first sketch we kept all the logic in the Arduino `setup()` function and were able to instantiate a local SpanCast variable.  But in the second sketch we placed the SpanCast polling logic in the Arduino `loop()` function, which required the use of a global SpanCast variable that is instantiated in the `setup()` function using `new`.  






