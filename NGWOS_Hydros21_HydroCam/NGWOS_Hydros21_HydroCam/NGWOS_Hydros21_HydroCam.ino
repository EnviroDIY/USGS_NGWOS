/** =========================================================================
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 * ======================================================================= */

// Define the modem you will use
// #define BUILD_MODEM_ESPRESSIF_ESP32
#define BUILD_MODEM_SIM_COM_SIM7080
// Select Sensors
// #define USE_VEGA_PULS
#define USE_METER_HYDROS21
#define USE_GEOLUX_HYDROCAM

// Defines to help me print strings
// this converts to string
#define STR_(X) #X
// this makes sure the argument is expanded before converting to string
#define STR(X) STR_(X)

// ==========================================================================
// Defines for TinyGSM
// ==========================================================================
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 64
#endif
#ifndef TINY_GSM_YIELD_MS
#define TINY_GSM_YIELD_MS 2
#endif
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 240
#endif

// ==========================================================================
// Include the libraries required for any data logger
// ==========================================================================
// The Arduino library is needed for every Arduino program.
#include <Arduino.h>

// Include the main header for ModularSensors
#include <ModularSensors.h>

// ==========================================================================
// Assigning Serial Port Functionality
// ==========================================================================
#define modemSerial SerialBee
#define cameraSerial Serial1

// ==========================================================================
// Data Logging Options
// ==========================================================================
// The name of this program file
const char* sketchName = "NGWOS_Hydros21_HydroCam.ino";
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char* LoggerID = "24008";
// How frequently (in minutes) to log data
const uint8_t loggingInterval = 5;
// Your logger's timezone.
const int8_t timeZone = -5;  // Eastern Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!

// Set the input and output pins for the logger
// NOTE:  Use -1 for pins that do not apply
const int32_t serialBaud    = 115200;  // Baud rate for debugging
const int8_t  greenLED      = 8;       // Pin for the green LED
const int8_t  redLED        = 9;       // Pin for the red LED
const int8_t  buttonPin     = 21;  // Pin for debugging mode (ie, button pin)
uint8_t       buttonPinMode = INPUT_PULLDOWN;  // mode for debugging pin
const int8_t  wakePin       = 38;  // MCU interrupt/alarm pin to wake from sleep
uint8_t       wakePinMode   = INPUT_PULLUP;  // mode for wake pin
// const int8_t sdCardPwrPin   = 32;  // MCU SD card power pin
const int8_t sdCardPwrPin   = -1;  // MCU SD card power pin
const int8_t sdCardSSPin    = 29;  // SD card chip select/slave select pin
const int8_t flashSSPin     = 20;  // onboard flash chip select/slave select pin
const int8_t sensorPowerPin = 22;  // MCU pin controlling main sensor power
const int8_t relayPowerPin = 41;  // MCU pin controlling an optional power relay

// ==========================================================================
// The Logger Object[s]
// ==========================================================================
// Create a new logger instance
// NOTE: This is an empty instance! We will need to call setLoggerID,
// setLoggingInterval, setVariableArray, and the various pin assignment
// functions in the setup!
Logger dataLogger;

// ==========================================================================
// Wifi/Cellular Modem Options
//    NOTE:  DON'T USE MORE THAN ONE MODEM OBJECT!
//           Delete the sections you are not using!
// ==========================================================================

// Network connection information
// APN for cellular connection
#define CELLULAR_APN "hologram"

// WiFi access point name
#define WIFI_ID "YourWiFiSSID"
// WiFi password (WPA2)
#define WIFI_PASSWD "YourWiFiPassword"

#if defined(BUILD_MODEM_ESPRESSIF_ESP32)
// For almost anything based on the Espressif ESP8266 using the
// AT command firmware
#include <modems/EspressifESP32.h>

