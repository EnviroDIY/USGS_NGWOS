/** =========================================================================
 * @example{lineno} NGWOS_AWS_LORA.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief A testing program for the USGS with LoRa.
 * ======================================================================= */

// Select your LoRa Module:
#define LORA_AT_MDOT

// Defines to help me print strings
// this converts to string
#define STR_(X) #X
// this makes sure the argument is expanded before converting to string
#define STR(X) STR_(X)

// ==========================================================================
//  Defines for LoRa_AT
// ==========================================================================
/** Start [defines] */
#ifndef LORA_AT_YIELD_MS
#define LORA_AT_YIELD_MS 2
#endif
#ifndef SDI12_YIELD_MS
#define SDI12_YIELD_MS 16
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
// #define LORA_AT_DEBUG Serial
/** End [defines] */


// ==========================================================================
//  Include the libraries required for any data logger
// ==========================================================================
/** Start [includes] */
// The Arduino library is needed for every Arduino program.
#include <Arduino.h>
// Needed for the pinPeripheral() function
#include "wiring_private.h"

// Include the LoRa library
#include <LoRa_AT.h>
// Cayenne LPP library for compressing data for TTN
#include <CayenneLPP.h>
// RTC Library (On-board clock)
#include <SparkFun_RV8803.h>
// The SDI-12 library for the Vega Puls
#include <SDI12.h>
// Local H files with separated fxns
#include "LoRaModemFxns.h"

// Include the main header for ModularSensors
#include <ModularSensors.h>
/** End [includes] */

// ==========================================================================
//  Assigning Serial Port Functionality
// ==========================================================================
/** Start [assign_ports_hw] */
// If there are additional hardware Serial ports possible - use them!
// We give the modem first priority and assign it to hardware serial
// All of the supported processors have a hardware port available named Serial1
#define modemSerial SerialBee
#define cameraSerial Serial2
/** End [assign_ports_hw] */


// ==========================================================================
//  Data Logging Options
// ==========================================================================
/** Start [logging_options] */
// The name of this program file - this is used only for console printouts at
// start-up
const char* sketchName = "NGWOS_AWS_LORA.ino";
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char* LoggerID = "YOUR_LORA_THING_NAME";
// How frequently (in minutes) to log data
const int8_t loggingInterval = 5;
// The number of 1-minute intervals to take before moving to the set logging
// interval
const int8_t initialShortIntervals = 5;
// Your logger's timezone.
const int8_t timeZone = -5;  // Eastern Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!

const uint32_t serialBaud    = 921600;  // Baud rate for debugging
const int8_t   greenLED      = 8;       // Pin for the green LED
const int8_t   redLED        = 9;       // Pin for the red LED
const int8_t   buttonPin     = 21;  // Pin for debugging mode (ie, button pin)
uint8_t        buttonPinMode = INPUT_PULLDOWN;  // mode for debugging pin
const int8_t   wakePin      = 38;  // MCU interrupt/alarm pin to wake from sleep
uint8_t        wakePinMode  = INPUT_PULLUP;  // mode for wake pin
const int8_t   sdCardPwrPin = -1;            // MCU SD card power pin
const int8_t   sdCardSSPin  = 29;  // SD card chip select/slave select pin
const int8_t   flashSSPin   = 20;  // onboard flash chip select/slave select pin
const int8_t   sensorPowerPin = 22;  // MCU pin controlling main sensor power
const int8_t relayPowerPin = 56;  // MCU pin controlling an optional power relay
const int8_t sdi12DataPin  = 3;
/** End [logging_options] */


// ==========================================================================
//  The Logger Object[s]
// ==========================================================================
/** Start [loggers] */
// Create a new logger instance
// NOTE: We haven't set the pins or variable array here! We will need to call
// setVariableArray and the various pin assignment functions in the setup!
Logger dataLogger;
/** End [loggers] */


// ==========================================================================
// LoRa Modem Options
// ==========================================================================

// Network connection information

