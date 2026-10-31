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

## SpanCast Quick-Start Overview

SpanCast is implemented as a single C++ *class* providing both static functions and non-static methods.  To access SpanCast, simply add `#include "SpanCast.h"` to the top of your sketch.

### Configure SpanCast and assign a SpanCast *Device ID*

Before you can create SpanCast objects you need to first configure SpanCast and assign a unique 16-bit SpanCast *Device ID* to your device.  This ID is used by SpanCast to direct transmissions to, and identifying transmissions received from, other SpanCast devices.  To configure SpanCast and set the *Device ID* call the static function `SpanCast::configure(devID)`, where *devID* is the *Device ID* you wish to assign to your device.

### Create one or more SpanCast Objects

Once SpanCast is configured, you can then create SpanCast objects. Each SpanCast object represents a bi-directional communication link betweeen your device and another device. To create a SpanCast object, use the constructor `SpanCast(remoteDevID, sendSize, receiveSize)`, where *remoteDevID* is the *Device ID* of the device at the other end of the link, *sendSize* is the size (in bytes) of any messsages you intend to transmit to that device, and *receiveSize* is the size (in bytes) of any messsages you expect to receive from that device.  You may set *sendSize* to zero if messsages are not intended to trasnsmitted to other devices using this link.  Similarly you may set *receiveSize* to zero if messages are not intended to be received from other devices using this link.

### Transmit and Receive SpanCast Data Messages

After you have created a SpanCast object, simply call the `send(void *data)` method on that object to transmit the data referenced by the pointer *data* from your device to the device at the other end of the link.  The `send()` method returns *true* if the transmission succeeds, else *false* if it fails.

To receive data from the device at the other end of the link simply call the `get(void *dataBuf)` method on that SpanCast object.  If data is available from that device, `get()` will return *true* and the received message will be copied to the variable referenced by the pointer *dataBuf*.  If data is not available to be received from that device, `get()` will instead return *false* and *dataBuf* will be unmodified.  Repeatedly calling `get()` from within a loop is an easy way to "poll" for incoming messages transmitted to your device from other SpanCast devices.

### A Basic Transmitter/Receiver Example

Just using the above, we can create a pair of sketches in which one device transmits a simple counting integer to another device every 5 seconds.  In this example we will set the *Device ID* to 3 for the transmitter the *Device ID* to 5 for the receiver.  Here is the sketch for the transmitting device:

```C++
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
```

And here is the sketch for the receiving device:

```C++
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
```

Note the differences in the SpanCast constructors in each sketch.  For the transmitter the second argument, *sendSize*, was set to the size of the counter and third argument, *receiveSize* was set to zero.  For the receiver, *sendSize* is set to zero and *receiveSize* is set to the size of the counter.

## SpanCast Optional Settings

SpanCast provides a variety of optional settings to modify the default behavior of the system. For ease of use, all settings that impact the global behavior of SpanCast are encapsulated into a single structure that can be passed to `SpanCast::configure()` as an optional second argument. This structure, and the defaults for each setting is left unspecified, as as follows:

```C++
struct SpConfig_t {
  uint8_t network=1;
  String password="spancast123";
  boolean encrypt=true;
  uint16_t channelMask=0;
  uint8_t numTries=3;
}
```
Similarly, all settings that impact the behavior of a specific SpanObject are encapsulated into a single structure that can be passed to the `SpanCast()` constructor as an optional fourth argument.  This structure, and the defaults for each setting is left unspecified, as as follows:

```C++
struct SpCast_t {
  boolean encrypt=false;
  size_t queueDepth=0;
  uint32_t timeout=60000;
}
```

Note that you do not need to first create an instance of these structures as a separate object to be passes to their corresponding functions as an optional parameter.  Instead SpanCast has been designed to allow you to pass a brace-enclosed list of designated initializers to each function, where you only need to specifiy the settings you want to change.  For example:

```C++
SpanCast::configure(3, { .password="MyPassword", .numTries=2 } );
```

sets the *Device ID* to 3, changes the settings specified, and retains the defaults for all others.  Note that in C++, designated initializers *must* be specified in the same order as they appear in the structure definition.  Skipping settings is fine (as shown above), but specifiying them out of order (i.e. first .numTries followed by .passsword) will result in a compilation error.

The sections below discuss each of these settings, their purpose, and typical use cases.

### WiFi Channel Auto-Synchronization (*channelMask* and *numTries* settings)

WiFi networks operating at 2.4 GHz span fourteen overlapping channels numbered 1-14 with regional limitations: in North America channels 1-11 are generally allowed; outside of North America channels 1-13 can be used; and in Japan channel 14 is also available for use in certain circumstances.  In order for WiFi transmissions from one device to be reliably received by another device, the WiFi radios on both devices must be set to the same channel.[^wifi]

