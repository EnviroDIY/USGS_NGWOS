#include <Arduino.h>
#include "wiring_private.h"  // pinPeripheral() function

// Select your LoRa Module:
#define TINY_GSM_MODEM_ESP8266

#define TINY_GSM_YIELD_MS 2
#define TINY_GSM_RX_BUFFER 512

#define SDI12_YIELD_MS 16

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG Serial

// Include the TinyGSM library
#include <TinyGsmClient.h>
// RTC Library (On-board clock)
#include <SparkFun_RV8803.h>
// Adafruit SHT library (On-board humidity and temperature)
#include "Adafruit_SHT4x.h"
// Adafruit MAX1704x library (On-board battery monitor)
#include <Adafruit_MAX1704X.h>
// The SDI-12 library for the Vega Puls
#include <SDI12.h>
// To communicate with the SD card
#include <SdFat.h>
// Local H files with separated fxns
#include "SDI12Master.h"

#include "LoggerBase.h"


// ==========================================================================
//  Data Logging Options
// ==========================================================================
/** Start [logging_options] */
// The name of this program file
const char* sketchName = "imgbb.ino";
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char* LoggerID = "Stonefly2";
// How frequently (in minutes) to log data
const uint8_t loggingInterval = 1;
// Your logger's timezone.
const int8_t timeZone = -5;  // Eastern Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!

// Set the input and output pins for the logger
// NOTE:  Use -1 for pins that do not apply
const int32_t serialBaud = 115200;  // Baud rate for debugging
const int8_t  greenLED   = 8;       // Pin for the green LED
const int8_t  redLED     = 9;       // Pin for the red LED
const int8_t  buttonPin  = 21;      // Pin for debugging mode (ie, button pin)
const int8_t  wakePin    = 38;  // MCU interrupt/alarm pin to wake from sleep
// Stonefly 0.1 would have SD power on pin 32, but the MOSFET circuit is bad
// const int8_t sdCardPwrPin = 32;  // MCU SD card power pin
const int8_t sdCardPwrPin   = -1;  // MCU SD card power pin
const int8_t sdCardSSPin    = 29;  // SD card chip select/slave select pin
const int8_t sensorPowerPin = 22;  // MCU pin controlling main sensor power
/** End [logging_options] */


// ==========================================================================
//  Wifi/Cellular Modem Options
// ==========================================================================

#define modemSerial SerialBee

// Network connection information
#define STROUD_APN "hologram"  // APN for GPRS connection

#if defined(USE_HOME_WIFI)
#define WIFI_ID "YourWiFiSSID"  // WiFi access point name
#define WIFI_PASSWD "YourWiFiPassword"   // WiFi password (WPA2)
#elif defined(USE_WIFI_HOTSPOT)
#define WIFI_ID "YourWiFiSSID"    // WiFi access point
#define WIFI_PASSWD "YourWiFiPassword"  // WiFi password
#else
#define WIFI_ID "YourWiFiSSID"  // WiFi access point name
#define WIFI_PASSWD "YourWiFiPassword"   // WiFi password (WPA2)
#endif

#if defined BUILD_MODEM_ESPRESSIF_ESP32
/** Start [espressif_esp32] */
// For almost anything based on the Espressif ESP8266 using the
// AT command firmware
#include <modems/EspressifESP32.h>

// NOTE: Extra hardware and software serial ports are created in the "Settings
// for Additional Serial Ports" section
const int32_t modemBaud = 115200;  // Communication speed of the modem

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
// Example pins here are for a imgbb ESP32 Bluetooth/Wifi Bee with
// Stonefly 0.1
const int8_t modemVccPin   = 18;      // MCU pin controlling modem power
const int8_t modemResetPin = -1;      // MCU pin connected to modem reset pin
const int8_t modemLEDPin   = redLED;  // MCU pin connected an LED to show modem
                                      // status

// Network connection information
const char* wifiId  = WIFI_ID;      // WiFi access point name
const char* wifiPwd = WIFI_PASSWD;  // WiFi password (WPA2)

// Create the modem object
EspressifESP32 modemESP(&modemSerial, modemVccPin, modemResetPin, wifiId,
                        wifiPwd);
// Create an extra reference to the modem by a generic name
EspressifESP32 modem = modemESP;
/** End [espressif_esp32] */
// ==========================================================================


