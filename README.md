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
  * users *do not* need know the MAC addresses of any device or hardcode MAC addresses into any sketch
* **Fully-Managed Transmissions**
  * built-in data queues with user-specified depth
  * allows use of native ESP-NOW message encryption
  * enhances security by further authenticating messages even when not encrypted
  * provides anti-duplication logic so that re-transmissions of unacknowledged messages do not re-trigger multiple alerts
* **Full Support for legacy ESP8266 devices when run under the [Arduino-ESP8266 Core](https://github.com/esp8266/arduino)**

AND...

* **SpanCast has been optimized to work seamlessly with [HomeSpan](https://github.com/HomeSpan/HomeSpan)**

  * enables users to create custom, standalone ESP32-based sensors and controls that connect directly to Apple HomeKit
  * low-power requirements of ESP-NOW mean such devices can be *battery-powered*

> ## PRE-RELEASE.  FINAL PRODUCTION VERSION COMING SHORTLY
> Latest Update - SpanCast 1.0.0 (XX NOV 2026)
>
> This is the initial release of SpanCast

# SpanCast Quick-Start Overview

SpanCast is implemented as a single C++ *class* providing both static class-level functions and non-static object-level methods.  To access SpanCast, simply add `#include "SpanCast.h"` to the top of your sketch.

## Step 1: Configure SpanCast and assign a SpanCast *Device ID*

Before you can create SpanCast objects you need to first configure SpanCast and assign a unique 16-bit SpanCast *Device ID* to your device.  This ID is used by SpanCast to direct transmissions to, and identifying transmissions received from, other SpanCast devices.  To configure SpanCast and set the *Device ID* call the static function `SpanCast::configure(deviceID)`, where **deviceID** is the *Device ID* you wish to assign to your device.

## Step 2: Create one or more SpanCast Objects

Once SpanCast is configured, you can then create SpanCast objects. Each SpanCast object represents a bi-directional communication link between your device and another device. To create a SpanCast object ("link"), use the constructor `SpanCast(deviceID, sendSize, receiveSize)`, where **deviceID** is the *Device ID* of the remote device **at the other end of the link**, *sendSize* is the size (in bytes) of any messages you intend to transmit to that device, and *receiveSize* is the size (in bytes) of any messages you expect to receive from that device.

As a result of message size limitations in ESP-NOW protocol, the maximum allowed value for **sendSize** and **receiveSize** is 1434 bytes on ESP32 devices and 214 bytes on ESP8266 devices.[^maxSize]  For messages transmitted between an ESP32 and ESP8266, the lower maximum of 214 prevails.

Note you may set **sendSize** to zero if messages are not intended to be transmitted to other devices using this link.  Similarly you may set **receiveSize** to zero if messages are not intended to be received from other devices using this link.  However, you can not set both **sendSize** and **receiveSize** to zero for the same link since there would be no point to creating such an object.

[^maxSize]: The ESP-NOW v2 protocol used by the ESP32 limits the size of a message transmission to 1470 bytes of application data.  The ESP-NOW v1 protocol used by the ESP8266 limits this size to 250 bytes.  Since SpanCast needs to reserve 36 bytes of each message for a 32-byte HMAC signature and 4-byte nonce, this leaves a maximum of 1434 bytes per message on the ESP32, and 214 bytes per message on the ESP8266, for user data.

## Step 3: Transmit and Receive SpanCast Data Messages

After you have created a SpanCast object, simply call the `send(void *data)` method on that object to transmit the data referenced by the pointer **data** from your device to the device at the other end of the link.  The `send()` method returns `true` if the transmission succeeds, else `false` if it fails.

To receive data from the device at the other end of the link simply call the `get(void *dataBuf)` method on that SpanCast object.  If data is available from that device, `get()` will return `true` and the received message will be copied to the variable referenced by the pointer **dataBuf**.  If data is not available to be received from that device, `get()` will instead return `false` and **dataBuf** will be unmodified.  Repeatedly calling `get()` from within a loop is an easy way to "poll" for incoming messages transmitted to your device from other SpanCast devices.

## A Basic Transmitter/Receiver Example

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

Note the differences in the SpanCast constructors in each sketch.  For the transmitter the second argument, **sendSize**, was set to the size of the counter and third argument, **receiveSize** was set to zero.  For the receiver, **sendSize** is set to zero and **receiveSize** is set to the size of the counter.

> [!TIP]
> Both of these sketches are included as part of the SpanCast library and can be found in the Arduino IDE under *File → Examples → SpanCast → Basic*.

# SpanCast Optional Settings

SpanCast provides a variety of optional settings to modify the default behavior of the system. For ease of use, all settings that impact the global behavior of SpanCast are encapsulated into a single structure, `SpConfig_t`, that can be passed to `SpanCast::configure()` as an optional *second* argument. This structure, and the defaults for each setting if left unspecified, are as follows:

```C++
struct SpConfig_t {
  uint8_t network=1;
  String password="spancast123";
  boolean encrypt=true;
  uint16_t channelMask=0;
  uint8_t numTries=3;
}
```
Similarly, all settings that impact the behavior of a specific SpanObject are encapsulated into a single structure, `SpCast_t`, that can be passed to the `SpanCast()` constructor as an optional *fourth* argument.  This structure, and the defaults for each setting if left unspecified, are as follows:

```C++
struct SpCast_t {
  boolean encrypt=false;
  size_t queueDepth=0;
  uint32_t timeout=60000;
}
```

Note that you *do not* need to first create an instance of these structures as a separate object to be passes to their corresponding functions as an optional parameter.  Instead SpanCast has been designed to allow you to pass a brace-enclosed list of designated initializers to each function, where you only need to specify the settings you want to change.  For example:

```C++
// using designated initializers to specify optional settings:

SpanCast::configure(3, { .password="MyPassword", .numTries=2 } );
SpanCast remoteDevice(5, sizeof(int), 0, { .queueDepth=2 } );
```

Remember that in C++, designated initializers *must* be specified in the same order as they appear in the structure definition.  Skipping settings is fine (the defaults above will be applied), but specifying them out of order (i.e. first **numTries** followed by **password**) will result in a compilation error.

## Optional Settings when calling `SpanCast::configure()`

The sections below fully describe the global settings structure, `SpConfig_t`, that can be passed to `SpanCast::configure()` as an optional second argument.

### SpanCast Network ID (the *network* setting)

Changing the *Network ID* of a device from its default value of 1 to any other 8-bit number by setting the **network** parameter allows you to create logically distinct SpanCast networks.  SpanCast *Device IDs* must be unique across all devices with the same *Network ID*, but they do not need to be unique across devices with different *Network IDs*.

For example, if you have one set of 5 devices serving as temperature sensors and another set of 5 devices serving as remote pushbuttons, as long as the two sets of devices *do not* need to communicate with each other via SpanCast, you can change the *Network ID* of one set of devices to 2 (or any other number), and then both sets of devices can use the same *Device IDs* from 1-5 without concern about overlapping.

Note that changing the **network** setting using `configure()` impacts the device's *Network ID* as well as all SpanCast objects constructed in your sketch.  When you call `send()` on a SpanCast object it uses both the *Device ID* associated with that specific object as well as the *Network ID* assigned in the sketch to direct transmissions and receive incoming messages. It is not possible to transmit or receive messages from devices with separate Network IDs regardless of their *Device IDs*.

### SpanCast Encryption (the *password* and *encrypt* settings)

By default, SpanCast configures the underlying ESP-NOW protocol to encrypt all message transmissions.  The default password is "spanCast123" though you can (and should) change this to something secret by setting the **password** parameter when calling `configure()`.  You can also disable encryption entirely for all SpanCast objects constructed in your sketch by setting the **encrypt** parameter to `false` when calling `configure()`.

Apart from the obvious benefits of encryption to promote security, there are some important differences in how encryption also impacts the success and failure of message transmissions, as well as how SpanCast reports this from both the perspective of the transmitting device and the receiving device.  

As discussed in previous sections, when transmitting a message over a SpanCast link using `send()`, SpanCast will return `true` if the transmission is successfully or `false` if it has failed.  However, this return status only indicates whether the sending device has received a low-level acknowledgement from the receiving device at the transport layer that the receiving device has indeed received all the bytes expected.  The receiving device performs this acknowledgement before it inspects the actual message, determines if it is encrypted, and tries to decrypt it.  This means that `send()` will return `true` even if the the receiving device cannot decrypt or properly process the message it received (e.g. there is no matching SpanCast link to receive the message).   The return status of `send()` will only be `false` if the intended receiving device is not powered or out of range of the transmitting device, or if WiFi channel auto-synchronization between the transmitting and receiving devices (as described in the next section) has failed.

From the perspective of a receiving device successfully receiving a message, only after the WiFi transport layer sends a response back to the transmitting device acknowledging receipt of the message (and thus causing `send()` to return `true` on the transmitting device), does the underlying ESP-NOW protocol inspect the message itself.

If the message it receives is labeled as encrypted, ESP-NOW tries to decrypt the message using the SpanCast **password** set during the call to `configure()`.  If successful, ESP-NOW passes the decrypted message to SpanCast for further processing.  If on the other hand ESP-NOW cannot decrypt the message, either because the **password** is incorrect, or because encryption was not enabled on the receiving SpanCast link, ESP-NOW *silently fails* and provides no indication to SpanCast that it has done so (this is an unfortunate limitation of the ESP-NOW protocol).  For this reason, when first testing a new SpanCast network, you may want to *temporarily* disable encryption on all devices to ensure messages are always passed from ESP-NOW to SpanCast, where SpanCast's extensive diagnostics (detailed later in this document) can aid in any debugging.

If instead the message it receives is labeled as unencrypted, ESP-NOW automatically passes the message to SpanCast for further processing regardless of whether encryption has been enabled on the receiving device or there is even a SpanCast link object created to receive the message.  SpanCast handles each of these situation (and more) gracefully and reports any such issues as part of its extensive diagnostics.

However, because ESP-NOW does not indicate to SpanCast whether the message it is passing was originally unencrypted, or was originally encrypted and has been successfully decrypted, SpanCast has no way of knowing whether an unencrypted message was received on a SpanCast link that was configured for encryption. To address this potential security loop-hole, SpanCast adds a separate message authentication layer to every message transmitted.  Whenever SpanCast receives a message, it checks to authentication data embedded in the message and if authentication fails, it reports a diagnostic warning and drops the message from further processing.  This means that the `get()` function will only return messages that have been authenticated by SpanCast regardless of whether encryption is enabled or disabled.

> [!IMPORTANT]
> The keys SpanCast uses for message authentication are derived from the **password** setting used in the call to `configure()`, so even if you disable encryption, transmissions will only be successful between devices if the **password** settings match.

### WiFi Channel Auto-Synchronization (the *channelMask* and *numTries* settings)

WiFi networks operating at 2.4 GHz span fourteen overlapping channels numbered 1-14 with regional limitations: in North America channels 1-11 are generally allowed; outside of North America channels 1-13 can be used; and in Japan channel 14 is also available for use in certain circumstances.  In order for WiFi transmissions from one device to be reliably received by another device, the WiFi radios on both devices must be set to the same channel.[^wifi]

[^wifi]: If two devices are in close proximity, it is possible for messages to be successfully received even if the WiFi channels of the devices differ by one or two (e.g. channels 5 and 6).  However, reception will degraded.  To ensure the best fidelity and coverage, the devices should always be set to the same channel.

By default, SpanCast will refrain from making any changes to the WiFi channel used by the device.  Instead, SpanCast will transmit data on, and listen for data from, whatever WiFi channel happens to be set when the methods `send()` and `get()` are called.  This is the proper setting for any SpanCast device that will be connected to a central WiFi network (e.g. using `WiFi.begin()`), since the WiFi library itself takes care of setting the WiFi channel to ensure it stays synchronized with whatever channel is being used by your central network's WiFi router.  In such cases SpanCast simply uses the channel the WiFi library has selected.

However, SpanCast also includes an ability to dynamically changes the channel of a device's WiFi radio whenever the method `send()` is called in an attempt to match the WiFi channel used by the receiving device.  Usage of this option is the proper setting for any SpanCast device that is not itself connected to a central WiFi network, but that is transmitting to another SpanCast device that is connected to a central WiFi network.

Invoking this option is done during the call to `configure()` by setting the optional **channelMask** parameter to a non-zero value.  The **channelMask** parameter represents a 16-bit mask that allows you to specify which channels SpanCast should utilize when transmitting SpanCast messages.  For each bit N in **channelMask**, where N ranges from 1 to 14, if the bit is *set* SpanCast will attempt transmissions using WiFi channel N.  If bit N is *not set* SpanCast will not use that channel.  For example, setting the **channelMask** to 64 (0x0040) tells SpanCast to use only WiFi channel 6.  Setting the **channelMask** instead to 4094 (0x0FFE) tells SpanCast to use any channel from 1-11. Note that since there are no WiFi Channels 0 or 15, bits 0 and 15 of **channelMask** do not matter and are typically left as zero.

WiFi channel auto-synchronization applies whenever you transmit a message using `send()`.  SpanCast first tries to transmit the message using whatever WiFi channel is currently set.  If the transmission is successful `send()` exits and returns `true`.  If the transmission instead fails, SpanCast tries again using the same channel.  SpanCast will continue re-transmitting the message until either it succeeds or the number of tries equals the **numTries** parameter, which defaults to 3, but can be set to any value greater than zero[^numTries] during the call to `configure()`.

[^numTries]: If you set **numTries** to zero, SpanCast will reset it to 1 during the call to `configure()`

If SpanCast still fails to transmit successfully after **numTries** attempts, it changes the WiFi radio to next allowed channel based on the **channelMask** parameter, and repeats the above process.  In this fashion SpanCast will loop through all the WiFi channels allowed by the **channelMask** and attempt to transmit until either it succeeds or it has tried every WiFi channel allowed at least once.  For example, if the **channelMask** is set to allow only channels {1,3,5,6,9} and the WiFi channel happens to be set to 5 when `send()` is called, SpanCast will first try channel 5, followed by 6, 9, 1, and finally 3.  If at any point the transmission succeeds, `send()` saves the current channel in long-term NVS memory, exits, and returns `true`.  This ensures that `send()` always uses the most recent successful channel when it is next called, even if power is cycled.[^channelStartup]  Unless the receiving device changes the channel it is using, `send()` will generally not have to probe different channels on subsequent calls.

[^channelStartup]: Provided that **channelMask** is non-zero, SpanCast will read the last-used WiFi channel stored in long-term NVS memory during the call to `configure()` and set the WiFi channel to that value with one exception: if the current **channelMask** does not allow that channel to be used, SpanCast will instead set the WiFi channel to the lowest channel number allowed by **channelMask**.  This may occur, for example, if you change your sketch to reduce the the number of channels allowed by **channelMask** such that a previously-used channel that is stored in NVS is no longer allowed.

However, if after trying channel 3 (the last channel allowed by the **channelMask** in this example) the transmission is still unsuccessful, SpanCast will reset the channel back to 5 (where it started), terminate `send()` and return `false`.

#### Use Cases for the *channelMask* Setting

The most common use case is a network topology where one or more standalone SpanCast devices serve as remote sensors transmitting data to a central SpanCast device that is connected to a central WiFi network, perhaps also connected to HomeKit, or maybe just running a Web Server to display the results of the remote sensors.  In this topology the **channelMask** on the device connected to the central WiFi network should either be set to zero or simply left unspecified (since zero is the default).  On all other devices, the **channelMask** should be set to use the widest range of allowable channels in your region.  This ensures each remote device will auto-synchronize its WiFi channel to match that of the central device whenever it transmits to that device.  As a result, every device will be synchronized to the same WiFi channel, which enables direct SpanCast transmissions from one remote device to another remote device. 

In topologies where *none* of the devices are connected to a central WiFi network, there are numerous possible configurations you can chose:

1. Pick a WiFi channel to use and set the **channelMask** on each device to allow just that channel.  Every device will always transmit (and listen for transmissions) on only that channel.  The advantage of this configuration is that there is no need for any channel synchronization between devices, so it does not matter if some device only receive data and never transmit (in which case there would be no opportunity of such a device to synchronize to other devices).  The disadvantage of this configuration is that if you want to use a different channel, you need to update **channelMask** and reload your sketches with the new value for every device.

1. Arbitrarily assign one of the devices to act as the "anchor" for channel synchronization by picking a WiFi channel to use for your network and setting the **channelMask** on only the anchor device to allow just that channel.  Then on all other devices, set the **channelMask** to use the widest range of channels allowed in your region.  The advantage of this configuration is that you only need to set the channel on one device and every other device auto-synchronizes to that channel.  If you later want to use a different channel you only need to update the sketch on the anchor device and all other devices will re-sync to that channel they next time they transmit data to the anchor device.  The disadvantage of this configuration is that it can only be used if every device is transmitting data to the anchor device, or if at least every device is transmitting to another device that is, in turn, transmitting the the anchor device, and so forth.

1. Arbitrarily assign one of the devices to act as the "anchor" for channel synchronization, but leave the **channelMask** unspecified on that device.  Then on all other devices, set the **channelMask** to use the widest range of channels allowed in your region.  This is basically the same as the configuration above, but instead of setting the WiFi channel on the anchor device to a value of you specify, the channel on that device will be set to the default start-up value determined by the Arduino-ESP32 core (usually channel 1).

1. Leave the **channelMask** unspecified on all of the devices, in which case they will each default to the same start-up value determined by the Arduino-ESP32 core (usually channel 1).  This is the method used in the Basic Transmitter/Receiver example above.

#### Further Considerations

* WiFi synchronization only occurs when a device transmits data.  If you have a SpanCast device that only receives data that is *not* an anchor device (i.e. one that has a fixed channel or whose channel is separately controlled by a central WiFi network), there will be no way for the device to synchronize its channel with the rest of the network.  In such case you should create a dummy variable to periodically transmit a short message (1 byte is sufficient) to some other device on your network that is synchronized with other devices or is itself the anchor device.  This dummy variable *does not* need to be read or consumed by that other device.  As long as the first device transmits something to the other device, it will be able to synchronize its channel to that device even if that other device does not have an instance of a SpanCast object that is expecting to receive any data from the first device. 

* In any network of SpanCast devices there should be at least one anchor device with its **channelMask** set to zero (or left unspecified, since zero is the default) or set to allow only a single channel.  If every device in your SpanCast network has a **channelMask** that allows multiple channels to be used during synchronization, the network will likely not settle down on a specific channel as each device is trying to synchronize its WiFi channel to every other device without any fixed channel as a reference.

* Both the ESP32 and ESP8266 microcontrollers have built-in logic that encode the region of the chip and the allowed channels for that region.  SpanCast uses this information during configuration to filter out from the **channelMask** any channels that are not allowed according the chips encoded region.  For example, if the encoded region for your chip is in North America and you set the **channelMask** to 0x3FFE (allowing for channels 1-13), **channelMask** will be reset to 0x0FFE with the call to  `SpanCast::configure()` to exclude channels 12 and 13 since they are not allowed in North America.  Caution: though tempting, do not simply set the **channelMask** to 0xFFFF and allow SpanCast to filter out the disallowed channels based on the region code for your chip. ESP32 and ESP8266 chips are shipped world-wide and there is no guarantee that device you have has been pre-coded with the region where the device will be used.  It's always best set the **channelMask** to allow only those channels you know are permitted in your region.  Since WiFi channels 1-11 are permitted everywhere, you may want to start with that for your **channelMask** on all devices except your anchor device.

#### Channel Mask Helper Functions

SpanCast provides two helper functions that convert a range or list of channels into a **channelMask**:

* `static uint16_t range(uint8_t start, uint8_t end)`

  * returns a **channelMask** allowing all channels in the range [**start**,**end**]
  * example: `SpanCast::configure(3,{.channelMask=SpanCast::range(1,11)});`

* `static uint16_t list(std::initializer_list<uint8_t> list)`

  * returns a **channelMask** allowing only those channels specified in **list**
  * example: `SpanCast::configure(3,{.channelMask=SpanCast::list( {1,3,5,6,9} )});`

## Optional Settings when calling the `SpanCast()` constructor

The sections below fully describe the per-link settings structure, *`pCast_t`, that can be passed to the `SpanCast()` constructor as an optional fourth argument.

### Per-Link Encryption (the *encrypt* setting)

This per-link setting allows you to enable encryption on some SpanCast links but disable it for others, while still retaining global control.  A specific link will be encrypted if *either* the global **encrypt** setting specified in the initial call to `configure()` is set to `true` (the default), *or* the **encrypt** setting specified in the SpanCast constructor for that link is set to `true`.  Encryption for a specific link will be disabled only if *both* the global **encrypt** setting *and* the **encrypt** setting specified in the SpanCast constructor for that link are set to `false`.

Since the per-link **encrypt** setting for each SpanCast object defaults to `false` if left unspecified when the constructor is called, the global **encrypt** setting will generally govern whether any specific link will be encrypted.  To enable encryption for only a subset of SpanCast links, first set the global **encrypt** setting to `false` in the call to `configure()` and then set the per-link **encrypt** setting to `true` in the constructor for only those links you want encrypted.

### Message Receiving Queues (the *queueDepth* setting)

When a SpanCast link object is constructed with a non-zero value for **receiveSize**, SpanCast creates a FIFO ("first-in-first-out") *receiving queue* to store incoming messages directed to that link for later retrieval by the `get()` method.  For each link, the **queueDepth** setting determines the size and behavior of this queue.

* If **queueDepth** is set to zero (the default), SpanCast stores only the last message received and overwrites that message if a new one arrives before the first one is read by `get()`.  This ensures the call to `get()` will always yield the most recent message received for that link.

* If **queueDepth** is set to any number greater than zero, SpanCast reserves space in the queue to store **queueDepth** messages.  Each call to `get()` returns a single message (in FIFO order), which reduces the number of messages remaining in the queue.  However, if the queue is fully filled before `get()` is called, SpanCast will drop any new messages received until a subsequent call to `get()` opens up more space.

In a typical sketch `get()` is called much more frequently than incoming messages are received so the default setting (**queueDepth**=0) should work fine.

### Active/Inactive Receiving Status (the *timeout* setting)

For each link, SpanCast retains the time of the most recent message received to ascertain whether the remote device that is supposed to send such messages is active or inactive.  This time is snapped upon initial validation of the message and does not depend on whether the message has been stored in that link's receive queue (e.g. the queue is full) or has yet been retrieved by calling the `get()` method.

The active/inactive state determined by SpanCast can be read by calling the `isActive()` method for any SpanCast object.  This function returns `true` if the last message received for that link has been within the last **timeout** milliseconds, else the function returns `false`.

The **timeout** setting defaults to 60,000 (i.e. 60 seconds) if left unspecified when constructing a SpanCast link.  Use of `isActive()` is completely optional, though if you decide to use it in your sketch to estimate whether a remote device may have gone off-line or is out of range, you should set **timeout** to be *longer* than the period that the remote device is expected to send messages.  For example, if you have a remote sensor that is programmed to send temperature readings every 30 seconds, setting **timeout** to 90,000 for the link that is receiving those messages means that the call to `isActive()` will return `true` even if an occasional message is dropped, but will return `false` after an extended time of not receiving any messages from the remote device has passed.

Note that upon start-up `isActive()` will return `false` for any given link until that link has received its first message.  Also note that `isActive()` only pertains to messages received, not messages transmitted, and thus will always return `false` for a SpanCast link configured as transmit-only (i.e. **receiveSize**=0).

# SpanCast Errors, Warnings, and Diagnostic Messages

SpanCast uses the ESP32's standard Log Library to provide 4 levels of diagnostic messages: *Error*, *Warn*, *Info*, and *Debug*.  You can select the level of logging under the *Tools → Core Debug Level* menu of the Arduino IDE.  Select *Error* or *Warn* for minimal, but important, output.  Select *Info* if you'd like to see the inner workings of internal SpanCast processes, such as WiFi synchronization.  The *Debug* level provides even more details that are useful for debugging the library itself.  Note you must recompile your sketch after selecting a new logging level for it to take effect.  

Since the legacy ESP8266 does not include a Log Library, SpanCast prints its log messages to the Serial Monitor using `Serial.printf()`.  To see those messages you must therefore include a call to `Serial.begin()` near the top of your sketch.  Also, the Arduino IDE does not provide log "levels" but instead allows users to select various combination of functions, such as WIFI or OTA, for logging.  To emulate the ESP32 Log Library, SpanCast maps the following ESP8266 log functions (selectable under the *Tools → Debug Level* menu of the Arduino IDE when compiling for an ESP8266) to ESP32 log levels as follows:

|ESP8266 Level|ESP32 Log Level|
|---|---|
|CORE|*Error*|
|WIFI|*Warn*|
|HTTP_UPDATE|*Info*
|UPDATE|*Debug*
|OTA|*Verbose* (unused by SpanCast)

# Maximum Number of SpanCast Links

There is no limit to the number of *unencrypted*, *receive-only* SpanCast links you can create (i.e **encrypt**=`false` *and* **sendSize**=0). For encrypted links (i.e. **encrypt**=`true`) as well as any link that can transmit data (i.e. **sendSize**>0) the following limits apply:

|Chip|Encrypted Links|Total Links|
|:---:|:---:|:---:|
|ESP32|7|20|
|ESP8266|10|20|

# Advanced "Weather Station" Example

In addition to the Basic Transmitter/Receiver example sketches presented above in a prior section, the SpanCast library also includes a more detailed "Weather Station" example that demonstrates bi-direction communication among three different devices: a Wind Speed Sensor, a Temperature/Humidity Sensor, and a Barometric Pressure Sensor.  Sketches for each device can be found in the Arduino IDE under *File → Examples → SpanCast → WeatherStation*.

Note the sketches only include *simulated* data as they are designed to illustrate how to use the different optional settings of SpanCast, not to actually take weather measurements.  The general setup for the three devices is as follows:

* the Temp/Hum device, the Wind Speed device, and the Barometer device, have each been assigned a *Device ID* of 1, 2, and 3, respectively

* for all three devices, the *Network ID* has been set to 4.  This ensures no interference with the Basic Receiver example sketch that also sets its *Device ID* to 3, but retains the default *Network ID* of 1

* the WiFi channel for the Wind Speed device has been set to a fixed value of 5

* the **channelMask*** for both the Temp/Hum device and Barometer device have been set to autosynchronize allowing any channel from 1 to 11.  When either of these two devices send data to the Wind Speed device for the first time, they will each automatically synchronize to WiFi channel 5, and use that channel for all future transmissions, even if the power is cycled.[^channel5]  However, if a transmission fails, perhaps because you power off the Wind Speed device, the autosynchronization on these devices will try every channel from 1 to 11 until the Wind Speed device is powered on again

* Encryption remains globally enabled on the Temp/Hum device (which is the default), but has been globally disabled on the Wind Sensor and Barometer devices.  However, in order for those devices to read the encrypted transmissions received from the Temp/Hum device, encryption is re-enabled on the specific links to the Temp/Hum device[^disable]

* the Temp/Hum device, the Wind Speed device, and the Barometer device, each update their simulated data every 5, 8, and 13 seconds, respectively.  After each update, the device transmits its new data to the other two devices, and then prints to the Serial Monitor an updated view of all the weather data it knows about

* each device repeatedly checks to see if there is new data received from any of the other devices, and if so, it displays an updated view of all the weather data on the Serial Monitor

* each device tracks whether it has been more than 60 seconds (the default) since data was received from the other devices.  If not, it prints a warning message to the Serial Monitor and displays that data as "--" in the Serial Monitor rather than use an outdated value.

[^channel5]: If your devices are in very close proximity it may be possible for transmissions to succeed even if the WiFi channels on the devices differ by one or two.  In such cases the Temp/Hum device and Barometer device may autosynchronize to channel 4 (and maybe even channel 3) if the transmissions using those channels are sufficiently reliable.  However, if this occurs and you start to increase the distance between your devices, you should see the autosynchronization update to WiFi channel 5 across all the devices as SpanCast finds transmissions on channel 4 (or channel 3) begin to fail.

[^disable]: Note what happens if you alter the sketch and do not re-enable encryption on one of those links, say from the Wind Speed device to Temp/Hum device: the un-encrypted transmissions sent from the Wind Speed device to the Temp/Hum device will still be properly read by the Temp/Hum device, but the encrypted transmissions sent from the Temp/Hum device to the Wind Speed device will be ignored since the Wind Speed sensor will not be able to read the encrypted transmission with encryption disabled.

> [!TIP]
> Example sketches showing how SpanCast can be used to create remote sensors that interact directly with HomeKit are provided as part of the [HomeSpan](https://github.com/HomeSpan/HomeSpan) Library.

# SpanCast MAC Addresses and WiFi Interfaces

SpanCast utilizes Espressif's ESP-NOW protocol for transmitting and receiving data.  ESP-NOW in turn uses the unique MAC addresses assigned in the WiFi stacks of the devices to identify which transmissions are intended for which recipients.  Implementing ESP-NOW directly in a sketch therefore requires you to either hardcode the MAC addresses of all the devices in your network, or to use a generic broadcast MAC address so that you transmit the same data to all device at the same time.  Unfortunately, each of the methods has drawbacks:

* Hardcoding MAC addresses into a sketch makes that sketch non-portable.  If you replace one device on your network you need to update the sketches on every other device that either transmits data to, or receives data from that new device, with that new device's specific MAC address.

* Using a broadcast MAC address to avoid hardcoding individual MAC addresses into a sketch means giving up ESP-NOW encryption and, even more importantly, the ability to easily verify whether transmissions were successfully received by their intended recipients.  SpanCast uses this ability to automatically synchronize WiFi channels across all of your devices, thereby ensuring your SpanCast data transmissions are successfully received.

To solve these issues, SpanCast abstracts away the MAC addresses and instead uses the concept of a simple, user-definable SpanCast *Device ID* (and optional *Network ID*) that you assign to your devices in each sketch, as fully described in the sections above.  Under the hood, SpanCast resets the MAC address for the device during the call to `configure()` based on the specified *Device ID* and *Network ID* as follows:

* byte 0 of the MAC address is always set to F2, consistent with a MAC address that is *unicast* (least-significant bit is set zero) and *locally administered* (second-to-last bit is set to one)

* byte 1 is set to the specified 8-bit *Network ID*

* bytes 2 and 3 are set to the specified 16-bit *Device ID* (stored in little-endian order)

* byte 4 is set as the XOR of bytes 0 and 2

* byte 5 is set as the XOR of bytes 1 and 3

For example, SpanCast will set the MAC address of a device with *Device ID=1* and *Network ID=4* (i.e. the Temp/Hum example above) to F2:04:01:00:F3:04.  SpanCast displays this value in the Serial Monitor during the call to `configure()` if the log level is set to *Info* or greater.

## STA vs AP Interfaces

The WiFi stacks on both the ESP32 and ESP8266 each support an STA interface (used to connect the device to a central WiFi network) and an AP interface (used to configure the device as an Access Point). On the ESP32, SpanCast utilizes the AP interface and therefore changes the MAC address only for that interface, leaving the MAC address for the STA interface untouched.  This is desirable since many WiFi routers can be configured to recognize the MAC addresses of the devices that are connected (or trying to connect) and may display a custom name for that device or may even determine whether to allow the device to join the network.  By utilizing the AP interface, SpanCast can freely change the MAC address of the interface without impacting the STA MAC address or how it may be used for recognition and permissioning by the central router to which the device is connected.

However, on the ESP8266 SpanCast must use the STA interface as there are instabilities in changing the MAC address of the AP interface.  Since the ESP8266 is a legacy device, there is no expectation that this will ever be resolved.  This does not mean you cannot use SpanCast on an ESP8266 that is also connected to a central WiFi network.  But is does mean that the connection may not operate as expected if your central router has been configured to require a specific MAC address for the device to connect.

Fortunately, SpanCast's use of the AP interface on the ESP32 versus its use of the STA interface on the ESP8266 does not impede the ability of SpanCast to transmit data between ESP32 nd ESP8266 devices. The transmissions themselves do not encode which interface was used by the sender.

> [!IMPORTANT]
> When `SpanCast::configure()` is called in your sketch, SpanCast enables the AP interface, but does not touch the STA interface.  If it is already enabled, it will remain enabled.  If it is disabled, it will remain disabled.  The `WIFI.begin()` functionality commonly used to connect a device to a central WiFi network acts similarly except that it enables the STA interface and does not touch the AP interface.  As such, you may call `configure()` to start SpanCast either before or after connecting to a central network using `WIFI.begin()` or any other WiFi connection library that enables the STA interface without touching the AP interface.
>
> However, if you use a library that sets the WiFi mode of the device to WIFI_STA (i.e. STA only) when trying to connect to a central network, or you call `WIFI.mode(WIFI_STA)` directly in your sketch, this will disable the AP interface.  In such cases the solution is to call `SpanCast::configure()` *after* you connect to the central network so SpanCast can properly enable the AP interface.
>
>If in your sketch you must call `SpanCast::configure()` *before* connecting to a central WiFi network using a library that *turns off the AP interface*, you can always re-enable it by calling `WIFI.enableAP(true)` *after* the connection library is called.  Setting the log level to *Warn* or greater when you compile your sketch will provide diagnostic messages to aid in debugging such situations.

# SpanCast API Reference

Below is a summary reference of the full SpanCast API with a brief description of each static class-level function and each non-static object-level method.  Messages reporting errors, warnings, and other diagnostic info are output to the Serial Monitor according to the log level specified under the *Tools → Core Debug Level* menu (for the ESP32) or the *Tools → Debug Level* menu (for the ESP8266) in the Arduino IDE when the sketch is compiled.

### Static Class-Level Functions

#### `static boolean configure(uint16_t deviceID [, SpConfig_t cfg])`

* configures SpanCast and sets the SpanCast *Device ID* of the device to **deviceID**
* this function must be called before any SpanCast objects can be created
* returns `true` if configuration succeeds, else returns `false`
* specifying the *optional* parameter **cfg** allows any of the following global SpanCast settings defined in the `SpConfig_t` structure to be overridden:

  * `uint8_t` **network** - sets the SpanCast *Network ID* of the device (default=1)

  * `String` **password** - sets the encryption password (default="spancast123")
  * `boolean` **encrypt** - enables global encryption if `true`, otherwise disables global encryption (default=`true`)
  * `uint16_t` **channelMask** - sets the bitmask specifying the WiFi channels (from 1 through 14; bits 0 and 15 are ignored) SpanCast can use during WiFi autosynchronization. Setting just a single bit results in SpanCast always using the corresponding channel.  Setting **channelMask** to zero disables autosynchronization and prohibits SpanCast from altering the WiFi channel (default=0)
  * `uint8_t` **numTries** - sets the number of times SpanCast attempts to transmit a message on any given WiFi channel before repeated failures cause it to try another channel.  Only applicable if **channelMask** is non-zero (default=3)

* note **cfg** can be defined as a separate variable of type `SpConfig_t` or simply expressed as a brace-enclosed initialization list specifying the settings to override (in the order listed above)

  * example: `SpanCast::configure(5, {.encrypt=false, .numTries=5});`

#### `static uint16_t list(std::initializer_list<uint8_t> list)`

* returns a **channelMask** representing the specified **list** of WiFi channels
* example: `SpanCast::configure(5 ,{ .channelMask=SpanCast::list({1,6,11}) });`

#### `static uint16_t range(uint8_t start, uint8_t end)`

* returns a **channelMask** representing all WiFi channel from **start** to **end**, inclusive

* example: `SpanCast::configure(5 ,{.channelMask=SpanCast::range(1,11)});`

#### `static int maxMessageSize()`

* returns the maximum message size that SpanCast can transmit or receive

* set to 1434 bytes for ESP32 devices and 214 bytes for ESP8266 devices

#### `static const char *version()`

* returns the SpanCast version as a string in the standard form "X.Y.Z"

### Non-static Object-Level Methods

#### `SpanCast(uint16_t deviceID, size_t sendSize, size_t receiveSize [, SpCast_t settings])`

* constructor to create a new SpanCast object representing a link that transmits **sendSize** bytes to, and/or receives **receiveSize** bytes from, a destination device with a *Device ID* of **deviceID**

* specifying the *optional* parameter **settings** allows any of the following per-link SpanCast settings defined in the `SpCast_t` structure to be overridden:

  * `boolean` **encrypt** - determines whether message encryption is enabled or disabled for this link.  If *either* **encrypt** is set to `true` *or* encryption has been enabled at the global level, encryption for this link will be enabled.  If *both* **encrypt** is set to `false` *and* encryption has been disabled at the global level (by setting **encrypt** to `false` in the call to `configure()`), encryption for this link will be disabled (default=`false`)

  * `size_t` **queueDepth** - sets the depth of the message receive queue to **queueDepth**. A value of zero causes new messages to overwrite any existing message already in the queue. A value greater than zero stores that number of messages and discards any new ones until messages are extracted from the queue with a call to `get()` (default=0)
  * `uint32_t` **timeout** - if SpanCast has not received any valid messages for this link in **timeout** milliseconds, the link will be flagged as *inactive* (default=60000, i.e. 60 seconds)

* note **settings** can be defined as a separate variable of type `SpCast_t` or simply expressed as a brace-enclosed initialization list specifying the settings to override (in the order listed above)

  * example: `SpanCast sensorDevice(7, 0, sizeof(uint16_t), {.queueDepth=3});`

* if the constructor is called before SpanCast has been successfully configured, or if any combination of the required or optional parameters specified are found to be invalid, SpanCast will create the object but flag it as *uninitialized*

* the `bool()` operator for a SpanCast object has been overloaded to return `true` if the link has been initialized, else `false` if it is uninitialized

  * example: `if(!sensorDevice) Serial.printf("Sensor not initialized!");`

#### `boolean send(const void *data)`

* if the link is not initialized *or* the link's **sendSize** parameter is zero, SpanCast simply exits the function and returns `false`

* otherwise, SpanCast transmits the data referenced by **data** to the destination device having a *Device ID* equal to the link's **deviceID** parameter

  * a value of `true` is returned if SpanCast receives an acknowledgement from the WiFi stack of the intended destination device that it has received the data, *regardless of whether or not the destination device was able to properly decrypt the data, process the data, store the data in a receive queue, etc.*

  * a value of `false` is returned if, after trying to transmit the data **numTries** times for each WiFi channel allowed by the **channelMask**, SpanCast nevertheless fails to receive an acknowledgement from the WiFi stack of the intended destination device that it has received the data[^sendFail]

* note that SpanCast expects (but does not check!) that **data** points to a valid memory location containing at least **sendSize** bytes of data

[^sendFail]: It is possible that the acknowledgement a destination device transmits back to the sending device to confirm receipt of the data has itself failed to reach the sending device.  In such cases the sending device will incorrectly believe the transmission did not succeed and, following its protocol, will repeat the transmission in accordance with the **numTries** and **channelMask** settings.  To prevent potentially duplicate messages being returned when `get()` is called on the destination device, the `send()` method adds a unique *nonce* to every message and the data handler on the receiving side drops any *consecutive* messages it receives from the same sending device with that same *nonce* so that a call to `get()` only returns the first copy of the duplicated message.

#### `boolean get(void *dataBuf)`

* if the link is not initialized *or* the link's **receiveSize** parameter is zero, SpanCast simply exits the function and returns `false`

* otherwise, SpanCast checks the link's receive queue for data

  * if a message is available SpanCast moves the data from the receive queue to **dataBuf** and returns `true`
  
  * if the queue is empty SpanCast does nothing and returns `false`

* note that SpanCast expects (but does not check!) that **dataBuf** points to a valid memory location sized to store at least **receiveSize** bytes of data

#### `boolean isActive()`

* if the link is not initialized *or* the link's **receiveSize** parameter is zero, SpanCast simply exits the function and returns `false`

* otherwise, SpanCast returns `true` if a message has been received for this link within the last **timeout** milliseconds, else it returns `false`

* this function will always return `false` upon start-up until at least one message has been received for this link





