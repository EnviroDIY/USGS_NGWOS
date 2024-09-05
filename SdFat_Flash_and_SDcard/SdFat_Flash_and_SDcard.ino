// Adafruit Grand Central M4 QSPI Flash and SD Card Setup Example
// Author: Joshua Scoggins
//
// This is an example of how to bring up both the QSPI Flash and SD Card found
// on the Adafruit Grand Central M4. This example will setup both the QSPI
// Flash and SD card (if present) and display information about the QSPI flash.
//

#include <SPI.h>

#include <SdFat.h>
#include <sdios.h>

#include "wiring_private.h"  // pinPeripheral() function

#include <Adafruit_SPIFlash.h>
/*
  Set DISABLE_CS_PIN to disable a second SPI device.
  For example, with the Ethernet shield, set DISABLE_CS_PIN
  to 10 to disable the Ethernet controller.
*/
const int8_t DISABLE_CS_PIN = -1;

// SD_FAT_TYPE =
//  - 0 for SdFat/File as defined in SdFatConfig.h,
//  - 1 for FAT16/FAT32,
//  - 2 for exFAT,
//  - 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3

SdSpiConfig customSdConfig(static_cast<SdCsPin_t>(SDCARD_SS_PIN),
                           (uint8_t)(DEDICATED_SPI | USER_SPI_BEGIN),
                           SPI_FULL_SPEED, &SDCARD_SPI);

void sdCsInit(SdCsPin_t pin) {
    pinMode(pin, OUTPUT);
    pinMode(30, OUTPUT);
}
void sdCsWrite(SdCsPin_t pin, bool level) {
    digitalWrite(pin, level);
    digitalWrite(30, level);
}
SdFat sd;
//------------------------------------------------------------------------------

// for flashTransport definition
#include "flash_config.h"
Adafruit_SPIFlash onboardFlash(&flashTransport);

constexpr int getSDCardPin() noexcept {
#ifdef SDCARD_SS_PIN
    return SDCARD_SS_PIN;
#else
    // modify to fit your needs
    // by default, pin 4 is the SD_CS pin used by the Adafruit 1.8" TFT SD
    // Shield
    return 4;
#endif
}

cid_t    cid;
csd_t    csd;
scr_t    scr;
uint8_t  cmd6Data[64];
uint32_t eraseSize;
uint32_t ocr;

// Serial streams
static ArduinoOutStream cout(Serial);
// SD card chip select
bool         firstTry     = true;
const int8_t sdCardPwrPin = 32;  // MCU SD card power pin

