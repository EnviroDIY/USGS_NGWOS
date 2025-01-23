# The EnviroDIY Stonefly

This new logger board was developed by the Stroud Water Research center with funding from the USGS's [NGWOS](https://www.usgs.gov/mission-areas/water-resources/science/next-generation-water-observing-system-ngwos) [External R&D](https://www.usgs.gov/mission-areas/water-resources/science/ngwos-external-research-and-development) Program.
The brain of the new board is a Microchip Technology's [ATSAMD51N19A](https://www.microchip.com/en-us/product/ATSAMD51N19A).

## UF2 bootloader

The bootloader we are using on the board is Adafruit's fork of Microsoft's UF2 bootloader.
Adafruit has more information on the bootloader [here](https://learn.adafruit.com/adafruit-feather-m0-express-designed-for-circuit-python-circuitpython/uf2-bootloader-details).
The bootloader was built with help of a python toolchain, forked [here](https://github.com/SRGDamia1/SAMD-custom-board).
The final bootloader files are available on GitHub [here](https://github.com/EnviroDIY/Arduino_boards/tree/master/EnviroDIYSAMDBoards).
There are detailed instructions for reinstalling the bootloader [here](https://github.com/SRGDamia1/SAMD-custom-board/blob/main/Install%20Bootloader.md), but there generally shouldn't be any reason to reinstall it.
You must have a special programmer or debugger board to install the bootloader.

## Setting up the Stonefly in the Arduino IDE

To use the Stonefly in the Arduino IDE, you must be using at least version 2 of the IDE.
The IDE can be downloaded [here](https://www.arduino.cc/en/software).
These instructions were testing using version 2.3.4 of the IDE.
You **must have administrator privileges** on a Windows computer to install the Arduino IDE and the Adafruit SAMD core needed by the Stonefly.

To add the EnviroDIY Stonefly board to your Arduino IDE, go to File > Preferences (for a Mac: Arduino > Preferences) and copy the following URL into the box labeled "Additional Boards Manager URLs":

https://raw.githubusercontent.com/EnviroDIY/Arduino_boards/master/package_EnviroDIY_index.json

If you already have other custom board manager URL's, extend your list with the EnviroDIY link.

Then in the IDE, click on Tools > Board > Boards Manager.
Use the dropdown box for Type to select "Contributed".
You should then see an option for "EnviroDIY SAMD Boards".
Click the "Install" button to add the EnviroDIY boards to your IDE.
The install process should also install the Adafruit core into the Arduino IDE.
As mentioned above, you **must have administrator privileges** on a Windows computer for the install to succeed.
If you have problems installing the board and core in the board manager, Adafruit has more detailed instructions [here](https://learn.adafruit.com/add-boards-arduino-v164/setup).
When following the Adafruit instructions, make sure to use the EnviroDIY URL above, not just Adafruit's URL.

Once the board and core packages are installed, when you click Tools > Board you will see EnviroDIY SAMD boards > EnviroDIY Stonefly listed as options for available boards.

If you get a board error when trying to compile for the Stonefly for the first time, the IDE probably did not correctly install the Adafruit SAMD core.
To add the Adafruit SAMD core, follow the directions linked above to add Adafruit's board URL to your boards list.
Then select and install the "Adafruit SAMD Boards" package.

## Setting up the Stonefly in PlatformIO on VSCode

To use the Stonefly in PlatformIO, you must manually download the boards and variants files and put them in the correct folder.

- Navigate to your [PlatformIO core directory](https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/core_dir.html#projectconf-pio-core-dir). On Windows, it will be something like `C:\Users\{yourUserName}\.platformio`.
- If they don't already exist, create a "boards" folder and a "variants" folder within the PlatformIO core directory.
- Download the Stonefly boards file [here](https://raw.githubusercontent.com/EnviroDIY/Arduino_boards/refs/heads/master/EnviroDIYSAMDBoards/boards/envirodiy_stonefly_m4.json) and put it into your new boards subdirectory.
  - On windows, the new file should end up being `C:\Users\{yourUserName}\.platformio\boards\envirodiy_stonefly_m4.json`
- Download all of the files in this [variants directory](https://github.com/EnviroDIY/Arduino_boards/tree/master/EnviroDIYSAMDBoards/variants/stonefly_m4) and put them in your new variants folder.
  - You should end up with these files:
    - `C:\Users\{yourUserName}\.platformio\variants\stonefly_m4\pins_arduino.h`
    - `C:\Users\{yourUserName}\.platformio\variants\stonefly_m4\variant.h`
    - `C:\Users\{yourUserName}\.platformio\variants\stonefly_m4\variant.cpp`
    - `C:\Users\{yourUserName}\.platformio\variants\stonefly_m4\linker_scripts\gcc\flash_with_bootloader.ld`
    - `C:\Users\{yourUserName}\.platformio\variants\stonefly_m4\linker_scripts\gcc\flash_without_bootloader.ld`
- Create a new PlatformIO project through PlatformIO home. If you have correctly added `envirodiy_stonefly_m4.json` to your boards folder, you should see the option for the Stonefly in the board selection dropdown.
- You must modify your platformio.ini file to specify the location of your variants folder.  Your Stonefly environment should look like the code below.
  - NOTE: You do NOT need to replace `${platformio.core_dir}` with your real core directory. Leave that exactly as is in your ini.

```ini
[env:envirodiy_stonefly_m4]
platform = atmelsam
board = envirodiy_stonefly_m4
framework = arduino
board_build.variant = stonefly_m4
board_build.variants_dir = ${platformio.core_dir}/variants
board_build.ldscript = ${platformio.core_dir}/variants/stonefly_m4/linker_scripts/gcc/flash_with_bootloader.ld
```

## Example Programs

This repo contains 3 example programs for the USGS:

- NGWOS_TTN
  - This program transmits data from a Vega Puls 21 and onboard sensors from a Stonefly data logger to The Things Network
- NGWOS_VegaAndHydroCam
  - This program transmits data from a Vega Puls 21 and onboard sensors from a Stonefly data logger to Monitor My Watershed. It also saves images from a Geolux HydroCam.
- NGWOS_Hydros21_HydroCam
  - This is identical to the Vega and HydroCam example, but with a Meter Hydros21 instead of a Vega Puls.

Within each example folder, there is a zip file of library dependencies.
Download the zip and un-zip it.
Move all of the files from the unzipped directory into your Arduino libraries folder.
Instructions for finding your libraries folder are [here](https://support.arduino.cc/hc/en-us/articles/4415103213714-Find-sketches-libraries-board-cores-and-other-files-on-your-computer).

Detailed instructions for each example are in the ReadMe files in the respective subfolders.
