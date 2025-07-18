/** =========================================================================
 * @example{lineno} USGS_AWS.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief A testing program for the USGS with AWS.
 * ======================================================================= */

// Defines to help me print strings
// this converts to string
#define STR_(X) #X
// this makes sure the argument is expanded before converting to string
#define STR(X) STR_(X)

// ==========================================================================
//  Defines for TinyGSM
// ==========================================================================
/** Start [defines] */
#ifndef TINY_GSM_RX_BUFFER
#define TINY_GSM_RX_BUFFER 64
#endif
#ifndef TINY_GSM_YIELD_MS
#define TINY_GSM_YIELD_MS 2
#endif
/** End [defines] */


// ==========================================================================
//  Include the libraries required for any data logger
// ==========================================================================
/** Start [includes] */
// The Arduino library is needed for every Arduino program.
#include <Arduino.h>

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
#define cameraSerial Serial1
/** End [assign_ports_hw] */


// ==========================================================================
//  Data Logging Options
// ==========================================================================
/** Start [logging_options] */
static const char AWS_IOT_ENDPOINT[] TINY_GSM_PROGMEM =
    "YOUR_ENDPOINT-ats.iot.us-west-2.amazonaws.com";
#define THING_NAME "YOUR_THING_NAME"

// The name of this program file - this is used only for console printouts at
// start-up
const char* sketchName = "NGWOS_AWS_MQTT.ino";
// Logger ID, also becomes the prefix for the name of the data file on SD card
// This is also used as the Thing Name, MQTT Client ID, and topic for AWS IOT
// Core
const char* LoggerID = THING_NAME;
// Sampling feature UUID
// This is used as the UUID for the sampling feature on Monitor My Watershed and
// the sub-topic for AWS IOT Core
const char* samplingFeature = "YOUR_SAMPLING_FEATURE_ID";
// How frequently (in minutes) to log data
const uint8_t loggingInterval = 5;
// Your logger's timezone.
const int8_t timeZone = -5;  // Eastern Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!

const int32_t serialBaud    = 921600;  // Baud rate for debugging
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
Logger dataLogger(LoggerID, samplingFeature, loggingInterval);
/** End [loggers] */


// ==========================================================================
//  Wifi/Cellular Modem Options
//    NOTE:  DON'T USE MORE THAN ONE MODEM OBJECT!
//           Delete the sections you are not using!
// ==========================================================================

// Network connection information
// APN for cellular connection
#define CELLULAR_APN "hologram"


#define MODEM_BAUD 921600
// ==========================================================================
/** Start [sim_com_sim7080] */
// For almost anything based on the SIMCom SIM7080G
#include <modems/SIMComSIM7080.h>

// NOTE: Extra hardware and software serial ports are created in the "Settings
// for Additional Serial Ports" section
const int32_t modemBaud = MODEM_BAUD;  // Communication speed of the modem

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
/** End [sim_com_sim7080] */
// ==========================================================================

/** Start [modem_variables] */
// Create RSSI and signal strength variable pointers for the modem
Variable* modemSignalPct = new Modem_SignalPercent(&modem, "", "signalPercent");
/** End [modem_variables] */


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
const int8_t cameraPower        = relayPowerPin;  // Power pin
const int8_t cameraAdapterPower = relayPowerPin;  // RS232 adapter power pin
// const char*  imageResolution    = "640x480";
// const char* imageResolution = "1600x1200";
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
const char* VegaPulsSDI12address = "4";  // The SDI-12 Address of the VegaPuls21
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
//  Calculated Variable[s]
// ==========================================================================
/** Start [calculated_variables] */
// Create the function to give your calculated result.
// The function should take no input (void) and return a float.
// You can use any named variable pointers to access values by way of
// variable->getValue()

