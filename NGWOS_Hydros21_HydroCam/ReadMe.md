# Hydros 21 and HydroCam

This program transmits data from a Meter Hydros 21 and onboard sensors from a Stonefly data logger to Monitor My Watershed.
It also takes images from a Geolux HydroCam and saves those images to an SD card.
The camera images are **NOT** transmitted to any endpoint.

> [!NOTE]
> I've already sent Bill code with UUID's prepared to go to the [TestVega](https://monitormywatershed.org/sites/TestVega/) site on Monitor My Watershed.

- [Hydros 21 and HydroCam](#hydros-21-and-hydrocam)
  - [Physical Connections](#physical-connections)
  - [Library Dependencies](#library-dependencies)
  - [Setting up in Monitor My Watershed](#setting-up-in-monitor-my-watershed)
  - [Customizing the Example Sketch](#customizing-the-example-sketch)
  - [Enabling the Vega Puls in this sketch](#enabling-the-vega-puls-in-this-sketch)
    - [v1: as sent to Bill](#v1-as-sent-to-bill)
    - [v2: as on GitHub](#v2-as-on-github)
  - [Using the Vega Puls and Meter Hydros21 together](#using-the-vega-puls-and-meter-hydros21-together)
    - [Option 1: Using both on the same SDI-12 bus (data pin) with different addresses](#option-1-using-both-on-the-same-sdi-12-bus-data-pin-with-different-addresses)
    - [Option 2: Using each on a separate SDI-12 bus (data pin)](#option-2-using-each-on-a-separate-sdi-12-bus-data-pin)
  - [Switching from LTE to WiFi and back](#switching-from-lte-to-wifi-and-back)
  - [Switching to WiFi](#switching-to-wifi)
  - [Switching to LTE](#switching-to-lte)

## Physical Connections

This program is written for an EnviroDIY Stonefly, a Vega Puls 23, a Geolux HydroCan, a Meter Hydros21, and either an EnviroDIY LTE Bee or an EnviroDIY WiFi Bee.

The physical connections needed are nearly identical to those described in the [The Things Network ReadMe](https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_TTN), but with the a different Bee replacing the mDot and the addition of the camera.

The LTE (or wifi) bee should be installed in the "bee" socket on the Stonefly.
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

This example program is built around the **HydroCam branch** of ModularSensors library, ***NOT*** the released version of the library!

To get all of the correct dependencies for Arduino IDE, please download them together in the [zip file](https://github.com/EnviroDIY/USGS_NGWOS/blob/main/AllDependencies.zip) in the repository main folder.
After unzipping the dependencies, move them all to your Arduino libraries folder.
Instructions for finding your libraries folder are [here](https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer).

If you are using PlatformIO, using the example platformio.ini in this folder should get all of the correct library versions installed.

## Setting up in Monitor My Watershed

This example has dummy UUID's.
To get real UUID's to send data to Monitor My Watershed, first create an account and new site following [these instructions](https://wikiwatershed.org/kbcategories/monitor-my-watershed-envirodiy-sensor-data-manual/).
Once you've created your site, click the "Manage sensors" button to add new sensor variables to the site.
You should create the following sensors:

- Vega - Vega Puls 23 - Stage - meter
- Vega - Vega Puls 23 - Range - meter
- Vega - Vega Puls 23 - Temperature - °C
- Vega - Vega Puls 23 - Measurement Reliability - Decibel
- Vega - Vega Puls 23 - Status Code - Dimensionless
- Meter - Hydros21 - Specific Conductance - µS/cm
- Meter - Hydros21 - Temperature - °C
- Meter - Hydros21 - Depth - millimeters
- Geolux - HydroCam - Image Size - bytes
- Sensirion - SHT40 - Humidity - %
- Sensirion - SHT40 - Temperature - °C
- EVERLIGHT - Ambient Light Sensor - Illuminance - Lux
- EnviroDIY - EnviroDIY LTE Bee - Signal Percent - %
- EnviroDIY - Stonefly - Battery Voltage - Volt
- EnviroDIY - Stonefly - Sample Number - Dimensionless

Once you've created all of the sensors, navigate back to the site and you should be able to see UUID's under the empty graphs for each variable.

## Customizing the Example Sketch

Once you have all of the UUID's for your sensors from Monitor My Watershed, copy them one-by-one into the example code between lines 254 and 284.
Also add the site and registration tokens in lines 293-296.
Make sure you replace all instances of `12345678-abcd-1234-ef00-1234567890ab` with correct UUID's.
The UUID's must be surrounded by quotes.

## Enabling the Vega Puls in this sketch

This program includes code for both the Vega Puls and the Hydros 21, but the Vega Puls is "commented out."

### v1: as sent to Bill

To enable the Vega Puls, change the `#if 0` in line 210 to `#if 1`.
Also remove the leading double slashes (`//`) before the Vega Puls variables in lines 249-256.

### v2: as on GitHub

Remove the leading double slashes (`//`) before `#define USE_VEGA_PULS` in line 10.

## Using the Vega Puls and Meter Hydros21 together

To use the Vega Puls and Meter Hydro21 they must either have different addresses or be connected to different data pin.
If both sensors are connected to the same data pin with the same address, their communication will clash and you will not get data out of either.

### Option 1: Using both on the same SDI-12 bus (data pin) with different addresses

For this, you leave the wiring for each sensor exactly as described in [Physical Connections](#physical-connections) but change the address of each sensor both on the sensor itself and in the example code.

To change the addresses of the sensors, use "[Example B](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/b_address_change)" from the SDI-12 Arduino library.

- Scroll to line 17 of that address change example (`int8_t dataPin = 7;`).
- Change the pin from `7` to `3` to match the wiring described above.
- Leave the power pin as `22`.

The new sensor addresses must be different from each other and ***neither should use address `0`***!
Meter devices return extra information when set to address 0.
To prevent confusion and garbled communication, it is best to avoid this address for Meter devices, especially when used in combination with other devices.
For more information on the Meter output with address 0, see "METER SDI-12 IMPLEMENTATION" on the bottom of page 4 of the [Hydros21 Integrator Guide](https://library.metergroup.com/Integrator%20Guide/18468%20HYDROS%2021%20Gen2%20Integrator%20Guide.pdf).

To change the addresses in the NGWOS_Hydros21_HydroCam example code, change these lines to your newly assigned addresses:

```cpp
const char* VegaPulsSDI12address = "0";  // The SDI-12 Address of the VegaPuls21
```

```cpp
const char*   hydros21SDI12address = "0";  // The SDI-12 Address of the Hydros21
```

### Option 2: Using each on a separate SDI-12 bus (data pin)

In this case, you will not need to change any sensor addresses.
Instead, you move the data wire of the Vega Puls to a different data pin and change the example code accordingly.
The *white* SDI-12 data wire from the Vega Puls is the only wire that needs to be changed.

- Disconnect the *white* Vega Puls wire from the "S2" connection of the screw terminal.
- Reconnect the *white* Vega Puls wire to the "S1" connection of the screw terminal.
- Keep the Grove screw terminal plugged into the same socket labeled "D2-3 & SDI12" on the Stonefly.

After rewiring, change the `3` to `2` in this single line in the NGWOS_Hydros21_HydroCam example code:

```cpp
const int8_t VegaPulsData        = 3;               // The SDI-12 data pin
                                // ^^ change to 2!
```

## Switching from LTE to WiFi and back

This program has code in it for both LTE and WiFi.
The default code as posted is for LTE.

## Switching to WiFi

- Ensure that there are ***not*** double slashes (`//`) before `#define BUILD_MODEM_ESPRESSIF_ESP32` in line 8
- Ensure that there ***are*** double slashes (`//`) before `#define BUILD_MODEM_SIM_COM_SIM7080` in line 9.
- Modify these lines (~96-99) with your correct wifi credentials:

```cpp
// WiFi access point name
#define WIFI_ID "YourWifiID"
// WiFi password (WPA2)
#define WIFI_PASSWD "YourWifiPassword"
```

NOTE:

- Only WPA-2 WiFi security is supported.
- Wifi networks that require a sign-in type page before access are *not* supported.
- Wifi networks that require special certificates are *not* supported.

## Switching to LTE

- Ensure that there ***are*** double slashes (`//`) before `#define BUILD_MODEM_ESPRESSIF_ESP32` in line 8
- Ensure that there are ***not*** double slashes (`//`) before `#define BUILD_MODEM_SIM_COM_SIM7080` in line 9.
- Modify this line (~94) with your proper cellular APN (Access Point Name):

```cpp
// Network connection information
// APN for cellular connection
#define CELLULAR_APN "hologram"
```

You must get the APN from your SIM card provider - it is usually publicly available on the providers website.
The APN for the Hologram SIMs preferred by the Stroud Water Research Center is simply "hologram."
This example as written does not support SIM cards that must be unlocked with a pin code or APN's that require a username or password.
