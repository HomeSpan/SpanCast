# SpanCast: Peer-to-Peer Messaging Made Simple

SpanCast is a lightweight and easy-to-use peer-to-peer messaging system for ESP32 devices packaged as a ready-to-run library under the Arduino-ESP32 Core. SpanCast is based on Espressif's ESP-NOW protocol and provides bi-directional, point-to-point communication of short, fixed-size messages directly between ESP32 devices using those device's WiFi radios but *without the need for a central WiFi network*.

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
* **Full Support for legacy ESP8266 devices when run under the Arduino-ESP8266 Core**

In addition:

* **SpanCast has been optimized to work seamlessly with HomeSpan**

  * enables users to create custom, standalone ESP32-based sensors and controls that connect directly Apple HomeKit
  * low-power requirements of ESP-NOW mean such devices can be *battery-powered*

## General Overview

SpanCast is implemented as a single C++ *class*.  To use, simply add `#include "SpanCast.h"` to the top of your sketch.