// Your OTAA connection credentials, if applicable
// The App EUI (also called the Join EUI or the Network ID)
// This must be exactly 16 hex characters (8 bytes = 64 bits)
const char appEui[] = "YourAppEUI";
// The App Key (also called the network key)
// This must be exactly 32 hex characters (16 bytes = 128 bits)
const char appKey[] = "YourAppKey";

const uint32_t modemBaud = 115200;  // Communication speed of the modem

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
const int8_t modemVccPin     = 18;  // MCU pin controlling modem power
const int8_t modemStatusPin  = 19;  // MCU pin used to read modem status
const int8_t modemSleepRqPin = 23;  // MCU pin for modem sleep/wake request
const int8_t modemLEDPin = redLED;  // MCU pin connected an LED to show modem
                                    // status

// the pin on the LoRa module that will listen for pin wakes
const int8_t lora_wake_pin = 8;  // NDTR_SLEEPRQ_DI8 (Default)
// The LoRa module's wake pin's pullup mode (0=NOPULL, 1=PULLUP, 2=PULLDOWN)
const int8_t lora_wake_pullup = 1;
// The LoRa module's wake trigger mode (ie, 0=ANY, 1=RISE, 2=FALL)
const int8_t lora_wake_edge = 1;

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialBee, LORA_AT_DEBUG);
LoRa_AT        loraAT(debugger);
#else
LoRa_AT loraAT(SerialBee);
#endif

LoRaStream loraStream(loraAT);

loraModemAWS loraModem(modemVccPin, modemSleepRqPin, modemStatusPin,
                       lora_wake_pin, lora_wake_pullup, lora_wake_edge);


// ==========================================================================
//  Using the Processor as a Sensor
// ==========================================================================
/** Start [processor_stats] */
#include <sensors/ProcessorStats.h>

// Create the main processor chip "sensor" - for general metadata
const char*    mcuBoardVersion = "v0.1";
ProcessorStats mcuBoard(mcuBoardVersion, 5);

// Create sample number, battery voltage, free RAM, and reset cause variable
// pointers for the processor
Variable* mcuBoardBatt   = new ProcessorStats_Battery(&mcuBoard);
Variable* mcuBoardSampNo = new ProcessorStats_SampleNumber(&mcuBoard);
// Variable* mcuBoardReset  = new ProcessorStats_ResetCode(&mcuBoard);
/** End [processor_stats] */


// ==========================================================================
//  Everlight ALS-PT19 Ambient Light Sensor
// ==========================================================================
/** Start [everlight_alspt19] */
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

// For an EnviroDIY Mayfly, you can use the abbreviated version
// EverlightALSPT19 alsPt19(alsNumberReadings);

// Create voltage, current, and illuminance variable pointers for the ALS-PT19
Variable* alsPt19Volt    = new EverlightALSPT19_Voltage(&alsPt19);
Variable* alsPt19Current = new EverlightALSPT19_Current(&alsPt19);
Variable* alsPt19Lux     = new EverlightALSPT19_Illuminance(&alsPt19);
/** End [everlight_alspt19] */


// ==========================================================================
//  Geolux HydroCam camera
// ==========================================================================
/** Start [geolux_hydro_cam] */
#include <sensors/GeoluxHydroCam.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const int8_t cameraPower        = relayPowerPin;   // Power pin
const int8_t cameraAdapterPower = sensorPowerPin;  // RS232 adapter power pin
// const char*  imageResolution    = "640x480";
const char* imageResolution = "1280x960";
const char* filePrefix      = LoggerID;
bool        alwaysAutoFocus = false;

// Create a GeoluxHydroCam sensor object
GeoluxHydroCam hydrocam(cameraSerial, cameraPower, dataLogger,
                        cameraAdapterPower, imageResolution, filePrefix,
                        alwaysAutoFocus);

// Create image size and byte error variables for the Geolux HydroCam
Variable* hydrocamImageSize = new GeoluxHydroCam_ImageSize(&hydrocam);
// Variable* hydrocamByteError = new GeoluxHydroCam_ByteError(&hydrocam);
/** End [geolux_hydro_cam] */