#elif defined BUILD_MODEM_SIM_COM_SIM7080
/** Start [sim_com_sim7080] */
// For almost anything based on the SIMCom SIM7080G
#include <modems/SIMComSIM7080.h>

// NOTE: Extra hardware and software serial ports are created in the "Settings
// for Additional Serial Ports" section
const int32_t modemBaud =
    9600;  //  SIM7080 does auto-bauding by default, but I set mine to 9600

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
// and-global breakout bk-7080a
const int8_t modemVccPin     = 18;  // MCU pin controlling modem power
const int8_t modemStatusPin  = 19;  // MCU pin used to read modem status
const int8_t modemSleepRqPin = 23;  // MCU pin for modem sleep/wake request
const int8_t modemLEDPin = redLED;  // MCU pin connected an LED to show modem
                                    // status

// Network connection information
const char* apn = STROUD_APN;  // APN for GPRS connection

// Create the modem object
SIMComSIM7080 modem7080(&modemSerial, modemVccPin, modemStatusPin,
                        modemSleepRqPin, apn);
// Create an extra reference to the modem by a generic name
SIMComSIM7080 modem = modem7080;
/** End [sim_com_sim7080] */
#endif


// ==========================================================================
//  SDI-12 Connection for the Vega Puls
// ==========================================================================

// The pin of the SDI-12 data bus
const int8_t sdiDataPin = 3;
// The Vega's Address
const char vega_address = '0';
// Define the SDI-12 bus
SDI12 mySDI12(sdiDataPin);


// ==========================================================================
//  Other on-board sensors
// ==========================================================================

// Create the SHT object
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// Create the battery monitor object
Adafruit_MAX17048 maxlipo;

const int8_t alsDataPin = 74;


// ==========================================================================
//  Camera Stuff
// ==========================================================================
File        imgFile;
const char* testImgName = "PIC087.jpg";


// ==========================================================================
//  The Logger Object[s]
// ==========================================================================
/** Start [loggers] */
// Create a new logger instance
Logger dataLogger(LoggerID, loggingInterval);
/** End [loggers] */

// ==========================================================================
//  A Publisher to https://imgbb.com/
// ==========================================================================
/** Start [imgbb_publisher] */

#ifndef MS_SEND_BUFFER_SIZE
/**
 * @brief Send Buffer
 *
 * This determines how many characters to set out at once over the TCP
 * connection. Increasing this may decrease data use by a logger, while
 * decreasing it will save memory. Do not make it smaller than 32 or bigger
 * than 1500 (a typical TCP Maximum Transmission Unit).
 *
 * This can be changed by setting the build flag MS_SEND_BUFFER_SIZE when
 * compiling.
 */
#define MS_SEND_BUFFER_SIZE 750
#endif

char    txBuffer[MS_SEND_BUFFER_SIZE];
Client* txBufferOutClient = nullptr;
size_t  txBufferLen;

// Basic chunks of HTTP
const char* getHeader  = "GET ";
const char* postHeader = "POST ";
const char* HTTPtag    = " HTTP/1.1";
const char* hostHeader = "\r\nHost: ";

const char* apiKey = "a6d42bad9b420a5031cc111b749fd887";  // Imgbb's API v1 key

const char* imgbbHost           = "api.imgbb.com";  ///< The host name
int         imgbbPort           = 80;               ///< The host port
const char* imgbbPath           = "/1/upload";      ///< The api path
const char* contentLengthHeader = "\r\nContent-Length: ";
const char* formBoundaryStr     = "------WebKitFormBoundary7MA4YWxkTrZu0gW";
const char* contentTypeHeader =
    "\r\nContent-Type: multipart/form-data; "
    "boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n\r\n";


void txBufferInit(Client* outClient) {
    // remember client we are sending to
    txBufferOutClient = outClient;

    // reset buffer length to be empty
    txBufferLen = 0;
}

void txBufferAppend(const char* data, size_t length) {
    while (length > 0) {
        // space left in the buffer
        size_t remaining = MS_SEND_BUFFER_SIZE - txBufferLen;
        // the number of characters that will be added to the buffer
        // this will be the lesser of the length desired and the space left in
        // the buffer
        size_t amount = remaining < length ? remaining : length;

        // copy as much as possible into the buffer
        memcpy(&txBuffer[txBufferLen], data, amount);
        // re-count how much is left to go
        length -= amount;
        // bump forward the pointer to where we're currently adding
        data += amount;
        // bump up the current length of the buffer
        txBufferLen += amount;

        // write out the buffer if it fills
        if (txBufferLen == MS_SEND_BUFFER_SIZE) { txBufferFlush(); }
    }
}

