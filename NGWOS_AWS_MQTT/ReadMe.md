# NGWOS AWS

This program transmits data from a Vega Puls 21, a Geolux camera, and onboard sensors from a Stonefly data logger to AWS IoT Core (for numeric data) and S3 (for images).

While this program is set for the above sensors, modem, and endpoints, the [ModularSensors library](https://github.com/EnviroDIY/ModularSensors/) this program is built on supports many more sensors, modems, and data endpoints.
Look at the [GitHub repository](https://github.com/EnviroDIY/ModularSensors/) and [documentation](https://envirodiy.github.io/ModularSensors/) for ModularSensors for help in adding more sensors.
The ["menu a la carte" example](https://github.com/EnviroDIY/ModularSensors/tree/master/examples/menu_a_la_carte) is a collection of code snippets for all of the supported units for ModularSensors.
As of 7/1/2025, this program depends on the [develop](https://github.com/EnviroDIY/ModularSensors/tree/develop) branch of the library, but that branch should be merged into the main branch soon.

> [!WARNING]
> You should have loaded and **successfully** run the NGWOS_AWS_MQTT_Certs.ino sketch *before* running this program.
> This program depends on correct certificates being pre-loaded onto the modem.

- [NGWOS AWS](#ngwos-aws)
  - [Physical Connections](#physical-connections)
  - [Library Dependencies](#library-dependencies)
  - [Confirm that you can connect to AWS](#confirm-that-you-can-connect-to-aws)
  - [Customizing the Example Sketch](#customizing-the-example-sketch)
    - [Set your AWS IoT Core Endpoint](#set-your-aws-iot-core-endpoint)
    - [Set your Thing Name](#set-your-thing-name)
    - [Set your Sampling Feature (Site) ID](#set-your-sampling-feature-site-id)
    - [Set your cellular APN](#set-your-cellular-apn)

## Physical Connections

This program is written for an EnviroDIY Stonefly, a Vega Puls 23, a Geolux HydroCan, a Meter Hydros21, and an EnviroDIY LTE Bee.

The physical connections needed are nearly identical to those described in the [The Things Network ReadMe](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_TTN), but with the a different Bee replacing the mDot and the addition of the camera.

The LTE bee should be installed in the "bee" socket on the Stonefly.
The cut corners should be at the top of the module, following the traced lines on the Stonefly

The Vega Puls should be connected to a screw-terminal-to-Grove adapter.
The Grove plug from the Vega Puls should be connected to one of the two sockets labeled "D2-3 & SDI12" on the Stonefly.

- The *brown* wire of the Vega Puls should be connected to the "V+" connection of the screw terminal.
- Both the *blue* and thick black/grey *shield* wires should be connected to "Gnd" on the screw terminal.
- The *white* SDI-12 data wire should be connected to the "S2" connection of the screw terminal.
- There should be nothing connected to the "S1" connection of the screw terminal.

The Geolux HydroCam connects to the Stonefly via an RS232 adapter board and *two* grove connections.
One Grove plug should be plugged into one of the two sockets labeled "D2-3 & SDI12" on the Stonefly - right next to where the Vega Puls is plugged in.
This connection is only supplying 12V power to the camera.
The other Grove plug should be connected to the "UART1" Grove plug on the bottom left of the Stonefly.
The jumper next to the "UART1" Grove plug should be connecting the middle two pins to supply the RS232 adapter with 3.3V.

TODO: Add instructions for connecting the secondary battery monitor!

## Library Dependencies

This example program is built around the **AWS IoT branch** of ModularSensors library, ***NOT*** the released version of the library!

To get all of the correct dependencies for Arduino IDE, please download them together in the [zip file](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/AllDependencies.zip) in the repository main folder.
After unzipping the dependencies, move them all to your Arduino libraries folder.
Instructions for finding your libraries folder are [here](https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer).

If you are using PlatformIO, using the example platformio.ini in this folder should get all of the correct library versions installed.

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

### Set your Sampling Feature (Site) ID

In line 70, find and replace the text `YOUR_SAMPLING_FEATURE_ID` with your assigned sampling feature ID or UUID.
Make sure there are quotation marks around the name string, as there are in the example.
There was no sampling feature ID used in the NGWOS_AWS_MQTT_Certs sketch, so you don't need to worry about it matching.

### Set your cellular APN

If you are using a SIM card *other than the [Hologram](https://www.hologram.io/) SIM card* provided by The Stroud Water Research Center, you must provide the [APN](https://en.wikipedia.org/wiki/Access_Point_Name) for your network.
In line 113, find and replace the text `hologram` with the APN for your SIM card.
Make sure there are quotation marks around the APN string, as there are in the example.
If you are using a Hologram SIM, you don't need to change this.
