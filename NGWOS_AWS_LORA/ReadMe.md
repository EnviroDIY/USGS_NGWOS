# AWS IoT Core over LoRa

This program transmits data from a VEGAPULS C 21 and onboard sensors from a Stonefly data logger to AWS IoT Core via LoRa. It also takes images with a Geolux Hydrocam and saves them to an SD card. The images are **not** transmitted anywhere, only saved locally.

- [AWS IoT Core over LoRa](#aws-iot-core-over-lora)
  - [Physical Connections](#physical-connections)
  - [Downloading the Program and Setting up the IDE](#downloading-the-program-and-setting-up-the-ide)
    - [Arduino IDE](#arduino-ide)
      - [Installing Library Dependencies](#installing-library-dependencies)
    - [PlatformIO/VSCode](#platformiovscode)
  - [Setting up in AWS](#setting-up-in-aws)
  - [Customizing the Example Sketch](#customizing-the-example-sketch)
    - [Set your Thing Name](#set-your-thing-name)
    - [Set your LoRa connection Credentials](#set-your-lora-connection-credentials)
  - [Upload to your Stonefly](#upload-to-your-stonefly)
  - [Further Reading: ArduinoJSON 6 vs 7](#further-reading-arduinojson-6-vs-7)

## Physical Connections

This program is written for an EnviroDIY Stonefly, a VEGAPULS C 21, a Geolux Hydrocam, and a MultiTech mDot.
Instructions for assembling the components are available in [this blog post](https://www.envirodiy.org/river-flood-monitoring-system-for-rapid-deployment/).
Scroll down to the "Assembly" steps.

The mDot should be installed in the "bee" socket on the Stonefly.
The antenna connection for the mDot should be at the top, so that the antenna is hanging off the end of the board.

## Downloading the Program and Setting up the IDE

For this example to work, you must download the entire contents of the [NGWOS_AWS_LORA folder](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_AWS_LORA/NGWOS_AWS_LORA) and put them into the folder with your sketch.
Without all of the files in the folders, the sketch will not compile.

### Arduino IDE

- Using Windows Explorer (or a folder/file manager), navigate to your Sketchbook folder (or your desired working folder).
  - The default folder for Windows is `C:\Users\{username}\Documents\Arduino`; instructions to find your folder if needed are [here](https://support.arduino.cc/hc/en-us/articles/4412950938514-Open-the-Sketchbook-folder).
- Within the Sketchbook folder, create a new subfolder and name it `NGWOS_AWS_LORA`. Open up the new folder after creating it.
- Download the  **two** files from the [NGWOS_AWS_LORA/NGWOS_AWS_LORA](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_AWS_LORA/NGWOS_AWS_LORA) folder on this repo. Note where you save the files.
  - You can download both files together as a zip [by following this link](https://downgit.github.io/#/home?url=https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_AWS_LORA/NGWOS_AWS_LORA).
- Move the two files downloaded above to the `NGWOS_AWS_LORA` folder you created.
- If you are using the default Arduino sketch folder it should look like this:

```txt
C:\Users\{your_user_name}\Documents\Arduino
    └ NGWOS_AWS_LORA
        └ LoRaModemFxns.h
        └ NGWOS_AWS_LORA.ino
```

- Once *both* files are in the folder, open the sketch in the Arduino IDE by using the file menu (`file > open > C:\Users\{username}\Documents\Arduino\NGWOS_AWS_LORA\NGWOS_AWS_LORA.ino`).
  - You only need to open the ino file; the h file will open automatically with the ino.

#### Installing Library Dependencies

This example program is built around the develop branch of ModularSensors library, NOT the released version of the library!  In addition to ModularSensors, you need to have the following libraries and versions installed:

- envirodiy/LoRa_AT@^0.4.2
- envirodiy/StreamDebugger from GitHub
- bblanchon/ArduinoJson@^6.21.5
  - NOTE: This is *NOT* the latest version of the ArduinoJson library! Use version 6! Version 7 will *probably* work, but the CayenneLPP depends on version 6.
- electroniccats/CayenneLPP@^1.4.0

You can download all of these dependencies together in the [zip file](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/AllDependencies.zip) in the repository main folder.
Follow the [instructions from the repository ReadMe](https://github.com/EnviroDIY/USGS_NGWOS/tree/main#installing-or-updating-libraries-for-the-examples-in-the-arduino-ide) to install all of the library dependencies together.

### PlatformIO/VSCode

- Using Windows Explorer (or a folder/file manager), navigate to your PlatformIO project folder (or your desired working folder).
  - The default folder for Windows is `C:\Users\{your_user_name}\Documents\PlatformIO\Projects`.
- Within the project folder, create a new subfolder and name it `NGWOS_AWS_LORA`. Open up the new folder after creating it.
- Download the [example platformio.ini file for this example](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/NGWOS_AWS_LORA/platformio_example.ini) and put it in the `NGWOS_AWS_LORA` folder you created.
  - After downloading, rename the file to `platformio.ini` (remove the `_example` part of the file name).
- Create another deeper subfolder, also named `NGWOS_AWS_LORA` inside of the already existing `NGWOS_AWS_LORA` folder.
- Download the **two** files from the [NGWOS_AWS_LORA/NGWOS_AWS_LORA](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_AWS_LORA/NGWOS_AWS_LORA) folder on this repo and move it into the deeper subfolder.
  - You can download both files together as a zip [by following this link](https://downgit.github.io/#/home?url=https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_AWS_LORA/NGWOS_AWS_LORA).
- Your final folder should look like this (assuming you are using the default PlatformIO project folder):

```txt
C:\Users\{your_user_name}\Documents\PlatformIO\Projects
    └ NGWOS_AWS_LORA
        └ platformio.ini
        └ NGWOS_AWS_LORA
            └ LoRaModemFxns.h
            └ NGWOS_AWS_LORA.ino
```

- Open the project in VSCode by using the file menu (`File > Open Folder > C:\Users\{your_user_name}\Documents\PlatformIO\Projects\NGWOS_AWS_LORA`) or by opening PlatformIO "Home" and opening the project from the `Open Project` quick access option.
- If you've correctly added the `platformio.ini` file to you folder, the project should be detected by the PlatformIO extension, which should download the correct core and libraries automatically.

## Setting up in AWS

- An AWS administrator for your account must create settings for your LoRa gateway within AWS IoT Core and provision your LoRa gateway with the proper CUPS settings and certificates.
- An AWS administrator must also create a new LoRa endpoint device within AWS IoT Core that is linked to the EUI of your LoRa module.
- OPTIONAL: An AWS administrator can create a new Think within AWS IoT Core that is linked to your LoRa module

After your AWS administrator configures everything for you, in addition to the hardware you will need:

- The *AppEUI* associated with your LoRa module
- The *AppKey* associated with your LoRa module
- The Thing name for the Thing associated with your device, if your AWS administrator gave you one

## Customizing the Example Sketch

You need to customize the NGWOS_AWS_LORA.ino file to send data to your application.
*Do not make any changes to the LoRaModemFxns.h file.*

### Set your Thing Name

In line 82, find and replace the text YOUR_LORA_THING_NAME with your assigned thing name.
Make sure there are quotation marks around the name string, as there are in the example.
If your AWS administrator did not assign you a Thing name, you can use any short string for this.

> [!NOTE]
> The thing name is *not* included in any of the transmitted messages.
> It is only added to the file name of the csv file.

### Set your LoRa connection Credentials

- Scroll to lines 126 - 131 in the program.
- Modify the appEui and appKey in the code to match the values provided by your AWS administrator

```cpp
// Your OTAA connection credentials, if applicable
// The App EUI (also called the Join EUI or the Network ID)
// This must be exactly 16 hex characters (8 bytes = 64 bits)
const char appEui[] = "YourAppEUI";
// The App Key (also called the network key)
// This must be exactly 32 hex characters (16 bytes = 128 bits)
const char appKey[] = "YourAppKey";
```

> [!NOTE]
> This program has pins and timings set for a MultiTech mDot and EnviroDIY Stonefly.
> To use it with a different module or logger board, you would need to do more heavy modification of the program.

## Upload to your Stonefly

After correctly modifying the configuration and set certificates files, upload the sketch to your Stonefly.
Instructions for uploading sketches can be found on the [Arduino support pages](https://support.arduino.cc/hc/en-us/articles/4733418441116-Upload-a-sketch-in-Arduino-IDE).
You should have already installed the board package from the Stonefly following the instructions in the [main Read Me for this repository](https://github.com/EnviroDIY/USGS_NGWOS/?tab=readme-ov-file#setting-up-the-stonefly-in-the-arduino-ide).

## Further Reading: ArduinoJSON 6 vs 7

This program will compile and run with either ArduinoJSON 6 or 7, but you will get processor warnings with v7.
There are some syntax changes between the two versions of ArduinoJSON.
The [latest version of this example sketch on GitHub](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/NGWOS_TTN/TheThingsNetwork/TheThingsNetwork.ino) attempts to automatically detect the version of the ArduinoJSON library with defines and use the appropriate syntax for it.
If you are getting compile *errors* that appear to be related to ArduinoJSON find and modify the block of code starting with the comment `// Initialize a buffer for decoding Cayenne LPP messages` (around lines 355-360):

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