void txBufferAppend(const char* s) {
    txBufferAppend(s, strlen(s));
}

void txBufferAppend(char c) {
    txBufferAppend(&c, 1);
}

void txBufferFlush() {
    if ((txBufferOutClient == nullptr) || (txBufferLen == 0)) {
        // sending into the void...
        txBufferLen = 0;
        return;
    }

#if defined(STANDARD_SERIAL_OUTPUT)
    // write out to the printout stream
    STANDARD_SERIAL_OUTPUT.write((const uint8_t*)txBuffer, txBufferLen);
    STANDARD_SERIAL_OUTPUT.flush();
#endif
    // write out to the client
    uint8_t        tries = 10;
    const uint8_t* ptr   = (const uint8_t*)txBuffer;
    while (true) {
        size_t sent = txBufferOutClient->write(ptr, txBufferLen);
        txBufferLen -= sent;
        ptr += sent;
        if (txBufferLen == 0) {
            // whole message is successfully sent, we are done
            txBufferOutClient->flush();
            return;
        }

#if defined(STANDARD_SERIAL_OUTPUT)
        // warn that we only partially sent the buffer
        STANDARD_SERIAL_OUTPUT.write('!');
#endif
        if (--tries == 0) {
            // can't convince the modem to send the whole message. just break
            // the connection now so it will get reset and we can try to
            // transmit the data again later
            txBufferOutClient = nullptr;
            txBufferLen       = 0;
            return;
        }

        // give the modem a chance to transmit buffered data
        delay(1000);
    }
}

int16_t imgbbPublish(Client* outClient, const char* name, File& image) {
    // Create a buffer for the portions of the request and response
    char     tempBuffer[37] = "";
    uint16_t did_respond    = 0;

    // Create the file.
    if (!image.open(name, O_RDONLY)) {
        Serial.println("Failed to open file on SD card");
        return 0;
    }

    // Open a TCP/IP connection to Imgbb
    MS_DBG(F("Connecting client"));
    MS_START_DEBUG_TIMER;
    if (outClient->connect(imgbbHost, imgbbPort)) {
        MS_DBG(F("Client connected after"), MS_PRINT_DEBUG_TIMER, F("ms\n"));
        txBufferInit(outClient);

        // copy the initial post header into the tx buffer
        txBufferAppend(postHeader);
        txBufferAppend(imgbbPath);
        txBufferAppend("?key=");
        txBufferAppend(apiKey);
        txBufferAppend(HTTPtag);

        // add the rest of the HTTP POST headers to the outgoing buffer
        txBufferAppend(hostHeader);
        txBufferAppend(imgbbHost);

        txBufferAppend(contentLengthHeader);
        txBufferAppend(image.size());

        txBufferAppend(contentTypeHeader);

        txBufferAppend(formBoundaryStr);
        txBufferAppend("\r\n");
        txBufferAppend("Content-Disposition: form-data; name=\"");
        txBufferAppend(name);
        txBufferAppend("\"\r\n\r\n");

        while (image.available()) { txBufferAppend(image.read()); }

        txBufferAppend("\r\n");
        txBufferAppend(formBoundaryStr);
        txBufferAppend("--\r\n");
        txBufferAppend("\r\n");

        // Flush the complete request
        txBufferFlush();

        // Wait 30 seconds for a response from the server
        uint32_t start = millis();
        while ((millis() - start) < 30000L && outClient->available() < 12) {
            delay(10);
        }

        // Read only the first 12 characters of the response.
        // We're only reading as far as the http code, anything beyond that we
        // don't care about.
        did_respond = outClient->readBytes(tempBuffer, 12);

        // Close the TCP/IP connection
        MS_DBG(F("Stopping client"));
        MS_RESET_DEBUG_TIMER;
        outClient->stop();
        MS_DBG(F("Client stopped after"), MS_PRINT_DEBUG_TIMER, F("ms"));
    } else {
        PRINTOUT(F("\n -- Unable to Establish Connection to imgbb Data "
                   "Portal --"));
    }

    // Process the HTTP response
    int16_t responseCode = 0;
    if (did_respond > 0) {
        char responseCode_char[4];
        responseCode_char[3] = 0;
        for (uint8_t i = 0; i < 3; i++) {
            responseCode_char[i] = tempBuffer[i + 9];
        }
        responseCode = atoi(responseCode_char);
    } else {
        responseCode = 504;
    }

    PRINTOUT(F("\n-- Response Code --"));
    PRINTOUT(responseCode);

    return responseCode;
}
/** End [imgbb_publisher] */


