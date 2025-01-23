# The EnviroDIY Stonefly

This new logger board was developed by the Stroud Water Research center with funding from the USGS's [NGWOS](https://www.usgs.gov/mission-areas/water-resources/science/next-generation-water-observing-system-ngwos) [External R&D](https://www.usgs.gov/mission-areas/water-resources/science/ngwos-external-research-and-development) Program
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
You **must have administrator privileges** on a Windows computer to install the Arduino IDE and the Adafruit SAMD core needed by the Stonefly.

To add the EnviroDIY Stonefly board to your Arduino IDE, go to File > Preferences (for a Mac: Arduino > Preferences) and copy the following URL into the box labeled "Additional Boards Manager URLs":

https://raw.githubusercontent.com/EnviroDIY/Arduino_boards/master/package_EnviroDIY_index.json

If you already have other custom board manager URL's, extend your list with the EnviroDIY

Then in the IDE, click on Tools > Board > Boards Manager.   Use the dropdown box for Type to select "Contributed".
You should then see an option for "EnviroDIY SAMD Boards".
Click the "Install" button to add the EnviroDIY boards to your IDE.
The install process should also install the Adafruit core into the Arduino IDE.
As mentioned above, you **must have administrator privileges** on a Windows computer for the install to succeed.
If you have problems installing the board and core in the board manager, Adafruit has more detailed instructions [here](https://learn.adafruit.com/add-boards-arduino-v164/setup).
When following the Adafruit instructions, make sure to use the EnviroDIY URL above, not just Adafruit's URL.

Once the board and core packages are installed, when you click Tools > Board you will see EnviroDIY SAMD boards > EnviroDIY Stonefly listed as options for available boards.