// NOTE: Extra hardware and software serial ports are created in the "Settings
// for Additional Serial Ports" section
const int32_t modemBaud = 57600;  // Communication speed of the modem
// NOTE:  This baud rate too fast for an 8MHz board, like the Mayfly!  The
// module should be programmed to a slower baud rate or set to auto-baud using
// the AT+UART_CUR or AT+UART_DEF command.

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
const int8_t modemVccPin   = 18;      // MCU pin controlling modem power
const int8_t modemResetPin = 24;      // MCU pin connected to modem reset pin
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
// ==========================================================================

#elif defined(BUILD_MODEM_SIM_COM_SIM7080)
// For almost anything based on the SIMCom SIM7080G
#include <modems/SIMComSIM7080.h>

// NOTE: Extra hardware and software serial ports are created in the "Settings
// for Additional Serial Ports" section
const int32_t modemBaud =
    9600;  //  SIM7080 does auto-bauding by default, but I set mine to 9600

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
const int8_t modemVccPin     = 18;  // MCU pin controlling modem power
const int8_t modemStatusPin  = 19;  // MCU pin used to read modem status
const int8_t modemSleepRqPin = 23;  // MCU pin for modem sleep/wake request
const int8_t modemLEDPin = redLED;  // MCU pin connected an LED to show modem
                                    // status

// Network connection information
const char* apn = CELLULAR_APN;  // APN for GPRS connection

// Create the modem object
SIMComSIM7080 modem7080(&modemSerial, modemVccPin, modemStatusPin,
                        modemSleepRqPin, apn);
// Create an extra reference to the modem by a generic name
SIMComSIM7080 modem = modem7080;
// ==========================================================================
#endif

// ==========================================================================
// Using the Processor as a Sensor
// ==========================================================================
#include <sensors/ProcessorStats.h>

// Create the main processor chip "sensor" - for general metadata
const char*    mcuBoardVersion = "v0.1";
ProcessorStats mcuBoard(mcuBoardVersion, 5);

// ==========================================================================
// Everlight ALS-PT19 Ambient Light Sensor
// ==========================================================================
#include <sensors/EverlightALSPT19.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const int8_t  alsPower      = sensorPowerPin;  // Power pin
const int8_t  alsData       = A8;              // The ALS PT-19 data pin
const int8_t  alsSupply     = 3.3;  // The ALS PT-19 supply power voltage
const int8_t  alsResistance = 10;   // The ALS PT-19 loading resistance (in kΩ)
const uint8_t alsNumberReadings = 10;

// Create a Everlight ALS-PT19 sensor object
EverlightALSPT19 alsPt19(alsPower, alsData, alsSupply, alsResistance,
                         alsNumberReadings);

#ifdef USE_GEOLUX_HYDROCAM
// ==========================================================================
// Geolux HydroCam camera
// ==========================================================================
#include <sensors/GeoluxHydroCam.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const int8_t cameraPower        = sensorPowerPin;  // Power pin
const int8_t cameraAdapterPower = sensorPowerPin;  // Power pin
const char*  imageResolution    = "1600x1200";
const char*  filePrefix         = "HydroCam";
bool         alwaysAutoFocus    = false;

// Create a GeoluxHydroCam sensor object
GeoluxHydroCam hydrocam(cameraSerial, cameraPower, dataLogger,
                        cameraAdapterPower, imageResolution, filePrefix,
                        alwaysAutoFocus);
#endif

// ==========================================================================
// Sensirion SHT4X Digital Humidity and Temperature Sensor
// ==========================================================================
#include <sensors/SensirionSHT4x.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const int8_t SHT4xPower     = sensorPowerPin;  // Power pin
const bool   SHT4xUseHeater = true;

// Create an Sensirion SHT4X sensor object
SensirionSHT4x sht4x(SHT4xPower, SHT4xUseHeater);

#ifdef USE_VEGA_PULS
// ==========================================================================
// VEGA PULS 21 Radar Sensor
// ==========================================================================
#include <sensors/VegaPuls21.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const char* VegaPulsSDI12address = "0";  // The SDI-12 Address of the VegaPuls10
const int8_t VegaPulsPower       = sensorPowerPin;  // Power pin
const int8_t VegaPulsData        = 3;               // The SDI-12 data pin
// NOTE:  you should NOT take more than one readings.  THe sensor already takes
// and averages 8 by default.