// ==========================================================================
//  Working Functions
// ==========================================================================
/** Start [working_functions] */
// Flashes the LED's on the primary board
void greenredflash(uint8_t numFlash = 4, uint8_t rate = 75) {
    for (uint8_t i = 0; i < numFlash; i++) {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        delay(rate);
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        delay(rate);
    }
    digitalWrite(redLED, LOW);
}
void sensorPowerOn() {
    if (sensorPowerPin >= 0) {
        Serial.print("Powering SDI-12 sensors with pin ");
        Serial.println(sensorPowerPin);
        pinMode(sensorPowerPin, OUTPUT);
        digitalWrite(sensorPowerPin, HIGH);
    }
}
void sensorPowerOff() {
    if (sensorPowerPin >= 0) {
        Serial.print("Cutting power SDI-12 sensors with pin ");
        Serial.println(sensorPowerPin);
        pinMode(sensorPowerPin, OUTPUT);
        digitalWrite(sensorPowerPin, LOW);
    }
}
// Uses the processor sensor object to read the battery voltage
// NOTE: This will actually return the battery level from the previous update!
float getBatteryVoltage() {
    // Get the battery voltage
    // The return value from analogRead() is IN BITS NOT IN VOLTS!!
    float  sensorValue_battery = -9999;
    int8_t _batteryPin         = 67;
    pinMode(_batteryPin, INPUT);
    analogRead(_batteryPin);  // priming reading
    analogRead(_batteryPin);  // priming reading
    analogRead(_batteryPin);  // priming reading
    float rawBattery = analogRead(_batteryPin);
    MS_DBG(F("Raw battery pin reading in bits:"), rawBattery);
    // convert bits to volts
    sensorValue_battery = (3.3 / 1024.) * 4.7 * rawBattery;
    MS_DBG(F("Battery in Volts:"), sensorValue_battery);
    // return sensorValue_battery;
    return 4;
}
void buttonISR(void) {
    Serial1.println(F("\nButton interrupt!"));
}
/** End [working_functions] */


// ==========================================================================
//  Arduino Setup Function
// ==========================================================================
void setup() {
    // Set console baud rate
    Serial.begin(serialBaud);
    delay(10);

// Wait for USB connection to be established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this
// could prevent the script from starting
#if defined SERIAL_PORT_USBVIRTUAL
    while (!SERIAL_PORT_USBVIRTUAL && (millis() < 10000L)) {}
#endif

    Serial1.begin(serialBaud);
    delay(10);

    // Print a start-up note to the first serial port
    Serial.print(F("\n\nNow running "));
    Serial.print(sketchName);
    Serial.print(F(" on Logger "));
    Serial.println(LoggerID);
    Serial.println();

    // Start the serial connection with the modem
    Serial.print(F("Starting modem connection at "));
    Serial.print(modemBaud);
    Serial.println(F(" baud"));
    SerialBee.begin(modemBaud);

    // Set up pins for the LED's
    Serial.println(F("Flashing lights"));
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();

    // Start the SPI library
    Serial.println(F("Starting SPI"));
    SPI.begin();

    // disable hardware slave select
    // This is needed because the slave select pin is on the wrong SPI pad on
    // the Stonefly 0.1

    // NOTE: While the Adafruit core sets the MSSEN bit, it does NOT set the
    // pinPeripheral for the SS pin.

#if 0
    //  All of this has to happen **after** calling SPI.begin(), which will
    //  reset the entirety of CTRLB!
    Serial.println(F("Disabling SPI hardware slave select (MSSEN)"));
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

    pinPeripheral(sdCardSSPin, PIO_OUTPUT);

    // In order to prevent the SD card library from calling SPI.begin ever
    // again, we need to make sure we set up the SD card object with a
    // SdSpiConfig object with option "USER_SPI_BEGIN."
#endif

    Serial.println(F("Starting I2C (Wire)"));
    Wire.begin();

    Serial.println(F("Setting onboard flash pins"));
    pinMode(20, OUTPUT);  // for proper operation of the onboard flash memory
                          // chip's ChipSelect

    Serial.println(F("Setting logger pins"));
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, greenLED);

    Serial.println(
        F("Setting analog read resolution for onboard ADC to 12 bit"));
    analogReadResolution(12);

    // Attach the modem and information pins to the logger
    delay(200);
    Serial.println(F("Attaching Modem"));
    delay(200);
    dataLogger.attachModem(modem);
    modem.setModemLED(modemLEDPin);

    // Begin the logger
    Serial.println(F("Beginning the logger"));
    dataLogger.begin();

    Serial.println(F("Setting time zones"));
    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    // NOTE: with the RV8803, this must happen **AFTER** the begin
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    Logger::setRTCTimeZone(0);

    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("TODO: Set up camera!"));
        if (!sht4.begin()) {
            Serial.println(F("Couldn't find SHT4x"));
        } else {
            Serial.println(F("SHT4x online!"));
        }
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
        ;

        if (!maxlipo.begin()) {
            Serial.println(
                F("Couldn't find Adafruit MAX17048?\nMake sure a battery "
                  "is plugged in!"));
        } else {
            Serial.print(F("Found MAX17048"));
            Serial.print(F(" with Chip ID: 0x"));
            Serial.println(maxlipo.getChipID(), HEX);
        }

        Serial.print("Opening SDI-12 bus on pin ");
        Serial.print(String(sdiDataPin));
        Serial.println("...");
        mySDI12.begin();
        delay(500);  // allow things to settle

        Serial.println(F("Timeout value: "));
        Serial.println(mySDI12.TIMEOUT);

        // turn on sensor power to check Vega Puls metadata
        sensorPowerOn();
        Serial.println("Waiting 5.2s for Vega Puls to warm up");
        delay(5200);
        // Print SDI-12 sensor info
        printInfo(mySDI12, vega_address, false);
        // Turn off sensor power
        sensorPowerOff();
    }