// ==========================================================================
//  Sensirion SHT4X Digital Humidity and Temperature Sensor
// ==========================================================================
/** Start [sensirion_sht4x] */
#include <sensors/SensirionSHT4x.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const int8_t SHT4xPower     = sensorPowerPin;  // Power pin
const bool   SHT4xUseHeater = true;

// Create an Sensirion SHT4X sensor object
SensirionSHT4x sht4x(SHT4xPower, SHT4xUseHeater);

// Create humidity and temperature variable pointers for the SHT4X
Variable* sht4xHumid = new SensirionSHT4x_Humidity(&sht4x);
Variable* sht4xTemp  = new SensirionSHT4x_Temp(&sht4x);
/** End [sensirion_sht4x] */


// ==========================================================================
//  VEGA PULS 21 Radar Sensor
// ==========================================================================
/** Start [vega_puls21] */
#include <sensors/VegaPuls21.h>

// NOTE: Use -1 for any pins that don't apply or aren't being used.
const char* VegaPulsSDI12address = "0";  // The SDI-12 Address of the VegaPuls21
const int8_t VegaPulsPower       = sensorPowerPin;  // Power pin
const int8_t VegaPulsData        = sdi12DataPin;    // The SDI-12 data pin
// NOTE:  you should NOT take more than one readings.  THe sensor already takes
// and averages 8 by default.

// Create a Vega Puls21 sensor object
VegaPuls21 VegaPuls(*VegaPulsSDI12address, VegaPulsPower, VegaPulsData);

// Create stage, distance, temperature, reliability, and error variable pointers
// for the VegaPuls21
Variable* VegaPulsStage       = new VegaPuls21_Stage(&VegaPuls);
Variable* VegaPulsDistance    = new VegaPuls21_Distance(&VegaPuls);
Variable* VegaPulsTemp        = new VegaPuls21_Temp(&VegaPuls);
Variable* VegaPulsReliability = new VegaPuls21_Reliability(&VegaPuls);
Variable* VegaPulsError       = new VegaPuls21_ErrorCode(&VegaPuls);
/** End [vega_puls21] */


// ==========================================================================
//  Calculated Variables
// ==========================================================================
/** Start [calculated_variables] */

//  LoRa RSSI as a Calculated Variable

// Properties of the calculated variable for the RSSI
// The number of digits after the decimal place
const uint8_t modemRSSIResolution = 0;
// This must be a value from http://vocabulary.odm2.org/variablename/
const char* modemRSSIName = "RSSI";
// This must be a value from http://vocabulary.odm2.org/units/
const char* modemRSSIUnit = "RSSI";
// A short code for the variable
const char* modemRSSICode = "loraRSSI";
float       getLoRaSignalQuality() {
    return static_cast<float>(loraAT.getSignalQuality());
}
// Finally, Create a calculated variable and return a variable pointer to it
Variable* modemRSSI = new Variable(getLoRaSignalQuality, modemRSSIResolution,
                                   modemRSSIName, modemRSSIUnit, modemRSSICode);

//  Extra Battery as an Analog Input via Calculated Variable


// helper function to read analog voltage
float getAnalogBatteryVoltage(int8_t _batteryPin, float _batteryMultiplier,
                              float _operatingVoltage = 3.3) {
    float sensorValue_battery = -9999;
    if (_batteryPin >= 0 && _batteryMultiplier > 0) {
        // Get the battery voltage
        // PRINTOUT(F("  Getting battery voltage from pin"), _batteryPin);
        pinMode(_batteryPin, INPUT);
        analogRead(_batteryPin);  // priming reading
        // The return value from analogRead() is IN BITS NOT IN VOLTS!!
        analogRead(_batteryPin);  // another priming reading
        float rawBattery = analogRead(_batteryPin);
        // PRINTOUT(F("  Raw battery pin reading in bits:"), rawBattery);
        // convert bits to volts
        sensorValue_battery =
            (_operatingVoltage / static_cast<float>(PROCESSOR_ADC_MAX)) *
            _batteryMultiplier * rawBattery;
        // PRINTOUT(F("  Battery in Volts:"), sensorValue_battery);
    } else {
        // PRINTOUT(F("  No battery pin specified!"));
    }
    return sensorValue_battery;
}