// Create a Campbell VegaPusl21 sensor object
VegaPuls21 VegaPuls(*VegaPulsSDI12address, VegaPulsPower, VegaPulsData);
#endif

#ifdef USE_METER_HYDROS21
// ==========================================================================
// Meter Hydros 21 Conductivity, Temperature, and Depth Sensor
// ==========================================================================
#include <sensors/MeterHydros21.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const char*   hydros21SDI12address = "0";  // The SDI-12 Address of the Hydros21
const uint8_t hydros21NumberReadings = 6;  // The number of readings to average
const int8_t  hydros21Power          = sensorPowerPin;  // Power pin
const int8_t  hydros21Data           = 3;               // The SDI-12 data pin

// Create a Decagon Hydros21 sensor object
MeterHydros21 hydros21(*hydros21SDI12address, hydros21Power, hydros21Data,
                       hydros21NumberReadings);
#endif

// ==========================================================================
// Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================
// Version 1: Create pointers for all of the variables from the sensors,
// at the same time putting them into an array
Variable* variableList[] = {
#ifdef USE_VEGA_PULS
    new VegaPuls21_Stage(&VegaPuls, "12345678-abcd-1234-ef00-1234567890ab"),
    new VegaPuls21_Distance(&VegaPuls, "12345678-abcd-1234-ef00-1234567890ab"),
    new VegaPuls21_Temp(&VegaPuls, "12345678-abcd-1234-ef00-1234567890ab"),
    new VegaPuls21_Reliability(&VegaPuls,
                               "12345678-abcd-1234-ef00-1234567890ab"),
    new VegaPuls21_ErrorCode(&VegaPuls, "12345678-abcd-1234-ef00-1234567890ab"),
#endif
#ifdef USE_METER_HYDROS21
    new MeterHydros21_Cond(&hydros21, "12345678-abcd-1234-ef00-1234567890ab"),
    new MeterHydros21_Temp(&hydros21, "12345678-abcd-1234-ef00-1234567890ab"),
    new MeterHydros21_Depth(&hydros21, "12345678-abcd-1234-ef00-1234567890ab"),
#endif
#ifdef USE_GEOLUX_HYDROCAM
    new GeoluxHydroCam_ImageSize(&hydrocam,
                                 "12345678-abcd-1234-ef00-1234567890ab"),
#endif
    new SensirionSHT4x_Humidity(&sht4x, "12345678-abcd-1234-ef00-1234567890ab"),
    new SensirionSHT4x_Temp(&sht4x, "12345678-abcd-1234-ef00-1234567890ab"),
    new EverlightALSPT19_Illuminance(&alsPt19,
                                     "12345678-abcd-1234-ef00-1234567890ab"),
    new ProcessorStats_Battery(&mcuBoard,
                               "12345678-abcd-1234-ef00-1234567890ab"),
    new Modem_SignalPercent(&modem, "12345678-abcd-1234-ef00-1234567890ab"),
    new ProcessorStats_SampleNumber(&mcuBoard,
                                    "12345678-abcd-1234-ef00-1234567890ab"),
};
// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);
// Create the VariableArray object
VariableArray varArray(variableCount, variableList);
// ==========================================================================

// ==========================================================================
// A Publisher to Monitor My Watershed / EnviroDIY Data Sharing Portal
// ==========================================================================
// Device registration and sampling feature information can be obtained after
// registration at https://monitormywatershed.org or https://data.envirodiy.org
const char* registrationToken =
    "12345678-abcd-1234-ef00-1234567890ab";  // Device registration token
const char* samplingFeature =
    "12345678-abcd-1234-ef00-1234567890ab";  // Sampling feature UUID