void cardOrSpeed() {
    Serial.print(F("Try another SD card or reduce the SPI bus speed.\n"));
    Serial.print(F("Edit SPI_SPEED in this program to change it.\n"));
}
void cidDmp() {
    cout << F("\nManufacturer ID: ");
    cout << uppercase << showbase << hex << int(cid.mid) << dec << endl;
    cout << F("OEM ID: ") << cid.oid[0] << cid.oid[1] << endl;
    cout << F("Product: ");
    for (uint8_t i = 0; i < 5; i++) { cout << cid.pnm[i]; }
    cout << F("\nRevision: ") << cid.prvN() << '.' << cid.prvM() << endl;
    cout << F("Serial number: ") << hex << cid.psn() << dec << endl;
    cout << F("Manufacturing date: ");
    cout << cid.mdtMonth() << '/' << cid.mdtYear() << endl;
    cout << F("CID HEX: ");
    hexDmp(&cid, sizeof(cid));
}
//------------------------------------------------------------------------------
void clearSerialInput() {
    uint32_t m = micros();
    do {
        if (Serial.read() >= 0) { m = micros(); }
    } while (micros() - m < 10000);
}
//------------------------------------------------------------------------------
void csdDmp() {
    eraseSize = csd.eraseSize();
    cout << F("\ncardSize: ") << 0.000512 * csd.capacity();
    cout << F(" MB (MB = 1,000,000 bytes)\n");

    cout << F("flashEraseSize: ") << int(eraseSize) << F(" blocks\n");
    cout << F("eraseSingleBlock: ");
    if (csd.eraseSingleBlock()) {
        cout << F("true\n");
    } else {
        cout << F("false\n");
    }
    cout << F("dataAfterErase: ");
    if (scr.dataAfterErase()) {
        cout << F("ones\n");
    } else {
        cout << F("zeros\n");
    }
    cout << F("CSD HEX: ");
    hexDmp(&csd, sizeof(csd));
}
//------------------------------------------------------------------------------
void errorPrint() {
    if (sd.sdErrorCode()) {
        cout << F("SD errorCode: ") << hex << showbase;
        printSdErrorSymbol(&Serial, sd.sdErrorCode());
        cout << F(" = ") << int(sd.sdErrorCode()) << endl;
        cout << F("SD errorData = ") << int(sd.sdErrorData()) << dec << endl;
    }
}
//------------------------------------------------------------------------------
void hexDmp(void* reg, uint8_t size) {
    uint8_t* u8 = reinterpret_cast<uint8_t*>(reg);
    cout << hex << noshowbase;
    for (size_t i = 0; i < size; i++) {
        cout << setw(2) << setfill('0') << int(u8[i]);
    }
    cout << dec << endl;
}
//------------------------------------------------------------------------------
bool mbrDmp() {
    MbrSector_t mbr;
    bool        valid = true;
    if (!sd.card()->readSector(0, (uint8_t*)&mbr)) {
        cout << F("\nread MBR failed.\n");
        errorPrint();
        return false;
    }
    cout << F("\nSD Partition Table\n");
    cout << F("part,boot,bgnCHS[3],type,endCHS[3],start,length\n");
    for (uint8_t ip = 1; ip < 5; ip++) {
        MbrPart_t* pt = &mbr.part[ip - 1];
        if ((pt->boot != 0 && pt->boot != 0X80) ||
            getLe32(pt->relativeSectors) > csd.capacity()) {
            valid = false;
        }
        cout << int(ip) << ',' << uppercase << showbase << hex;
        cout << int(pt->boot) << ',';
        for (int i = 0; i < 3; i++) { cout << int(pt->beginCHS[i]) << ','; }
        cout << int(pt->type) << ',';
        for (int i = 0; i < 3; i++) { cout << int(pt->endCHS[i]) << ','; }
        cout << dec << getLe32(pt->relativeSectors) << ',';
        cout << getLe32(pt->totalSectors) << endl;
    }
    if (!valid) {
        cout << F("\nMBR not valid, assuming Super Floppy format.\n");
    }
    return true;
}
//------------------------------------------------------------------------------
void dmpVol() {
    cout << F("\nScanning FAT, please wait.\n");
    int32_t freeClusterCount = sd.freeClusterCount();
    if (sd.fatType() <= 32) {
        cout << F("\nVolume is FAT") << int(sd.fatType()) << endl;
    } else {
        cout << F("\nVolume is exFAT\n");
    }
    cout << F("sectorsPerCluster: ") << sd.sectorsPerCluster() << endl;
    cout << F("fatStartSector:    ") << sd.fatStartSector() << endl;
    cout << F("dataStartSector:   ") << sd.dataStartSector() << endl;
    cout << F("clusterCount:      ") << sd.clusterCount() << endl;
    cout << F("freeClusterCount:  ");
    if (freeClusterCount >= 0) {
        cout << freeClusterCount << endl;
    } else {
        cout << F("failed\n");
        errorPrint();
    }
}
//------------------------------------------------------------------------------
void printCardType() {
    cout << F("\nCard type: ");

    switch (sd.card()->type()) {
        case SD_CARD_TYPE_SD1: cout << F("SD1\n"); break;

        case SD_CARD_TYPE_SD2: cout << F("SD2\n"); break;

        case SD_CARD_TYPE_SDHC:
            if (csd.capacity() < 70000000) {
                cout << F("SDHC\n");
            } else {
                cout << F("SDXC\n");
            }
            break;

        default: cout << F("Unknown\n");
    }
}
//------------------------------------------------------------------------------
void printConfig(SdSpiConfig config) {
    if (DISABLE_CS_PIN < 0) {
        cout << F("\nAssuming the SD is the only SPI device.\n"
                  "Edit DISABLE_CS_PIN to disable an SPI device.\n");
    } else {
        cout << F("\nDisabling SPI device on pin ");
        cout << int(DISABLE_CS_PIN) << endl;
        pinMode(DISABLE_CS_PIN, OUTPUT);
        digitalWrite(DISABLE_CS_PIN, HIGH);
    }
    cout << F("\nAssuming the SD chip select pin is: ") << int(config.csPin);
    cout << F("\nEdit SD_CS_PIN to change the SD chip select pin.\n");
}
//------------------------------------------------------------------------------
void printConfig(SdioConfig config) {
    (void)config;
    cout << F("Assuming an SDIO interface.\n");
}