// Properties of the calculated variable for the extra battery
// The number of digits after the decimal place
const uint8_t extraBatteryResolution = 3;
// This must be a value from http://vocabulary.odm2.org/variablename/
const char* extraBatteryName = "batteryVoltage";
// This must be a value from http://vocabulary.odm2.org/units/
const char* extraBatteryUnit = "volt";
// A short code for the variable
const char* extraBatteryCode = "12VBattery";

float readExtraBattery() {
    pinMode(relayPowerPin, OUTPUT);
    digitalWrite(relayPowerPin, HIGH);  // turn on the relay power
    float  _batteryMultiplier = 5.88;
    float  _operatingVoltage  = 3.3;
    int8_t _batteryPin        = A0;
    // PRINTOUT(F("Reading 12V battery:"));
    digitalWrite(relayPowerPin, LOW);  // turn off the relay power
    return getAnalogBatteryVoltage(_batteryPin, _batteryMultiplier,
                                   _operatingVoltage);
}

// Finally, Create a calculated variable and return a variable pointer to it
Variable* extraBatteryVar =
    new Variable(readExtraBattery, extraBatteryResolution, extraBatteryName,
                 extraBatteryUnit, extraBatteryCode);
/** End [calculated_variables] */

// ==========================================================================
//  Creating the Variable Array and Filling with Variable Objects
// ==========================================================================
/** Start [variable_array] */
Variable* variableList[] = {
    alsPt19Volt,    alsPt19Current,      alsPt19Lux,      hydrocamImageSize,
    sht4xHumid,     sht4xTemp,           VegaPulsStage,   VegaPulsDistance,
    VegaPulsTemp,   VegaPulsReliability, VegaPulsError,   modemRSSI,
    mcuBoardSampNo, mcuBoardBatt,        extraBatteryVar,
};
// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);
// Create the VariableArray object
VariableArray varArray(variableCount, variableList);
/** End [variable_array] */


// ==========================================================================
// Cayenne Low Power Protocol setup
// ==========================================================================

// Initialize a buffer for the Cayenne LPP message
CayenneLPP lpp(128);

// Initialize a buffer for decoding Cayenne LPP messages
#if ARDUINOJSON_VERSION_MAJOR < 7
DynamicJsonDocument jsonBuffer(1024);  // ArduinoJson 6
#else
JsonDocument jsonBuffer;  // ArduinoJson 7
#endif

// From ArduinoJson Website:
// https://arduinojson.org/v7/how-to/upgrade-from-v6/
// ArduinoJson 7 is significantly bigger ⚠️
// ArduinoJson 6 had a strong focus on code size because 8-bit micro-controllers
// were still dominant at the time. ArduinoJson 7 loosened the code size
// constraint to focus on ease of use. As a result, version 7 is significantly
// bigger than version 6.
// If your program targets 8-bit micro-controllers, I recommend keeping
// version 6.

// ==========================================================================
//  Working Functions
// ==========================================================================
/** Start [working_functions] */
// Starts up all of the serial ports
void startSerials() {
    // Start the primary serial connection
    Serial.begin(serialBaud);
#if defined(MS_2ND_OUTPUT)
    // secondary serial connection
    MS_2ND_OUTPUT.begin(serialBaud);
#endif
#if defined(modemSerial)
    // Start the serial connection with the modem
    modemSerial.begin(modemBaud);
#endif
#if defined(cameraSerial) && defined(GEOLUX_CAMERA_RS232_BAUD)
    // Start the stream for the camera; it will always be at 115200 baud
    cameraSerial.begin(GEOLUX_CAMERA_RS232_BAUD);
#endif
}

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

// Helper function to read the main battery voltage
float getPrimaryBatteryVoltage() {
    int8_t _batteryPin        = A9;  // aka 75
    float  _batteryMultiplier = 4.7;
    float  _operatingVoltage  = 3.3;
    // PRINTOUT(F("Reading LiPo battery:"));
    return getAnalogBatteryVoltage(_batteryPin, _batteryMultiplier,
                                   _operatingVoltage);
}