// Create a data publisher for the Monitor My Watershed/EnviroDIY POST endpoint
#include <publishers/EnviroDIYPublisher.h>
EnviroDIYPublisher EnviroDIYPOST(dataLogger, &modem.gsmClient,
                                 registrationToken, samplingFeature);

// ==========================================================================
// Working Functions
// ==========================================================================
// Flashes the LED's on the primary board
void greenRedFlash(uint8_t numFlash = 4, uint8_t rate = 75) {
    // Set up pins for the LED's
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Flash the lights
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

// Uses the processor sensor object to read the battery voltage
// NOTE: This will actually return the battery level from the previous update!
float getBatteryVoltage() {
    if (mcuBoard.sensorValues[PROCESSOR_BATTERY_VAR_NUM] == -9999 ||
        mcuBoard.sensorValues[PROCESSOR_BATTERY_VAR_NUM] == 0) {
        mcuBoard.update();
    }
    return mcuBoard.sensorValues[PROCESSOR_BATTERY_VAR_NUM];
}

// ==========================================================================
// Arduino Setup Function
// ==========================================================================
void setup() {
    // Blink the LEDs to show the board is on and starting up
    greenRedFlash(3, 35);

// Wait for USB connection to be established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this adds an
// unnecessary startup delay
#if defined(SERIAL_PORT_USBVIRTUAL)
    while (!SERIAL_PORT_USBVIRTUAL && (millis() < 10000L)) {
        // wait
    }
#endif

    // Start the primary serial connection
    Serial.begin(serialBaud);
#if defined(MS_2ND_OUTPUT)
    MS_2ND_OUTPUT.begin(serialBaud);
#endif
    greenRedFlash(5, 50);

    // Print a start-up note to the first serial port
    PRINTOUT("\n\n\n=============================");
    PRINTOUT("=============================");
    PRINTOUT("=============================");
    PRINTOUT(F("\n\nNow running"), sketchName, F(" on Logger"), LoggerID, '\n');

    PRINTOUT(F("Using ModularSensors Library version"),
             MODULAR_SENSORS_VERSION);
    PRINTOUT(F("TinyGSM Library version"), TINYGSM_VERSION, '\n');
    PRINTOUT(F("Processor:"), mcuBoard.getSensorLocation());
    PRINTOUT(F("The most recent reset cause was"), mcuBoard.getLastResetCode(),
             '(', mcuBoard.getLastResetCause(), ')');

    // Start the serial connection with the modem
    PRINTOUT(F("Starting modem connection on"), STR(modemSerial), F(" at"),
             modemBaud, F(" baud"));
    modemSerial.begin(modemBaud);

    // Start the stream for the camera; it will always be at 115200 baud
    cameraSerial.begin(115200);

    // Start the SPI library
    PRINTOUT(F("Starting SPI"));
    SPI.begin();

#if defined(EXTERNAL_FLASH_DEVICES)
    PRINTOUT(F("Setting onboard flash pin modes"));
    pinMode(flashSSPin,
            OUTPUT);  // for proper operation of the onboard flash memory
#endif

    PRINTOUT(F("Starting I2C (Wire)"));
    Wire.begin();

    // set the logger ID
    PRINTOUT(F("Setting logger id to"), LoggerID);
    dataLogger.setLoggerID(LoggerID);
    // set the logging interval
    PRINTOUT(F("Setting logging interval to"), loggingInterval, F("minutes"));
    dataLogger.setLoggingInterval(loggingInterval);
    PRINTOUT(F("Setting number of initial 1 minute intervals to 10"));
    dataLogger.setinitialShortIntervals(10);
    // Attach the variable array to the logger
    PRINTOUT(F("Attaching the variable array"));
    dataLogger.setVariableArray(&varArray);
    // set logger pins
    PRINTOUT(F("Setting logger pins"));
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED, wakePinMode, buttonPinMode);

#if defined(ARDUINO_ARCH_SAMD)
    PRINTOUT(F("Setting analog read resolution for onboard ADC to 12 bit"));
    analogReadResolution(12);
#endif

    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    PRINTOUT(F("Setting logger time zone"));
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    PRINTOUT(F("Setting RTC time zone"));
    loggerClock::setRTCOffset(0);

    // Attach the modem and information pins to the logger
    PRINTOUT(F("Attaching the modem"));
    dataLogger.attachModem(modem);
    PRINTOUT(F("Setting modem LEDs"));
    modem.setModemLED(modemLEDPin);

    // Begin the logger
    PRINTOUT(F("Beginning the logger"));
    dataLogger.begin();

    // Note:  Please change these battery voltages to match your battery
    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4) {
        PRINTOUT(F("Setting up sensors..."));
        varArray.sensorsPowerUp();
        varArray.setupSensors();
        varArray.sensorsPowerDown();
    }