[^wifi]: If two devices are in close proximity, it is possible for messages to be successfully received even if the WiFi channels of the devices differ by one or two (e.g. channels 5 and 6).  However, reception will degraded.  To ensure the best fidelity and coverage, the devices should always be set to the same channel.

By default, SpanCast will refrain from making any changes to the WiFi channel used by the device.  Instead, SpanCast will transmit data on, and listen for data from, whatever WiFi channel happens to be set when the member functions `send()` and `get()` are called.  This is the proper setting for any SpanCast device that will be connected to a central WiFi network (e.g. using `WiFi.begin()`), since the WiFi library itself takes care of setting the WiFi channel to ensure it stays synchronized with whatever channel is being used by your central network's WiFi router.  In such cases SpanCast simply uses the channel the WiFi library has selected.

However, SpanCast also includes an ability to dynamically changes the channel of the device's WiFi radio whenever the member function `send()` is called in an attempt to match the WiFi channel used by the receiving device.  Usage of this option is the proper setting for any SpanCast device that is not itself connected to a central WiFi network, but that is transmitting to another SpanCast device that is connected to a central WiFi network.

Invoking this option is done during the call to `configure()` by setting the optional *channelMask* parameter to a non-zero value.  The *channelMask* parameter is a 16-bit mask that allows you to specify which channels SpanCast should utilize when transmitting SpanCast messages.  For each bit N in *channelMask*, where N ranges from 1 to 14, if the bit is **set** SpanCast will attempt transmissions using WiFi channel N.  If bit N is **not set** SpanCast will not use that channel.  For example, setting the *channelMask* to 64 (0x0040) tells SpanCast to use only WiFi channel 6.  Setting the *channelMask* instead to 4094 (0x0FFE) tells SpanCast to use any channel from 1-11. Note that since there are no WiFi Channels 0 or 15, bit 0 and bit 15 of *channelMask* do not matter and are typically left as zero.

WiFi channel auto-synchronization applies whenever you transmit a messsge using `send()`.  SpanCast first tries to transmit the message using whatever WiFi channel is currently set.  If the transmission is successful `send()` exits and returns `true`.  If the transmission instead fails, SpanCast tries again using the same channel.  SpanCast will continue re-transmitting the message until either it succeeds or the number of tries equals the *numTries* parameter, which defaults to 3, but can be set to any value greater than zero[^numTries] during the call to `configure()`.

[^numTries:] If you set *numTries* to zero, SpanCast will reset it to 1 during the call to `configure()`

If SpanCast still fails to transmit successfully after *numTries* attempts, it changes the WiFi radio to next allowed channel based on the *channelMask* parameter, and repeats the above process.  In this fashion SpanCast will loop through WiFi channels allowed by *channelMask* and attempt to transmit until either it succeeds or it has tried every WiFi channel allowed at least once.  For example, if the *channelMask* is set to allow only channels {1,3,5,6,9} and the WiFi channel happens to be set to 5 when `send()` is called, SpanCast will first try channel 5, followed by 6, 9, 1, and finally 3.  If at any point the transmission succeeds, `send()` saves the current channel in long-term memory, exits, and returns `true`.  This ensures that `send()` always uses the most recent successful channel when it is next called, even if power is cycled.  Unless the receiving device changes the channel it is using, `send()` will generally not have to probe different channels on subsequent calls.

However, if after trying channel 3 (the last channel allowed by *channelMask* in this example) the transmission is still unsuccessful, SpanCast will reset the channel back to 5 (where it started), terminate `send()` and return `false`.

#### Use Cases

The most common use case is a network topology where one or more standalone SpanCast devices serve as remote sensors transmitting data to a central SpanCast device that is connected to a central WiFi network, perhaps also connected to HomeKit, or maybe just running a Web Server to display the results of the remote sensors.  In this topology the *channelMask* is either left unspecified or set to zero (the default) on the device connected to the central WiFi network, and the *channelMask* on all other devices is set to use the widest range of allowable channels in your region.  This allows each remote device to auto-synchronize its WiFi channel to match that of the central device when it transmits to that device.  As a result, each remote device will naturally sync to the same channel, which thus enables transmissions from one remote device to any other as well. 

In topologies where none of the devices are connected to a central WiFi network, there are numerous possible configurations you can chose:

* pick a WiFi channel to use and set the *channelMask* on each device to allow just that channel.  Every device will always transmit (and listen for transmissions) on only that channel.  The advantage of this configuration is that there is no need for any channel synchronization between devices, so it does not matter if some device only receive data and never transmit (in which case there would be no opportunity of such a device to synchronize to other devices).  The disadvantage of this configuration is that if you want to use a different channel, you need to update *channelMask* and reload your sketches with the new value for every device.