// Just a function to pretty-print the modbus hex frames
// This is purely for debugging
void printFrameHex(byte modbusFrame[], int frameLength) {
    for (int i = 0; i < frameLength; i++) {
        if (modbusFrame[i] < 16) MS_SERIAL_OUTPUT.print("0");
        MS_SERIAL_OUTPUT.print(modbusFrame[i], HEX);
    }
    MS_SERIAL_OUTPUT.println();
}
/** End [working_functions] */


// ==========================================================================
//  Arduino Setup Function
// ==========================================================================
void setup() {
    /** Start [setup_flashing_led] */
    // Blink the LEDs to show the board is on and starting up
    greenRedFlash(3, 35);
    /** End [setup_flashing_led] */

    // Hold the green LED on so we know the board is on
    digitalWrite(greenLED, HIGH);

/** Start [setup_wait] */
// Wait for USB connection to be established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this adds an
// unnecessary startup delay
#if defined(SERIAL_PORT_USBVIRTUAL)
    while (!SERIAL_PORT_USBVIRTUAL && (millis() < 10000L));
#endif
    /** End [setup_wait] */

    /** Start [setup_serial_begins] */
    // Start all of the various serial connections
    startSerials();
    // Flash again to show serial has started
    greenRedFlash(5, 50);
    // Hold the green LED on so we know the board is on
    digitalWrite(greenLED, HIGH);
    /** End [setup_serial_begins] */


    /** Start [setup_prints] */
    // Print a start-up note to the first serial port
    PRINTOUT("\n\n\n=============================");
    PRINTOUT("=============================");
    PRINTOUT("=============================");
    PRINTOUT(F("\n\nNow running"), sketchName, F("on Logger"), LoggerID, '\n');

    PRINTOUT(F("Using ModularSensors Library version"),
             MODULAR_SENSORS_VERSION);
    PRINTOUT(F("LoRa AT Library version"), LORA_AT_VERSION, '\n');
    PRINTOUT(F("Processor:"), mcuBoard.getSensorLocation());
    PRINTOUT(F("The most recent reset cause was"), mcuBoard.getLastResetCode(),
             '(', mcuBoard.getLastResetCause(), ")\n");
    /** End [setup_prints] */

    // Start the SPI library
    PRINTOUT(F("Starting SPI"));
    SPI.begin();

    PRINTOUT(F("Setting onboard flash pin modes"));
    pinMode(flashSSPin,
            OUTPUT);  // for proper operation of the onboard flash memory

    PRINTOUT(F("Starting I2C (Wire)"));
    Wire.begin();

    /** Start [setup_logger] */

    // set the logger ID
    PRINTOUT(F("Setting logger id to"), LoggerID);
    dataLogger.setLoggerID(LoggerID);
    // set the logging interval
    PRINTOUT(F("Setting logging interval to"), loggingInterval, F("minutes"));
    dataLogger.setLoggingInterval(loggingInterval);
    PRINTOUT(F("Setting number of initial 1 minute intervals to"),
             initialShortIntervals);
    dataLogger.setinitialShortIntervals(initialShortIntervals);
    // Attach the variable array to the logger
    PRINTOUT(F("Attaching the variable array"));
    dataLogger.setVariableArray(&varArray);
    // set logger pins
    PRINTOUT(F("Setting logger pins"));
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED, wakePinMode, buttonPinMode);
    // Don't allow pins to "tri-state" when sleeping
    Logger::disablePinTristate(true);

    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    PRINTOUT(F("Setting logger time zone"));
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    PRINTOUT(F("Setting RTC time zone"));
    loggerClock::setRTCOffset(0);

    PRINTOUT(F("Setting analog read resolution for onboard ADC to 12 bit"));
    analogReadResolution(12);

    // Begin the logger
    PRINTOUT(F("Beginning the logger"));
    dataLogger.begin();
    /** End [setup_logger] */

    /** Start [setup_sensors] */
    // Note:  Please change these battery voltages to match your battery
    // Set up the sensors, except at lowest battery level
    if (getPrimaryBatteryVoltage() > 3.4) {
        PRINTOUT(F("Setting up sensors..."));
        varArray.sensorsPowerUp();  // only needed if you have sensors that need
                                    // power for setups
        varArray.setupSensors();
        varArray.sensorsPowerDown();  // only needed if you have sensors that
                                      // need power for setups
    }
    /** End [setup_sensors] */

    /** Start [setup_lora] */
    // Power on, set up, and connect the LoRa modem
    PRINTOUT(F("Waking the LoRa module..."));
    loraModem.modemPowerOn();
    PRINTOUT(F("Setting up the LoRa module..."));
    loraModem.setupModemAWS(loraAT);
    PRINTOUT(F("Attempting to connect to LoRa network..."));
    loraModem.modemConnect(loraAT, appEui, appKey);
    /** End [setup_lora] */

    /** Start [setup_clock] */
    // Sync the clock if it isn't valid or we have battery to spare
    if (getPrimaryBatteryVoltage() > 3.55 || !dataLogger.isRTCSane()) {
        // get the epoch time from the LoRa network
        uint32_t epochTime = loraModem.modemGetTime(loraAT);

        // set the RTC time from the epoch
        if (epochTime != 0) {
            MS_SERIAL_OUTPUT.print(F("Setting RTC epoch to "));
            MS_SERIAL_OUTPUT.println(epochTime);
            loggerClock::setRTClock(epochTime, UNIX, epochStart::unix_epoch);
        }
    }
    /** End [setup_clock] */

    // put the modem to sleep
    PRINTOUT(F("Putting the LoRa module to sleep..."));
    loraModem.modemSleep(loraAT);

    // Confirm the date and time using the ISO 8601 timestamp
    uint32_t currentEpoch = dataLogger.getNowLocalEpoch();
    MS_SERIAL_OUTPUT.print(F("Current RTC timestamp:"));
    MS_SERIAL_OUTPUT.println(dataLogger.formatDateTime_ISO8601(currentEpoch));

    /** Start [setup_file] */
    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct.
    // Writing to the SD card can be power intensive, so if we're skipping the
    // sensor setup we'll skip this too.
    if (getPrimaryBatteryVoltage() > 3.4) {
        PRINTOUT(F("Setting up file on SD card"));
        dataLogger.turnOnSDcard(true);
        // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(true);
        // true = wait for internal housekeeping after write
    }
    /** End [setup_file] */

    /** Start [setup_sleep] */
    // Turn off the green LED
    digitalWrite(greenLED, LOW);

    // Call the processor sleep
    PRINTOUT(F("Putting processor to sleep\n"));
    dataLogger.systemSleep();
    /** End [setup_sleep] */
}


