# SpanCast: Peer-to-Peer Messaging Made Simple

SpanCast is a lightweight and easy-to-use peer-to-peer messaging system for ESP32 devices packaged as a ready-to-run library under the Arduino-ESP32 Core. SpanCast is based on Espressif's ESP-NOW protocol and provides bi-directional, point-to-point communication of short, fixed-size messages directly between ESP32 devices using those device's WiFi radios but *without the need for a central WiFi network*.  Key features of SpanCast include:

* automatic channel management to ensure all devices remain on same WiFi channel
  * does not interfere with the ability of device to connect to, and use, a central WiFi network while also using SpanCast
* user-defined Device IDs
  * the Device IDs used by SpanCast are defined by the user in each sketch (unlike ESP-NOW, which requires the use of chip-based MAC addresses)
* fully managed transmissions
  * built-in data queues, message encryption, separate message authentication, and anti-duplication logic

SpanCast can be used for any purpose where ESP32 devices need to communicate with each other but has been specifically designed to work seamlessly with HomeSpan, enabling users to create custom remote (battery-operated) sensors that connect directly Apple HomeKit.

In addition to the ESP32, SpanCast fully supports ESP8266 devices run under the Arduino-ESP8266 Core.

SpanCast is implemented as a single C++ *class*.  To use, simply add `#include "SpanCast.h"` to the top of your sketch. Detailed descriptions of the SpanCast class and all of its methods are provided below.
