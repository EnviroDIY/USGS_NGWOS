# NGWOS AWS IoT Core and S3 Imagery

This program transmits data from a Vega Puls 21, a Geolux camera, and onboard sensors from a Stonefly data logger to AWS IoT Core (for numeric data) and S3 (for images).
This program uses an EnviroDIY LTE Bee based on a SIMCom SIM7080G LTE module.

While this program is set for the above sensors, modem, and endpoints, the [ModularSensors library](https://github.com/EnviroDIY/ModularSensors/) this program is built on supports many more sensors, modems, and data endpoints.
Look at the [GitHub repository](https://github.com/EnviroDIY/ModularSensors/) and [documentation](https://envirodiy.github.io/ModularSensors/) for ModularSensors for help in adding more sensors.
The ["menu a la carte" example](https://github.com/EnviroDIY/ModularSensors/tree/master/examples/menu_a_la_carte) is a collection of code snippets for all of the supported units for ModularSensors.
As of 9/16/2025, this program depends on the [develop](https://github.com/EnviroDIY/ModularSensors/tree/develop) branch of the library, but that branch should be merged into the main branch soon.

> [!WARNING]
> You should have loaded and **successfully** run the NGWOS_AWS_MQTT_Certs.ino sketch *before* running this program.
> This program depends on correct certificates being pre-loaded onto the modem.

- [NGWOS AWS IoT Core and S3 Imagery](#ngwos-aws-iot-core-and-s3-imagery)
  - [Physical Connections](#physical-connections)
  - [Downloading the Program and Setting up the IDE](#downloading-the-program-and-setting-up-the-ide)
    - [Arduino IDE](#arduino-ide)
      - [Installing Library Dependencies](#installing-library-dependencies)
    - [PlatformIO/VSCode](#platformiovscode)
  - [Confirm that you can connect to AWS](#confirm-that-you-can-connect-to-aws)
  - [Configuring the ModularSensors library](#configuring-the-modularsensors-library)
  - [Customizing the Example Sketch](#customizing-the-example-sketch)
    - [Set your AWS IoT Core Endpoint](#set-your-aws-iot-core-endpoint)
    - [Set your Thing Name](#set-your-thing-name)
    - [Set your cellular APN](#set-your-cellular-apn)
  - [Upload to your Stonefly](#upload-to-your-stonefly)

## Physical Connections

This program is written for an EnviroDIY Stonefly, a Vega Puls 21, a Geolux HydroCan, an EnviroDIY LTE Bee, and other solar and battery components.
Instructions for assembling the components are available in [this blog post](https://www.envirodiy.org/river-flood-monitoring-system-for-rapid-deployment/).
Scroll down to the "Assembly" steps.

## Downloading the Program and Setting up the IDE

### Arduino IDE

- Using Windows Explorer (or a folder/file manager), navigate to your Sketchbook folder (or your desired working folder).
  - The default folder for Windows is `C:\Users\{username}\Documents\Arduino`; instructions to find your folder if needed are [here](https://support.arduino.cc/hc/en-us/articles/4412950938514-Open-the-Sketchbook-folder).
- Within the Sketchbook folder, create a new subfolder and name it `NGWOS_AWS_MQTT`. Open up the new folder after creating it.
- Download the ino file from the [NGWOS_AWS_MQTT/NGWOS_AWS_MQTT](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_AWS_MQTT/NGWOS_AWS_MQTT) folder on this repo. Note where you save the file.
  - There is only a single ino file needed for this example.
- Move the file downloaded above to the `NGWOS_AWS_MQTT` folder you created.  Your final folder should look like this (assuming you are using the default Windows Sketchbook folder):

```txt
C:\Users\{your_user_name}\Documents\Arduino
    └ NGWOS_AWS_MQTT
        └ NGWOS_AWS_MQTT.ino
```

- Once the file is in the folder, open the sketch in the Arduino IDE by using the file menu (`file > open > C:\Users\{username}\Documents\Arduino\NGWOS_AWS_MQTT\NGWOS_AWS_MQTT.ino`).

#### Installing Library Dependencies

This example program is built around the **develop branch** of ModularSensors library, ***NOT*** the released version of the library!
That libraries and all of its many dependencies are included in the [zip file](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/AllDependencies.zip) in the repository main folder.
Follow the [instructions from the repository ReadMe](https://github.com/EnviroDIY/USGS_NGWOS/tree/main#installing-or-updating-libraries-for-the-examples-in-the-arduino-ide) to install all of the library dependencies together.

### PlatformIO/VSCode

- Using Windows Explorer (or a folder/file manager), navigate to your PlatformIO project folder (or your desired working folder).
  - The default folder for Windows is `C:\Users\{your_user_name}\Documents\PlatformIO\Projects`.
- Within the project folder, create a new subfolder and name it `NGWOS_AWS_MQTT`. Open up the new folder after creating it.
- Download the [example platformio.ini file for this example](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/NGWOS_AWS_MQTT/platformio_example.ini) and put it in the `NGWOS_AWS_MQTT` folder you created.
  - After downloading, rename the file to `platformio.ini` (remove the `_example` part of the file name).
- Create another deeper subfolder, also named `NGWOS_AWS_MQTT` inside of the already existing `NGWOS_AWS_MQTT` folder.
- Download the ino file from the [NGWOS_AWS_MQTT/NGWOS_AWS_MQTT](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_AWS_MQTT/NGWOS_AWS_MQTT) folder on this repo and move it into the deeper subfolder.
- Your final folder should look like this (assuming you are using the default Windows Sketchbook folder):
- Your final folder should look like this (assuming you are using the default PlatformIO projects folder):

```txt
C:\Users\{your_user_name}\Documents\PlatformIO\Projects
    └ NGWOS_AWS_MQTT
        └ platformio.ini
        └ NGWOS_AWS_MQTT
            └ NGWOS_AWS_MQTT.ino
```

- Open the project in VSCode by using the file menu (`File > Open Folder > C:\Users\{your_user_name}\Documents\PlatformIO\Projects\NGWOS_AWS_MQTT`) or by opening PlatformIO "Home" and opening the project from the `Open Project` quick access option.
- If you've correctly added the `platformio.ini` file to you folder, the project should be detected by the PlatformIO extension, which should download the correct core and libraries automatically.

## Confirm that you can connect to AWS

Before running this program, you must upload certificates to your modem.
You should have done this with the NGWOS_AWS_MQTT_Certs sketch.
Do not proceed to this program until you have successfully connected to and published a message to AWS IoT Core using the NGWOS_AWS_MQTT_Certs sketch.

You need your thing name and endpoint again for this sketch.

## Customizing the Example Sketch

### Set your AWS IoT Core Endpoint

In line 57, find and replace the text `YOUR_ENDPOINT-ats.iot.us-west-2.amazonaws.com` with your real endpoint.
Make sure there are quotation marks around the endpoint string, as there are in the example.
This must be the same value you used in the NGWOS_AWS_MQTT_Certs sketch.

### Set your Thing Name

In line 58, find and replace the text `YOUR_THING_NAME` with your assigned thing name.
Make sure there are quotation marks around the name string, as there are in the example.
This must be the same value you used in the NGWOS_AWS_MQTT_Certs sketch.

### Set your cellular APN

If you are using a SIM card *other than the [Hologram](https://www.hologram.io/) SIM card* provided by The Stroud Water Research Center, you must provide the [APN](https://en.wikipedia.org/wiki/Access_Point_Name) for your network.
In line 109, find and replace the text `hologram` with the APN for your SIM card.
Make sure there are quotation marks around the APN string, as there are in the example.
If you are using a Hologram SIM, you don't need to change this.

## Upload to your Stonefly

After correctly modifying the configuration and set certificates files, upload the sketch to your Stonefly.
Instructions for uploading sketches can be found on the [Arduino support pages](https://support.arduino.cc/hc/en-us/articles/4733418441116-Upload-a-sketch-in-Arduino-IDE).
You should have already installed the board package from the Stonefly following the instructions in the [main Read Me for this repository](https://github.com/EnviroDIY/USGS_NGWOS/?tab=readme-ov-file#setting-up-the-stonefly-in-the-arduino-ide).

To upload with PlatformIO, use the upload task shortcut from the bottom menu or select the "Upload" project task from the PlatformIO menu.
