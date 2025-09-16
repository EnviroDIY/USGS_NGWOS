# Stonefly Power On and Blink Test

This is a very simple program to turn on sensor power and blink the onboard LEDs on the Stonefly.
This program can also be used to power the VEGAPULS C 21 so that it can be configured using the Vega Tools app.

- [Stonefly Power On and Blink Test](#stonefly-power-on-and-blink-test)
  - [Physical Connections](#physical-connections)
  - [Downloading the Program and Setting up the IDE](#downloading-the-program-and-setting-up-the-ide)
    - [Arduino IDE](#arduino-ide)
    - [PlatformIO/VSCode](#platformiovscode)
  - [Upload to your Stonefly](#upload-to-your-stonefly)
  - [Configuring the Vega Puls using the Vega Tools App](#configuring-the-vega-puls-using-the-vega-tools-app)

## Physical Connections

This program is written for an EnviroDIY Stonefly. If you attach a VEGAPULS C 21, you can use the program to power the Vega Puls so that you can configure it with the Vega Tools app.
Instructions for assembling the connecting the Vega Puls are available in [this blog post](https://www.envirodiy.org/river-flood-monitoring-system-for-rapid-deployment/).
Scroll down to the "Assembly" steps.

There are no libraries needed to run this example.

## Downloading the Program and Setting up the IDE

### Arduino IDE

- Using Windows Explorer (or a folder/file manager), navigate to your Sketchbook folder (or your desired working folder).
  - The default folder for Windows is `C:\Users\{username}\Documents\Arduino`; instructions to find your folder if needed are [here](https://support.arduino.cc/hc/en-us/articles/4412950938514-Open-the-Sketchbook-folder).
- Within the Sketchbook folder, create a new subfolder and name it `PowerAndBlink`. Open up the new folder after creating it.
- Download the ino file from the [PowerAndBlink/PowerAndBlink](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/PowerAndBlink/PowerAndBlink) folder on this repo. Note where you save the file.
  - There is only a single ino file needed for this example.
- Move the file downloaded above to the `PowerAndBlink` folder you created.  Your final folder should look like this (assuming you are using the default Windows Sketchbook folder):

```txt
C:\Users\{your_user_name}\Documents\Arduino
    └ PowerAndBlink
        └ PowerAndBlink.ino
```

- Once the file is in the folder, open the sketch in the Arduino IDE by using the file menu (`file > open > C:\Users\{username}\Documents\Arduino\PowerAndBlink\PowerAndBlink.ino`).

### PlatformIO/VSCode

- Using Windows Explorer (or a folder/file manager), navigate to your PlatformIO project folder (or your desired working folder).
  - The default folder for Windows is `C:\Users\{your_user_name}\Documents\PlatformIO\Projects`.
- Within the project folder, create a new subfolder and name it `PowerAndBlink`. Open up the new folder after creating it.
- Download the [example platformio.ini file for this example](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/PowerAndBlink/platformio_example.ini) and put it in the `PowerAndBlink` folder you created.
  - After downloading, rename the file to `platformio.ini` (remove the `_example` part of the file name).
- Create another deeper subfolder, also named `PowerAndBlink` inside of the already existing `PowerAndBlink` folder.
- Download the ino file from the [PowerAndBlink/PowerAndBlink](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/PowerAndBlink/PowerAndBlink) folder on this repo and move it into the deeper subfolder.
- Your final folder should look like this (assuming you are using the default PlatformIO projects folder):

```txt
C:\Users\{your_user_name}\Documents\PlatformIO\Projects
    └ PowerAndBlink
        └ platformio.ini
        └ PowerAndBlink
            └ PowerAndBlink.ino
```

- Open the project in VSCode by using the file menu (`File > Open Folder > C:\Users\{your_user_name}\Documents\PlatformIO\Projects\PowerAndBlink`) or by opening PlatformIO "Home" and opening the project from the `Open Project` quick access option.

## Upload to your Stonefly

After correctly modifying the configuration and set certificates files, upload the sketch to your Stonefly.
Instructions for uploading sketches can be found on the [Arduino support pages](https://support.arduino.cc/hc/en-us/articles/4733418441116-Upload-a-sketch-in-Arduino-IDE).
You should have already installed the board package from the Stonefly following the instructions in the [main Read Me for this repository](https://github.com/EnviroDIY/USGS_NGWOS/?tab=readme-ov-file#setting-up-the-stonefly-in-the-arduino-ide).

To upload with PlatformIO, use the upload task shortcut from the bottom menu or select the "Upload" project task from the PlatformIO menu.

## Configuring the Vega Puls using the Vega Tools App

The VEGAPULS C 21 can be configured using the [Vega Tools App](https://www.vega.com/en-us/products/product-catalog/signal-conditioning/software/vega-tools-app).
The free app can be downloaded for both iOS and Android devices.
When you first open the app, you will need to allow location and/or Bluetooth access for the app.

After uploading and starting this sketch, open the Vega Tools app.
You will start with an "Overview" screen.
Select "Setup & Diagnosis" from the list of options.
The app will search for Bluetooth devices and should find your VEGAPULS C 21.
When you select your device, you will get a screen asking you to enter the access code for the Bluetooth connection.
The access code is printed on the sensor itself.
It is a 6-digit number near the top of the sensor.
There's a phone icon next to the number.
Once the app finds your sensor, you can click on the sensor to enter its setup.

Using the app, you can fully configure the device and view its diagnostics.
***There is nothing you need to configure using the app for any of the examples in this repository!***
Some settings you may be most interested in changing later include the stage-to-depth offset, the SDI-12 address, the sensor name, and the output units.

> [!note]
> The Vega Puls App frequently reports the sensor in a state of "failure" when connected to the app although the sensor doesn't seem to have any problems when used over SDI-12.
> I have not figure out why this is.
