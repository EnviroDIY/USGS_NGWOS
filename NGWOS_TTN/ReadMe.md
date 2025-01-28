# The Things Network

This program transmits data from a Vega Puls 21 and onboard sensors from a Stonefly data logger to The Things Network via LoRa.

- [The Things Network](#the-things-network)
  - [Physical Connections](#physical-connections)
  - [Library Dependencies](#library-dependencies)
  - [Setting up in The Things Network](#setting-up-in-the-things-network)
  - [Downloading and Setting up the Example Sketch](#downloading-and-setting-up-the-example-sketch)
  - [Customizing the Example Sketch](#customizing-the-example-sketch)
    - [ArduinoJSON 6 vs 7](#arduinojson-6-vs-7)
    - [Vega Puls and Hydros 21](#vega-puls-and-hydros-21)

## Physical Connections

This program is written for an EnviroDIY Stonefly, a Vega Puls 21, a Meter Hydros 21, and a MultiTech mDot.

The physical connections needed are nearly identical to those described in the [Monitor My Watershed ReadMe](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_Hydros21_HydroCam), but with the mDot replacing the modem and no camera.

The mDot should be installed in the "bee" socket on the Stonefly.
The antenna connection for the mDot should be at the top, so that the antenna is hanging off the end of the board.

The Vega Puls should be connected to a screw-terminal-to-Grove adapter.
The Grove plug from the Vega Puls should be connected to one of the two sockets labeled "D2-3 & SDI12" on the Stonefly.

- The *brown* wire of the Vega Puls should be connected to the "V+" connection of the screw terminal.
- Both the *blue* and thick black/grey *shield* wires should be connected to "Gnd" on the screw terminal.
- The *white* SDI-12 data wire should be connected to the "S2" connection of the screw terminal.
- There should be nothing connected to the "S1" connection of the screw terminal.

The Hydros21 should be connected to either a screw-terminal-to-Grove adapter or a Grove-to-headphone adapter.
The Grove plug from the Hydros21 should be connected to the D2-3/SDI12 Grove plug on the bottom left of the Stonefly.
*Use the Grove socket all the way on the bottom, not the 12V plugs used by the Vega and HydroCam.*
The jumper next to the plug should be connecting the middle two pins to supply the Hydros21 with 3.3V.
The Hydros21 can use a 12V power supply, but it is also satisfied with only 3.3V.
The Vega and HydroCam need the 12V, so save those plugs for them.

- The *brown* wire of the Hydros21 should be connected to the "V+" connection of the screw terminal.
- The *bare grey* wire should be connected to "Gnd" on the screw terminal.
- The *orange* SDI-12 data wire should be connected to the "S2" connection of the screw terminal.
- There should be nothing connected to the "S1" connection of the screw terminal.

## Library Dependencies

For this program to compile, you need to have the following libraries and versions installed:

- envirodiy/LoRa_AT@^0.3.0
- sparkfun/SparkFun Qwiic RTC RV8803 Arduino Library@^1.2.9
- envirodiy/SDI-12@^2.2.0
- adafruit/Adafruit BusIO@^1.16.1
- adafruit/Adafruit Unified Sensor@^1.1.14
- adafruit/Adafruit SHT4x Library@^1.0.5
- adafruit/Adafruit MAX1704X@^1.0.3
- vshymanskyy/StreamDebugger@^1.0.1
- bblanchon/ArduinoJson@^6.21.5
  - NOTE: This is *NOT* the latest version of the ArduinoJson library! Use version 6! Version 7 will *probably* work, but the CayenneLPP depends on version 6.
- electroniccats/CayenneLPP@^1.4.0
- greiman/SdFat@^2.3

You can download all of these dependencies together in the [zip file](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/AllDependencies.zip) in the repository main folder.
After unzipping the dependencies, move them all to your Arduino libraries folder.
Instructions for finding your libraries folder are [here](https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer).

If you are using PlatformIO, you can copy this list above into the lib_deps section of your platformio.ini or use the example ini file in this folder.

## Setting up in The Things Network

- Create a new application on The Things Network following [these instructions](https://www.thethingsindustries.com/docs/integrations/adding-applications/).
  - You do not need an application API key or any payload encryption or decryption in this application.
  - If you already have an application you want to join the data from this program to, feel free to use it.
- After creating the application, [create a default payload formatter](https://www.thethingsindustries.com/docs/integrations/payload-formatters/create/) for the application.
  - Select "Custom Javascript Formatter"
  - Add the code from either [LoRaNotes/TTNDecoder.js](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/NGWOS_TTN/LoRa%20Notes/TTNDecoder.js) or [LoRaNotes/TTNDecoder_withInstruments.js](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/NGWOS_TTN/LoRa%20Notes/TTNDecoder_withInstruments.js)as the formatter code.
    - This custom formatter is an expansion of the original CayenneLPP formatter.
    - The "with instruments" version adds fields for the sensor model, specific parameter, and units. These fields will only be valid for *this specific example sketch*. The other version will work to expand any Cayenne LPP format.
  - If you did not create a new application for this example, you can [add the formatter as a device specific formatter](https://www.thethingsindustries.com/docs/integrations/payload-formatters/create/#create-a-device-specific-payload-formatter) after registering the end device.
- Now [register a new end device](https://www.thethingsindustries.com/docs/hardware/devices/adding-devices/) on your application.
  - Select "Enter end device specifics manually"
    - Frequency plan: United States 902-928 MHz, FSB 2 (used by TTN)
    - LoRaWAN version: LoRaWAN Specifications 1.0.2
      - You should select the specification based on the specifications and firmware on your module. The mDOT and LoRa-E5 I have both use 1.0.2 in their latest firmware.
    - Regional Parameters version: RP001 Regional Parameters 1.0.2 revision B
      - Again, modify this if necessary for your module.
  - In the advanced settings:
    - Selection "Over the air activation (OTAA)
    - Additional LoRaWAN Class capabilities: None (class A only)
    - Network defaults: Check
    - Cluster settings: Uncheck
  - Provisioning information:
    - JoinEUI (also called App EUI)
      - *For a MultiTech mDot, the Join EUI (AppEUI) is programmable*. That means it is not set or specified on the module packaging. I suggest you create a new random EUI for your module using [this random generator](https://descartes.co.uk/CreateEUIKey.html). Just click the "Create EUI" button.
      <!-- - For my Seeed LoRa-E5, the App EUI is confusingly labeled "KEY" on the product sticker -->
      - This must be exactly 16 hex characters (8 bytes = 64 bits).
    - DevEUI (Device EUI)
      - For an MultiTech mDot, the DevEUI is on the product sticker labeled as the "NODE"
      <!-- - For my Seeed LoRa-E5, the DevEUI is labeled as "EUI" on the product sticker -->
      - This must be exactly 16 hex characters (8 bytes = 64 bits).
    - AppKey (also called Network Key)
      - Click "Generate" to get a new, unique network key. You could also use [this random generator](https://descartes.co.uk/CreateEUIKey.html) again with the "Create Key" button.
    - End device ID
      - This can be any combination of lower case letters, numbers, and dashes you want to use to identify the device. I suggest using the model name and the serial number.
      - You can change this device ID later if you wish.
  - Once you've entered everything, click the blue "Register end device" button.

## Downloading and Setting up the Example Sketch

For this example to work, you must download the entire contents of the [TheThingsNetwork folder](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_TTN/TheThingsNetwork) and put them into the folder with your sketch.
Without *all* of the files in the folders, the sketch will not compile.

If you are using the default Arduino sketch folder it should look like this:

```txt
C:\Users\{your_user_name}\Documents\Arduino
    └ The Things Network
        └ LoRaModemFxns.h
        └ SDI12Master.h
        └ TheThingsNetwork.ino
        └ src
            └ LoggerBase.h
            └ LoggerBase.cpp
            └ ModSensorDebugger.h
            └ WatchDogSAMD.h
            └ WatchDogSAMD.cpp
```

## Customizing the Example Sketch

Once you've set up your end device in The Things Network and set up your sketch folder, you need to customize the ino file to send data to your application.

- Open the example TheThingsNetwork.ino in the Arduino IDE (or PlatformIO/VSCode).
- Scroll to lines 100 - 106 in the program.
- Modify the appEui and appKey in the code to match the settings for your new end device in The Things Network

```cpp
// Your OTAA connection credentials, if applicable
// The App EUI (also called the Join EUI or the Network ID)
// This must be exactly 16 hex characters (8 bytes = 64 bits)
const char appEui[] = "YourAppEUI";
// The App Key (also called the network key)
// This must be exactly 32 hex characters (16 bytes = 128 bits)
const char appKey[] = "YourAppKey";
```

While you're customizing the sketch, you should also check/adjust the logging interval in line 71.

You should *not* change anything in the cpp/h files in the sketch folder.

> [!NOTE]
> This program has pins and timings set for a MultiTech mDot and EnviroDIY Stonefly.
> To use it with a different module or logger board, you would need to do more heavy modification of the program.

### ArduinoJSON 6 vs 7

This program will compile and run with either ArduinoJSON 6 or 7, but you will get processor warnings with v7.
There are some syntax changes between the two versions of ArduinoJSON.
The [latest version of this example sketch on GitHub](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/NGWOS_TTN/TheThingsNetwork/TheThingsNetwork.ino) attempts to automatically detect the version of the ArduinoJSON library with defines and use the appropriate syntax for it.
If you are getting compile *errors* that appear to be related to ArduinoJSON find and modify the block of code starting with the comment `// Initialize a buffer for decoding Cayenne LPP messages` (around lines 168-173):

For ArduinoJSON **7**, use this:

```cpp
// Initialize a buffer for decoding Cayenne LPP messages
JsonDocument jsonBuffer;  // ArduinoJson 7
```

For ArduinoJSON **6**, use this:

```cpp
// Initialize a buffer for decoding Cayenne LPP messages
DynamicJsonDocument jsonBuffer(1024);  // ArduinoJson 6
```

More detail on the difference between the two versions and migration is available [here](https://arduinojson.org/v7/how-to/upgrade-from-v6/).
This example has warnings for version 7 because the [Cayenne LPP library](https://github.com/ElectronicCats/CayenneLPP) used to compress data and send it over LoRa was built for the smaller ArduinoJSON 6 library.


### Vega Puls and Hydros 21

This program includes code for both the Vega Puls and the Hydros 21.
To enable the Vega Puls, remove the leading double slashes (`//`) before `#define USE_VEGA_PULS` in line 14.
To enable the Hydros 21, remove the leading double slashes (`//`) before `#define USE_METER_HYDROS21` in line 15.
Either sensor can be disabled by adding slashes to comment out the define.

To use the Vega Puls and Hydros 21 together, follow the instructions in the [Monitor My Watershed sketch ReadMe](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_Hydros21_HydroCam#using-the-vega-puls-and-meter-hydros21-together).