float readExtraBattery() {
    float  _batteryMultiplier  = 5.88;
    float  _operatingVoltage   = 3.3;
    int8_t _batteryPin         = A0;
    float  sensorValue_battery = -9999;
    if (_batteryPin >= 0 && _batteryMultiplier > 0) {
        // Get the battery voltage
        MS_DBG(F("Getting battery voltage from pin"), _batteryPin);
        pinMode(_batteryPin, INPUT);
        analogRead(_batteryPin);  // priming reading
        // The return value from analogRead() is IN BITS NOT IN VOLTS!!
        analogRead(_batteryPin);  // another priming reading
        float rawBattery = analogRead(_batteryPin);
        MS_DBG(F("Raw battery pin reading in bits:"), rawBattery);
        // convert bits to volts
        sensorValue_battery =
            (_operatingVoltage / static_cast<float>(PROCESSOR_ADC_MAX)) *
            _batteryMultiplier * rawBattery;
        MS_DBG(F("Battery in Volts:"), sensorValue_battery);
    } else {
        MS_DBG(F("No battery pin specified!"));
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
// The (optional) universally unique identifier
const char* extraBatteryUUID = "";

// Finally, Create a calculated variable and return a variable pointer to it
Variable* extraBattery = new Variable(readExtraBattery, extraBatteryResolution,
                                      extraBatteryName, extraBatteryUnit,
                                      extraBatteryCode, extraBatteryUUID);
/** End [calculated_variables] */

//^^ BUILD_TEST_PRE_NAMED_VARS
/** Start [variables_pre_named] */
// Version 3: Fill array with already created and named variable pointers
Variable* variableList[] = {
    VegaPulsStage,       hydrocamImageSize, mcuBoardBatt,     extraBattery,
    modemSignalPct,      sht4xTemp,         sht4xHumid,       alsPt19Lux,
    alsPt19Current,      alsPt19Volt,       VegaPulsDistance, VegaPulsTemp,
    VegaPulsReliability, VegaPulsError,     mcuBoardSampNo,
};
// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);
// Create the VariableArray object
VariableArray varArray(variableCount, variableList);
/** End [variables_pre_named] */

// ==========================================================================
//  AWS S3 Pre-signed URL Publisher
// ==========================================================================
/** Start [s3_presigned_publisher] */
// The name of your certificate authority certificate file
const char* caCertName = "AmazonRootCA1.pem";

// Expand the expected S3 publish topic into a buffer
String s3URLPubTopic = "$aws/rules/GetUploadURL/" + String(LoggerID);
// Expand the expected S3 subscribe topic into a buffer
String s3URLSubTopic = String(LoggerID) + "/upload_url";
// A function for the IoT core publisher to call to get the message content for
// a new URL request
String s3URLMsgGetter() {
    return String("{\"file\": \"") + hydrocam.getLastSavedImageName() +
        String("\"}");
}

// Create a data publisher for AWS IoT Core
#include <publishers/S3PresignedPublisher.h>
S3PresignedPublisher s3pub;
/** End [s3_presigned_publisher] */


// ==========================================================================
//  AWS IoT Core MQTT Publisher
// ==========================================================================
/** Start [aws_io_t_publisher] */
// The endpoint for your AWS IoT instance
const char* awsIoTEndpoint = AWS_IOT_ENDPOINT;
// Sampling feature UUID, this will be the sub-topic for your data
// NOTE: Because we already set the sampling feature with the logger
// constructor, don't do it here.
// const char* samplingFeature = "";
// The name of your client certificate file
const char* clientCertName = THING_NAME "-certificate.pem.crt";
// The name of your client private key file
const char* clientKeyName = THING_NAME "-private-key.pem.key";

// Create a data publisher for AWS IoT Core
#include <publishers/AWS_IoT_Publisher.h>
AWS_IoT_Publisher awsIoTPub(dataLogger, awsIoTEndpoint, caCertName,
                            clientCertName, clientKeyName);

// Callback function
void IoTCallback(char* topic, byte* payload, unsigned int length) {
    // the topic is a char and guaranteed to be null-terminated, so we can
    // directly convert to a String
    if (String(topic) == s3URLSubTopic) {
        PRINTOUT(F("Received data on pre-signed URL topic from AWS IoT Core!"));
        // PRINTOUT(F("Got message of length"), length, F("on topic"), topic);
        // Allocate the correct amount of memory for the payload copy
        // We CANNOT directly convert it to a string because it's not guaranteed
        // to be null-terminated
        char* rx_url = (char*)malloc(length + 1);
        // Copy the payload to the new buffer
        memcpy(rx_url, payload, length);
        // Null terminate the string
        memset(rx_url + length, '\0', 1);
        // PRINTOUT(F("Setting S3 URL to:"), rx_url);
        s3pub.setPreSignedURL(String(rx_url));
        // Free the memory now that the URL has been copied into a new String
        free(rx_url);
        // let the publisher know we got what we expected and it can stop
        // waiting
        awsIoTPub.closeConnection();
    }
}
/** End [aws_io_t_publisher] */


// ==========================================================================
//  Working Functions
// ==========================================================================
/** Start [working_functions] */
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
/** End [working_functions] */


// ==========================================================================
//  Arduino Setup Function
// ==========================================================================
void setup() {
    /** Start [setup_flashing_led] */
    // Blink the LEDs to show the board is on and starting up
    greenRedFlash(3, 35);
    /** End [setup_flashing_led] */

/** Start [setup_wait] */
// Wait for USB connection to be established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this adds an
// unnecessary startup delay
#if defined(SERIAL_PORT_USBVIRTUAL)
    while (!SERIAL_PORT_USBVIRTUAL && (millis() < 10000L)) {
        // wait
    }
#endif
    /** End [setup_wait] */

    /** Start [setup_prints] */
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
    PRINTOUT(F("\n\nNow running"), sketchName, F("on Logger"), LoggerID, '\n');

    PRINTOUT(F("Using ModularSensors Library version"),
             MODULAR_SENSORS_VERSION);
    PRINTOUT(F("TinyGSM Library version"), TINYGSM_VERSION, '\n');
    PRINTOUT(F("Processor:"), mcuBoard.getSensorLocation());
    PRINTOUT(F("The most recent reset cause was"), mcuBoard.getLastResetCode(),
             '(', mcuBoard.getLastResetCause(), ")\n");
    /** End [setup_prints] */

    /** Start [setup_serial_begins] */
    // Start the serial connection with the modem
    PRINTOUT(F("Starting modem connection on"), STR(modemSerial), F(" at"),
             modemBaud, F(" baud"));
    modemSerial.begin(modemBaud);

    // Start the stream for the camera; it will always be at 115200 baud
    PRINTOUT(F("Starting camera connection at"), 115200, F("baud"));
    cameraSerial.begin(115200);

    /** End [setup_serial_begins] */

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
    PRINTOUT(F("Setting the sampling feature UUID to"), LoggerID);
    dataLogger.setSamplingFeatureUUID(samplingFeature);
    // set the logging interval
    PRINTOUT(F("Setting logging interval to"), loggingInterval, F("minutes"));
    dataLogger.setLoggingInterval(loggingInterval);
    PRINTOUT(F("Setting number of initial 1 minute intervals to 10"));
    dataLogger.setinitialShortIntervals(15);
    // Attach the variable array to the logger
    PRINTOUT(F("Attaching the variable array"));
    dataLogger.setVariableArray(&varArray);
    // set logger pins
    PRINTOUT(F("Setting logger pins"));
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED, wakePinMode, buttonPinMode);

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

    // Set the S3 certificate authority names
    s3pub.setCACertName(caCertName);
    s3pub.attachToLogger(dataLogger);

    // Set the callback function for the AWS IoT Core MQTT connection
    awsIoTPub.setCallback(IoTCallback);
    awsIoTPub.addSubTopic(s3URLSubTopic.c_str());
    awsIoTPub.addPublishRequest(s3URLPubTopic.c_str(), s3URLMsgGetter);

    // Begin the logger
    PRINTOUT(F("Beginning the logger"));
    dataLogger.begin();
    /** End [setup_logger] */

    /** Start [setup_sensors] */
    // Note:  Please change these battery voltages to match your battery
    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4) {
        PRINTOUT(F("Setting up sensors..."));
        varArray.sensorsPowerUp();  // only needed if you have sensors that need
                                    // power for setups
        varArray.setupSensors();
        varArray.sensorsPowerDown();  // only needed if you have sensors that
                                      // need power for setups
    }
    /** End [setup_sensors] */

    /** Start [setup_sim7080] */
    modem.setModemWakeLevel(HIGH);   // ModuleFun Bee inverts the signal
    modem.setModemResetLevel(HIGH);  // ModuleFun Bee inverts the signal
    PRINTOUT(F("Waking modem and setting Cellular Carrier Options..."));
    modem.modemWake();  // NOTE:  This will also set up the modem
    if (!modem.gsmModem.testAT()) {
        PRINTOUT(F("Attempting autobauding.."));
        uint32_t foundBaud = TinyGsmAutoBaud(modemSerial);
        if (foundBaud != 0 && !(F_CPU <= 8000000L && foundBaud >= 115200) &&
            !(F_CPU <= 16000000L && foundBaud > 115200)) {
            PRINTOUT(F("Got modem response at baud of"), foundBaud,
                     F("Firing an attempt to change the baud rate to"),
                     modemBaud);
            modem.gsmModem.setBaud(
                modemBaud);  // Make sure we're *NOT* auto-bauding!
            modem.gsmModem.waitResponse();
            modemSerial.end();
            modemSerial.begin(modemBaud);
        }
    }
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

    /** Start [setup_clock] */
    // Sync the clock if it isn't valid or we have battery to spare
    if (getBatteryVoltage() > 3.55 || !loggerClock::isRTCSane()) {
        // Set up the modem, synchronize the RTC with NIST, and publish
        // configuration information to publishers that support it.
        dataLogger.makeInitialConnections();
    }
    /** End [setup_clock] */

    /** Start [setup_file] */
    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct.
    // Writing to the SD card can be power intensive, so if we're skipping the
    // sensor setup we'll skip this too.
    if (getBatteryVoltage() > 3.4) {
        PRINTOUT(F("Setting up file on SD card"));
        dataLogger.turnOnSDcard(true);
        // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(true);
        // true = wait for internal housekeeping after write
    }
    /** End [setup_file] */

    /** Start [setup_sleep] */
    // Call the processor sleep
    PRINTOUT(F("Putting processor to sleep\n"));
    dataLogger.systemSleep();
    /** End [setup_sleep] */
}


// ==========================================================================
//  Arduino Loop Function
// ==========================================================================
/** Start [simple_loop] */
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
/** End [simple_loop] */