#if defined BUILD_MODEM_SIM_COM_SIM7080
    /** Start [setup_sim7080] */
    modem.setModemWakeLevel(HIGH);   // ModuleFun Bee inverts the signal
    modem.setModemResetLevel(HIGH);  // ModuleFun Bee inverts the signal
    Serial.println(F("Waking modem and setting Cellular Carrier Options..."));
    modem.modemWake();  // NOTE:  This will also set up the modem
    modem.gsmModem.setBaud(modemBaud);   // Make sure we're *NOT* auto-bauding!
    modem.gsmModem.setNetworkMode(38);   // set to LTE only
                                         // 2 Automatic
                                         // 13 GSM only
                                         // 38 LTE only
                                         // 51 GSM and LTE only
    modem.gsmModem.setPreferredMode(1);  // set to CAT-M
                                         // 1 CAT-M
                                         // 2 NB-IoT
                                         // 3 CAT-M and NB-IoT
    /** End [setup_sim7080] */
#endif

    // Sync the clock if it isn't valid or we have battery to spare
    if (getBatteryVoltage() > 3.55 || !dataLogger.isRTCSane()) {
        // Synchronize the RTC with NIST
        // This will also set up the modem
        dataLogger.syncRTC();
    }

    // put the modem to sleep
    modem.modemSleep();

    // Confirm the date and time using the ISO 8601 timestamp
    dataLogger.rtc.updateTime();
    Serial.print(F("Current RTC timestamp:"));
    Serial.println(dataLogger.rtc.stringTime8601TZ());

    // Create the log file
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("Setting up file on SD card"));
        dataLogger.turnOnSDcard(true);
        dataLogger.createLogFile();
        // write a new header to the file
        String header = "";
        header += "Time,";
        header += "Unix Timestamp,";
        header += "modem RSSI,";
        header += "SHT Temperature,";
        header += "SHT Humidity,";
        header += "VegaPuls Stage,";
        header += "VegaPuls Distance,";
        header += "VegaPuls Temperature,";
        header += "VegaPuls Reliability,";
        header += "VegaPuls Error Code,";
        header += "ALS Lux,";
        header += "Battery Voltage,";
        header += "Battery Percent,";
        header += "Battery (Dis)Charge Rate,";
        header += "Uploaded Image Name,";
        header += "HTTP Response Code";
        Serial.println(F("Writing header to SD card"));
        Serial.println(header);
        if (dataLogger.logToSD(header)) {
            Serial.println(F("SD card success"));
        } else {
            Serial.println(F("Failed to write to SD card!"));
        }
        dataLogger.turnOffSDcard(true);
        // true = wait for internal housekeeping after write
    }

    pinMode(21, INPUT_PULLUP);
    attachInterrupt(21, buttonISR, CHANGE);

    // Call the processor sleep
    Serial.println(F("Putting processor to sleep\n"));
    dataLogger.systemSleep();
}