#if (defined BUILD_MODEM_ESPRESSIF_ESP8266 || \
     defined BUILD_MODEM_ESPRESSIF_ESP32)
    PRINTOUT(F("Waking the modem.."));
    PRINTOUT(F("Attempting to begin modem communication at"), modemBaud,
             F("baud.  This will fail if the baud is mismatched.."));
    modemSerial.begin(modemBaud);
    modem.modemWake();  // NOTE:  This will also set up the modem
    if (!modem.gsmModem.testAT()) {
        PRINTOUT(F("Attempting autobauding.."));
        uint32_t foundBaud = TinyGsmAutoBaud(modemSerial);
        if (foundBaud != 0 || F_CPU == 8000000L) {
            PRINTOUT(F("Got modem response at baud of"), foundBaud,
                     F("Firing an attempt to change the baud rate to"),
                     modemBaud);
            modem.gsmModem.sendAT(GF("+UART_DEF="), modemBaud, F(",8,1,0,0"));
            modem.gsmModem.waitResponse();
            modemSerial.end();
            modemSerial.begin(modemBaud);
        }
    }
#endif

#if defined(BUILD_MODEM_SIM_COM_SIM7080)
    modem.setModemWakeLevel(HIGH);   // ModuleFun Bee inverts the signal
    modem.setModemResetLevel(HIGH);  // ModuleFun Bee inverts the signal
    PRINTOUT(F("Waking modem and setting Cellular Carrier Options..."));
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
#endif

    // Sync the clock if it isn't valid or we have battery to spare
    if (getBatteryVoltage() > 3.55 || !loggerClock::isRTCSane()) {
        // Synchronize the RTC with NIST
        // This will also set up the modem
        dataLogger.syncRTC();
    }

    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.
    if (getBatteryVoltage() > 3.4) {
        PRINTOUT(F("Setting up file on SD card"));
        delay(50);
        dataLogger.turnOnSDcard(true);
        // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(true);
        // true = wait for internal housekeeping after write
    }

    // Call the processor sleep
    PRINTOUT(F("Putting processor to sleep\n"));
    dataLogger.systemSleep();
}

// ==========================================================================
// Arduino Loop Function
// ==========================================================================
// Use this short loop for simple data logging and sending
void loop() {
    // Note:  Please change these battery voltages to match your battery
    // At very low battery, just go back to sleep
    if (getBatteryVoltage() < 3.4) {
        PRINTOUT(F("Battery too low, ("),
                 mcuBoard.sensorValues[PROCESSOR_BATTERY_VAR_NUM],
                 F("V) going back to sleep."));
        dataLogger.systemSleep();
    } else if (getBatteryVoltage() < 3.55) {
        // At moderate voltage, log data but don't send it over the modem
        PRINTOUT(F("Battery at"),
                 mcuBoard.sensorValues[PROCESSOR_BATTERY_VAR_NUM],
                 F("V; high enough to log, but will not publish!"));
        dataLogger.logData();
    } else {
        // If the battery is good, send the data to the world
        PRINTOUT(F("Battery at"),
                 mcuBoard.sensorValues[PROCESSOR_BATTERY_VAR_NUM],
                 F("V; high enough to log and publish data"));
        dataLogger.logDataAndPublish();
    }
}