void reformatMsg() {
    Serial.print(F("Try reformatting the card.  For best results use\n"));
    Serial.print(F("the SdFormatter program in SdFat/examples or download\n"));
    Serial.print(F("and use SDFormatter from www.sdcard.org/downloads.\n"));
}

//------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial) {
        // wait for native usb
        delay(100);
    }

#ifndef EXTERNAL_FLASH_USE_QSPI
    Serial.print(F("\nOnboard Flash SPI pins:\n"));
    Serial.print(F("EXTERNAL_FLASH_USE_CS:   "));
    Serial.print(int(EXTERNAL_FLASH_USE_CS));
    Serial.println();
#endif

    Serial.print(F("\nSD Card SPI pins:\n"));
    Serial.print(F("SDCARD_MISO_PIN: "));
    Serial.print(int(SDCARD_MISO_PIN));
    Serial.println();
    Serial.print(F("SDCARD_MOSI_PIN: "));
    Serial.print(int(SDCARD_MOSI_PIN));
    Serial.println();
    Serial.print(F("SDCARD_SCK_PIN:  "));
    Serial.print(int(SDCARD_SCK_PIN));
    Serial.println();
    Serial.print(F("SDCARD_SS_PIN:   "));
    Serial.print(int(SDCARD_SS_PIN));
    Serial.println();

    cout << F("SdFat version: ") << SD_FAT_VERSION_STR << endl;
    printConfig(customSdConfig);

    // Start the SPI library
    SDCARD_SPI.begin();

#if 0
    // disable hardware slave select
    // This is needed because the slave select pin is on the wrong SPI pad on
    // the Stonefly 0.1
    // All of this has to happen **after** calling SPI.begin(), which will reset
    // the entirety of CTRLB!
    Serial.println(F("\nDisabling SPI hardware slave select (MSSEN)"));
    // First, disable SPI (CTRL.ENABLE=0)
    // All of the bits of CTRL B (except RXEN) are "Enable-Protected" - they
    // cannot be written while the peripheral is enabled Setting the CTRLB
    // register
    sercom4.disableSPI();  // NOTE: sercom4 is as sercom object,
                           // SERCOM4 is a specific memory address.
    // Now set the CTRLB register to disable hardware SS
    SERCOM4->SPI.CTRLB.bit.MSSEN = 0;  // Disable MSSEN
    while (SERCOM4->SPI.SYNCBUSY.bit.CTRLB == 1)
        ;  // not required, the MSSEN bit is not synchronized
    // Re-enable SPI
    sercom4.enableSPI();
    Serial.println(F("Correcting the pin peripheral mode for the SS pin"));
    // Ensure the pin-periperal type for the SS pin is digital I/O so it can be
    // set high and low manually
    // NOTE: Setting the pin peripheral to digital actually happens in the
    // pinMode function. Calling the pinPeripheral(x, PIO_DIGITAL) just calls
    // the pinMode fxn.
    pinPeripheral(
        SDCARD_SS_PIN,
        PIO_OUTPUT);  // PIO_OUTPUT = PIO_DIGITAL + pinMode( ulPin, OUTPUT )

    // In order to prevent the SD card library from calling SPI.begin ever
    // again, we need to make sure we set up the SD card object with a
    // SdSpiConfig object with option "USER_SPI_BEGIN."