void loop() {
    // Reset the watchdog
    dataLogger.watchDogTimer.resetWatchDog();

    // Assuming we were woken up by the clock, check if the current time is an
    // even interval of the logging interval
    // We're only doing anything at all if the battery is above 3.4V
    if (dataLogger.checkInterval() && getBatteryVoltage() > 3.4) {
        // Flag to notify that we're in already awake and logging a point
        Logger::isLoggingNow = true;
        dataLogger.watchDogTimer.resetWatchDog();

        // Print a line to show new reading
        Serial.println(F("------------------------------------------"));
        // Turn on the LED to show we're taking a reading
        dataLogger.alertOn();
        // Power up the SD Card, but skip any waits after power up
        dataLogger.turnOnSDcard(true);
        dataLogger.watchDogTimer.resetWatchDog();

        // Turn on the modem to let it start searching for the network
        // Only turn the modem on if the battery at the last interval was high
        // enough
        if (getBatteryVoltage() > 3.55) modem.modemPowerUp();


        // Confirm the date and time using the ISO 8601 timestamp
        dataLogger.rtc.updateTime();
        Serial.print(F("Current RTC timestamp:"));
        Serial.println(dataLogger.rtc.stringTime8601TZ());
        dataLogger.watchDogTimer.resetWatchDog();

        sensorPowerOn();
        // time for the VegaPuls to warm up, kicking the dog while waiting
        uint32_t start = millis();
        while (millis() - 5200L < start) {
            dataLogger.watchDogTimer.resetWatchDog();
            delay(100L);
        }

        String csvOutput = "";

        // get the time from the on-board RTC
        // Add to the CSV
        csvOutput += dataLogger.rtc.stringTime8601TZ();
        csvOutput += ",";
        csvOutput += Logger::markedUTCEpochTime;
        dataLogger.watchDogTimer.resetWatchDog();

        // Get modem signal quality
        // NOTE: This is trivial, because the quality is added automatically
        int rssi = modem.gsmModem.getSignalQuality();
        Serial.print(F("Signal quality: "));
        Serial.println(rssi);
        csvOutput += ",";
        csvOutput += rssi;
        dataLogger.watchDogTimer.resetWatchDog();

        // Get temperature and humidity from the SHT40
        sensors_event_t humidity;
        sensors_event_t temp;
        sht4.getEvent(
            &humidity,
            &temp);  // populate temp and humidity objects with fresh data
        Serial.print(F("Temperature: "));
        Serial.println(temp.temperature, 2);
        Serial.print(F("Humidity: "));
        Serial.println(humidity.relative_humidity, 2);
        // Add to the CSV
        csvOutput += ",";
        csvOutput += String(temp.temperature, 2);
        csvOutput += ",";
        csvOutput += String(humidity.relative_humidity, 2);
        dataLogger.watchDogTimer.resetWatchDog();

        // Get SDI-12 Data
        startMeasurementResult startResult =
            startMeasurement(mySDI12, vega_address, false, true, "", true);
        dataLogger.watchDogTimer.resetWatchDog();
        if (startResult.numberResults > 0) {
            uint32_t timerStart = millis();
            // wait up to 1 second longer than the specified return time
            while ((millis() - timerStart) <
                   (static_cast<uint32_t>(startResult.meas_time_s) + 1) *
                       1000) {
                if (mySDI12.available()) {
                    break;
                }  // sensor can interrupt us to let us know it is done
                   // early
            }
            String interrupt_response = mySDI12.readStringUntil('\n');

            // array to hold sdi-12 results
            float sdi12_results[10];

            getResults(mySDI12, vega_address, startResult.numberResults,
                       sdi12_results, true, true, 4, 0);
            dataLogger.watchDogTimer.resetWatchDog();
            Serial.print(F("Stage: "));
            Serial.println(sdi12_results[0], 3);
            Serial.print(F("Distance: "));
            Serial.println(sdi12_results[1], 3);
            Serial.print(F("Temperature: "));
            Serial.println(sdi12_results[2], 1);
            Serial.print(F("Reliability: "));
            Serial.println(sdi12_results[3], 2);
            Serial.print(F("Error Code: "));
            Serial.println(sdi12_results[4], 2);
            // Add to the CSV
            csvOutput += ",";
            csvOutput += String(sdi12_results[0], 3);
            csvOutput += ",";
            csvOutput += String(sdi12_results[1], 3);
            csvOutput += ",";
            csvOutput += String(sdi12_results[2], 1);
            csvOutput += ",";
            csvOutput += String(sdi12_results[3], 1);
            csvOutput += ",";
            csvOutput += sdi12_results[4];
            dataLogger.watchDogTimer.resetWatchDog();
        }

        // measure from the als
        analogReadResolution(12);
        // First reading will be low - discard
        analogRead(alsDataPin);
        // Take the reading we'll keep
        uint32_t sensor_adc = analogRead(alsDataPin);
        // convert bits to volts
        float volt_val = (3.3 / static_cast<float>(((1 << 12) - 1))) *
            static_cast<float>(sensor_adc);
        // convert volts to current
        // resistance is entered in kΩ and we want µA
        float current_val = (volt_val / (10 * 1000)) * 1e6;
        // convert current to illuminance
        // from sensor datasheet, typical 200µA current for1000 Lux
        float lux_val = current_val * (1000. / 200.);
        Serial.print(F("Lux: "));
        Serial.println(lux_val);
        // Add to the CSV
        csvOutput += ",";
        csvOutput += String(lux_val, 1);
        dataLogger.watchDogTimer.resetWatchDog();

        // Read the battery monitor
        float cellVoltage = maxlipo.cellVoltage();
        float cellPercent = maxlipo.cellPercent();
        float chargeRate  = maxlipo.chargeRate();
        Serial.print(F("Batt Voltage: "));
        Serial.print(cellVoltage, 3);
        Serial.println(" V");
        Serial.print(F("Battery Percent: "));
        Serial.print(cellPercent, 1);
        Serial.println(" %");
        Serial.print(F("(Dis)Charge rate: "));
        Serial.print(chargeRate, 1);
        Serial.println(" %/hr");
        // Add to the CSV
        csvOutput += ",";
        csvOutput += String(cellVoltage, 3);
        csvOutput += ",";
        csvOutput += String(cellPercent, 1);
        csvOutput += ",";
        csvOutput += String(chargeRate, 1);
        dataLogger.watchDogTimer.resetWatchDog();

        // turn off the sensors since we have all data
        sensorPowerOff();

        // Save data to the SD Card

        Serial.println(F("Writing line to SD card"));
        Serial.println(csvOutput);
        if (dataLogger.logToSD(csvOutput)) {
            Serial.println(F("SD card success"));
        } else {
            Serial.println(F("Failed to write to SD card!"));
        }
        // Cut power from the SD card
        dataLogger.turnOffSDcard(true);
        dataLogger.watchDogTimer.resetWatchDog();

        // Connect to the network
        // Again, we're only doing this if the battery is doing well
        if (getBatteryVoltage() > 3.55) {
            dataLogger.watchDogTimer.resetWatchDog();
            if (modem.connectInternet()) {
                dataLogger.watchDogTimer.resetWatchDog();
                // Publish data to remotes
                Serial.println(F("Modem connected to internet."));
                imgbbPublish(&modem.gsmClient, testImgName, imgFile);

                // Sync the clock at noon
                dataLogger.watchDogTimer.resetWatchDog();
                if ((Logger::markedLocalEpochTime != 0 &&
                     Logger::markedLocalEpochTime % 86400 == 43200) ||
                    !dataLogger.isRTCSane()) {
                    Serial.println(F("Running a daily clock sync..."));
                    dataLogger.syncRTC();
                    dataLogger.watchDogTimer.resetWatchDog();
                }

                // Disconnect from the network
                modem.disconnectInternet();
                dataLogger.watchDogTimer.resetWatchDog();
            }
            // Turn the modem off
            modem.modemSleepPowerDown();
            dataLogger.watchDogTimer.resetWatchDog();
        }

        // Turn off the LED
        dataLogger.alertOff();
        // Print a line to show reading ended
        Serial.println(F("------------------------------------------\n"));
    }

    // Call the processor sleep
    dataLogger.systemSleep();
}