* arbitrarily assign one of the devices to act as the anchor for channel synchronization, pick a WiFi channel to use for your network and set the *channelMask* on that specific device to allow just that channel.  Then on all other devices, set the *channelMask* to use the widest range of channels allowed in your region.  The advantage of this configuration is that you only need to set the channel on one device and every other device auto-synchronizes to that channel.  If you later want to use a different channel you only need to update the sketch on one device and all other devices will re-sync to that channel they next time they transmit data to that device.  The disadvantage of this configuration is that it can only be used if every device is transmitting data to a common device (either directly, or through another device) that is configured with the channel you specified.

* leave the *channelMask* unspecified on one of the devices and set to use the widest range of channels allowed in your region on all other devices.  This is basically the same as the configuration above, but instead of setting the channel on a specific "anchor" device, the channel on that device will default to some value at start-up.

Further considerations:

* WiFi synchronization only occurs when a device transmits data.  If you have a SpanCast device that only receives data that is *not* a central device with a channel that is either pre-specified or controlled by a central WiFi network, there will be no way for the device to synchronize its channel with the rest of the network.  In such case you should create a dummy variable to periodically transmit to some other device on your network that is synchronized or that has a centrally-controlled channel.  This dummy variable does not need to be read or consumed by that other device.  As long as the first device transmits something to the other device, it will be able to synchronize its channel to that device even if that other device does not have an instance of a SpanCast object that is expecting to receive any data from the first device (though it at least needs to have SpanCast configured). 

* In any network of SpanCast devices there should be at least one device with either a fixed WiFi channel or one that is connected to a central WiFi network.  If every device in your SpanCast network is configured to auto-sync with a range of channels allowed by the *channelMask* the devices will likely not settle down on a specific channel as they are each is trying to synchronize to the other without any fixed channel as a reference.

* Both the ESP32 and ESP8266 microcontrollers have built-in logic that encode the region of the chip and the allowed channels.  SpanCast uses this information during configuration to filter out from the *channelMask* any channels that are not allowed according the chips encoded region.  For example, if the encoded region for your chip is North America and you set the *channelMask* to 0x3FFE (allowing for channels 1-13), *channelMask* will be reset to 0x0FFE by `config()` to exclude channels 12 and 13 since they are not allowed in North America.  Caution: though tempting, do not simply set the *channelMask* to 0xFFFF and allow SpanCast to filter out the disallowed channels based on the region code for your chip. ESP32 and ESP8266 chips are shipped world-wide and there is no guarantee that device you have has been pre-coded with the region where the device will be used.  It's always best set the *channelMask* to only those channels you know are allowed in your region.

### Example Sketches

Below is a simple example showing the sketches for one device transmitting a simple counting variable to another device.  For those new to C++, note that in the first sketch we kept all the logic in the Arduino `setup()` function and were able to instantiate a local SpanCast object.  But in the second sketch we placed the SpanCast polling logic in the Arduino `loop()` function, which required the use of a global SpanCast pointer that is instantiated in the `setup()` function using `new`.  The reason for these differences is purely for demonstration purposes.  Both sketches could have been structured in the same manner.



## Technical Details (for those interested)

### SpanCast Device Identification

SpanCast utilizes Espressif's ESP-NOW protocol for transmitting and receiving data.  ESP-NOW in turn uses the unique MAC addresses built into the WiFi stacks of ESP32 devices to identify which transmissions are intended for which recipients.  Implementing ESP-NOW directly in a sketch therefore requires you to either hardcode the MAC addresses of all the devices in your network, or to use a generic broadcast MAC address so that you transmit the same data to all device at the same time.  Unfortunately, each of the methods  has drawbacks:

* Hardcoding MAC addresses into a sketch makes that sketch non-portable.  If you replace one device on your network you need to update the sketches on every other device that either transmits data to, or receives data from that new device, with that new device's specific MAC address.

* Using a broadcast MAC address to avoid hardcoding individual MAC addresses into a sketch means giving up ESP-NOW encryption and, even more importantly, the ability to easily verify whether transmissions were sucessfully received by their intended recipients.  SpanCast uses this ability to automatically synchronize WiFi channels across all of your devices, thereby ensuring your SpanCast data transmissions are successfully received.

To solve these issues, SpanCast abstracts away the MAC addresses and instead uses the concept of a simple, user-definable SpanCast *Device ID* that you create in each sketch.  SpanCast requires that every device on your SpanCast network is assigned a unique *Device ID*, in the range of 0 through 255.  As described below, within each sketch you can easily direct data transmissions to, and receive data from, other devices based on their resepctive SpanCast *Device IDs*, without needing to know anything about any device's MAC address.  And if a device in your network needs to be replaced, you can simply upload the same sketch to your replacement device without changing any SpanCast identifiers in any sketch, whether on the new device or any other device in your network.