#endif
}
//------------------------------------------------------------------------------
void loop() {
    // Read any existing Serial data.
    clearSerialInput();

    if (!firstTry) { Serial.print(F("\nRestarting\n")); }
    firstTry = false;

#if 1
    // Power up the SD card
    Serial.print(F("Power cycling the SD card with pin "));
    Serial.println(sdCardPwrPin);
    Serial.println();
    pinMode(sdCardPwrPin, OUTPUT);
    digitalWrite(sdCardPwrPin, LOW);
    delay(1000L);
    digitalWrite(sdCardPwrPin, HIGH);
    delay(1000L);
#endif

    Serial.print("Starting up onboard QSPI Flash...");
    onboardFlash.begin();
    Serial.println("Done");
    Serial.println("Onboard Flash information");
    Serial.print("JEDEC ID: 0x");
    Serial.println(onboardFlash.getJEDECID(), HEX);
    Serial.print("Flash size: ");
    Serial.print(onboardFlash.size() / 1024);
    Serial.println(" KB");


    if (DISABLE_CS_PIN >= 0) {
        Serial.print(F("\nDisabling SPI device on pin "));
        Serial.print(int(DISABLE_CS_PIN));
        Serial.println();
        pinMode(DISABLE_CS_PIN, OUTPUT);
        digitalWrite(DISABLE_CS_PIN, HIGH);
    }

    uint32_t t = millis();
    if (!sd.cardBegin(customSdConfig)) {
        Serial.println("\ncardBegin failed");
        errorPrint();

        Serial.print(F("\nType any character to restart.\n"));
        while (!Serial.available()) { ; }
        return;
    }

    t = millis() - t;
    cout << F("init time: ") << dec << t << " ms" << endl;

    if (!sd.card()->readCID(&cid)) {
        cout << F("readCID failed\n");
        errorPrint();

        Serial.print(F("\nType any character to restart.\n"));
        while (!Serial.available()) { ; }
        return;
    }

    if (!sd.card()->readCSD(&csd)) {
        cout << F("readCSD failed\n");
        errorPrint();

        Serial.print(F("\nType any character to restart.\n"));
        while (!Serial.available()) { ; }
        return;
    }

    if (!sd.card()->readOCR(&ocr)) {
        cout << F("readOCR failed\n");
        errorPrint();

        Serial.print(F("\nType any character to restart.\n"));
        while (!Serial.available()) { ; }
        return;
    }

    if (!sd.card()->readSCR(&scr)) {
        cout << F("readSCR failed\n");
        errorPrint();

        Serial.print(F("\nType any character to restart.\n"));
        while (!Serial.available()) { ; }
        return;
    }

    printCardType();

    cout << F("sdSpecVer: ") << 0.01 * scr.sdSpecVer() << endl;
    cout << F("HighSpeedMode: ");
    if (scr.sdSpecVer() > 101 && sd.card()->cardCMD6(0X00FFFFFF, cmd6Data) &&
        (2 & cmd6Data[13])) {
        cout << F("true\n");
    } else {
        cout << F("false\n");
    }
    cidDmp();
    csdDmp();
    cout << F("\nOCR: ") << uppercase << showbase;
    cout << hex << ocr << dec << endl;

    if (!mbrDmp()) {
        Serial.print(F("\nType any character to restart.\n"));
        while (!Serial.available()) { ; }
        return;
    }

    if (!sd.volumeBegin()) {
        Serial.println("volumeBegin failed");
        errorPrint();

        Serial.print(F("\nType any character to restart.\n"));
        while (!Serial.available()) { ; }
        return;
    }

    Serial.print(F("\nCard successfully initialized.\n"));
    Serial.println();

    uint32_t size = sd.card()->sectorCount();
    if (size == 0) {
        Serial.print(F("Can't determine the card size.\n"));
        cardOrSpeed();
        return;
    }
    uint32_t sizeMB = 0.000512 * size + 0.5;
    Serial.print(F("Card size: "));
    Serial.print(sizeMB);
    Serial.print(F(" MB (MB = 1,000,000 bytes)\n"));
    Serial.println();

    dmpVol();

    Serial.print(F("Files found (date time size name):\n"));
    sd.ls(LS_R | LS_DATE | LS_SIZE);

    if ((sizeMB > 1100 && sd.vol()->sectorsPerCluster() < 64) ||
        (sizeMB < 2200 && sd.vol()->fatType() == 32)) {
        Serial.print(
            F("\nThis card should be reformatted for best performance.\n"));
        Serial.print(
            F("Use a cluster size of 32 KB for cards larger than 1 GB.\n"));
        Serial.print(
            F("Only cards larger than 2 GB should be formatted FAT32.\n"));
        reformatMsg();
        return;
    }

    // Read any extra Serial data.
    clearSerialInput();

    Serial.print(F("\nSuccess!  Type any character to restart.\n"));
    while (!Serial.available()) { ; }
}