// ==========================================================================
//  Arduino Loop Function
// ==========================================================================
/** Start [complex_loop] */
void loop() {
    // Reset the watchdog
    extendedWatchDog::resetWatchDog();

    // Assuming we were woken up by the clock, check if the current time is an
    // even interval of the logging interval
    // We're only doing anything at all if the battery is above 3.4V
    if (dataLogger.checkInterval() && getPrimaryBatteryVoltage() > 3.4) {
        // Flag to notify that we're in already awake and logging a point
        Logger::isLoggingNow = true;
        startSerials();
        extendedWatchDog::resetWatchDog();

        // Print a line to show new reading
        PRINTOUT(F("------------------------------------------"));
        // Turn on the LED to show we're taking a reading
        dataLogger.alertOn();
        // Power up the SD Card, but skip any waits after power up
        dataLogger.turnOnSDcard(false);
        extendedWatchDog::resetWatchDog();

        // wake up the modem
        bool successful_wake = loraModem.modemWake(loraAT);
        extendedWatchDog::resetWatchDog();
        if (successful_wake) {
            PRINTOUT(F("Attempting to connect to LoRa network..."));
            successful_wake &= loraModem.modemConnect(loraAT, appEui, appKey);
        }

        // Confirm the date and time using the ISO 8601 timestamp
        uint32_t currentEpoch = dataLogger.getNowLocalEpoch();
        MS_SERIAL_OUTPUT.print(F("Current RTC timestamp:"));
        MS_SERIAL_OUTPUT.println(
            dataLogger.formatDateTime_ISO8601(currentEpoch));
        extendedWatchDog::resetWatchDog();

        // Do a complete update on the variable array.
        // This this includes powering all of the sensors, getting updated
        // values, and turing them back off.
        // NOTE:  The wake function for each sensor should force sensor setup
        // to run if the sensor was not previously set up.
        varArray.completeUpdate();
        extendedWatchDog::resetWatchDog();

        // Create a csv data record and save it to the log file
        dataLogger.logToSD();
        extendedWatchDog::resetWatchDog();

        // set lpp buffer pointer back to the head of the buffer
        lpp.reset();
        // Create a JsonArray object for decoding/debugging
        JsonObject root = jsonBuffer.to<JsonObject>();

        // Add the time to the Cayenne LPP Buffer
        lpp.addUnixTime(1, Logger::markedUTCUnixTime);
        extendedWatchDog::resetWatchDog();

        // Add the RSSI to the Cayenne LPP Buffer
        // NOTE: There is no reason to add the RSSI to the LPP buffer
        // the gateway can automatically add the RSSI to the message metadata
        // when it forwards the message to IoT Core.
        // But we'll print the RSSI to the serial console.
        MS_SERIAL_OUTPUT.print(F("Signal quality: "));
        MS_SERIAL_OUTPUT.println(modemRSSI->getValueString(false));

        // lpp.addGenericSensor(2, modemRSSI->getValue(false));
        // extendedWatchDog::resetWatchDog();

        // Add the SHT40 data to the Cayenne LPP buffer
        MS_SERIAL_OUTPUT.print(F("Temperature: "));
        MS_SERIAL_OUTPUT.println(sht4xTemp->getValueString(false));
        MS_SERIAL_OUTPUT.print(F("Humidity: "));
        MS_SERIAL_OUTPUT.println(sht4xHumid->getValueString(false));
        // Add the temperature and humidity to the Cayenne LPP Buffer
        lpp.addTemperature(3, sht4xTemp->getValue(false));
        lpp.addRelativeHumidity(4, sht4xHumid->getValue(false));
        extendedWatchDog::resetWatchDog();

        // Add VegaPuls21 data to the Cayenne LPP buffer
        MS_SERIAL_OUTPUT.print(F("Stage: "));
        MS_SERIAL_OUTPUT.println(VegaPulsStage->getValueString(false));
        MS_SERIAL_OUTPUT.print(F("Distance: "));
        MS_SERIAL_OUTPUT.println(VegaPulsDistance->getValueString(false));
        MS_SERIAL_OUTPUT.print(F("Temperature: "));
        MS_SERIAL_OUTPUT.println(VegaPulsTemp->getValueString(false));
        MS_SERIAL_OUTPUT.print(F("Reliability: "));
        MS_SERIAL_OUTPUT.println(VegaPulsReliability->getValueString(false));
        MS_SERIAL_OUTPUT.print(F("Error Code: "));
        MS_SERIAL_OUTPUT.println(VegaPulsError->getValueString(false));
        // stage in m (resolution 1mm)
        lpp.addDistance(5, VegaPulsStage->getValue(false));
        // distance in m (resolution 1mm)
        lpp.addDistance(6, VegaPulsDistance->getValue(false));
        // temperature in °C (resolution 0.1°C)
        lpp.addTemperature(7, VegaPulsTemp->getValue(false));
        // reliability in dB (resolution 0.1db)
        // lpp.addGenericSensor(8, VegaPulsReliability->getValue(false));
        // error code
        // lpp.addGenericSensor(9, VegaPulsError->getValue(false));
        extendedWatchDog::resetWatchDog();

        // Add the ALS-PT19 data to the Cayenne LPP buffer
        MS_SERIAL_OUTPUT.print(F("Lux: "));
        MS_SERIAL_OUTPUT.println(alsPt19Lux->getValueString(false));
        // Add to LPP buffer
        lpp.addLuminosity(10, alsPt19Lux->getValue(false));
        extendedWatchDog::resetWatchDog();

        // Add the analog battery voltage monitor to the Cayenne LPP buffer
        MS_SERIAL_OUTPUT.print(F("Analog 3.3V Batt Voltage: "));
        MS_SERIAL_OUTPUT.print(mcuBoardBatt->getValueString(false));
        MS_SERIAL_OUTPUT.println(" V");
        // Add to LPP buffer
        lpp.addVoltage(14, mcuBoardBatt->getValue(false));
        extendedWatchDog::resetWatchDog();

        // Add the secondary battery voltage monitor to the Cayenne LPP buffer
        MS_SERIAL_OUTPUT.print(F("Analog 12V Batt Voltage: "));
        MS_SERIAL_OUTPUT.print(extraBatteryVar->getValueString(false));
        MS_SERIAL_OUTPUT.println(" V");
        // Add to LPP buffer
        lpp.addVoltage(15, extraBatteryVar->getValue(false));
        extendedWatchDog::resetWatchDog();

        // decode and print the Cayenne LPP buffer we just created
        MS_SERIAL_OUTPUT.println("Cayenne LPP Buffer:");
        printFrameHex(lpp.getBuffer(), lpp.getSize());
        MS_SERIAL_OUTPUT.println("\nDecoded Buffer:");
        lpp.decodeTTN(lpp.getBuffer(), lpp.getSize(), root);
        serializeJsonPretty(root, MS_SERIAL_OUTPUT);
        MS_SERIAL_OUTPUT.println();
        extendedWatchDog::resetWatchDog();


        // Send out the Cayenne LPP buffer
        if (successful_wake) {
            if (loraStream.write(lpp.getBuffer(), lpp.getSize()) ==
                lpp.getSize()) {
                MS_SERIAL_OUTPUT.println(F("  Successfully sent data"));
                extendedWatchDog::resetWatchDog();
                if ((Logger::markedLocalUnixTime != 0 &&
                     Logger::markedLocalUnixTime % 86400 == 43200) ||
                    !dataLogger.isRTCSane()) {
                    MS_SERIAL_OUTPUT.println(
                        F("Running a daily clock sync..."));
                    // get the epoch time from the LoRa network
                    uint32_t epochTime = loraModem.modemGetTime(loraAT);
                    // set the RTC time from the epoch
                    if (epochTime != 0) {
                        MS_SERIAL_OUTPUT.print(F("Setting RTC epoch to "));
                        MS_SERIAL_OUTPUT.println(epochTime);
                        loggerClock::setRTClock(epochTime, UNIX,
                                                epochStart::unix_epoch);
                    }
                    extendedWatchDog::resetWatchDog();
                }
            } else {
                MS_SERIAL_OUTPUT.println(F("--Failed to send data!"));
                bool res = loraAT.isNetworkConnected();
                MS_SERIAL_OUTPUT.print(F("Network status: "));
                MS_SERIAL_OUTPUT.println(res ? "connected" : "not connected");
                extendedWatchDog::resetWatchDog();
            }
        } else {
            MS_SERIAL_OUTPUT.println(
                F("--Failed to send data! Can not communicate with modem!"));
        }

        // dump anything in the LoRa buffer
        while (loraStream.available()) { Serial.write(loraStream.read()); }

        // put the modem to sleep
        loraModem.modemSleep(loraAT);

        // Dump anything in the LoRa buffer again, and then flush it
        while (loraStream.available()) { Serial.write(loraStream.read()); }
        // It is crucial that we flush the stream or we will not be able to
        // sleep
        loraStream.flush();

        // Turn off the LED
        dataLogger.alertOff();
        // Print a line to show reading ended
        MS_SERIAL_OUTPUT.println(
            F("------------------------------------------\n"));

        // Unset flag
        Logger::isLoggingNow = false;
    }

    // Check if it was instead the testing interrupt that woke us up
    if (Logger::startTesting) { dataLogger.benchTestingMode(); }

    // Flush the streams, just in case - data left in the streams will prevent
    // sleep
    Serial.flush();
    modemSerial.flush();
    cameraSerial.flush();
#if defined(MS_2ND_OUTPUT)
    MS_2ND_OUTPUT.flush();
#endif
    // Call the processor sleep
    dataLogger.systemSleep();
}
/** End [complex_loop] */
