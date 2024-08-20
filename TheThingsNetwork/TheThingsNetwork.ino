#include <Arduino.h>

// Select your LoRa Module:
#define LORA_AT_MDOT

#define LORA_AT_YIELD_MS 2

#define SDI12_YIELD_MS 16

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define LORA_AT_DEBUG Serial

// Include the LoRa library
#include <LoRa_AT.h>
// Cayenne LPP library for compressing data for TTN
#include <CayenneLPP.h>
// RTC Library (On-board clock)
#include <SparkFun_RV8803.h>
// Adafruit SHT library (On-board humidity and temperature)
#include "Adafruit_SHT4x.h"
// Adafruit MAX1704x library (On-board battery monitor)
#include <Adafruit_MAX1704X.h>
// The SDI-12 library for the Vega Puls
#include <SDI12.h>
// Local H files with separated fxns
#include "SDI12Master.h"
#include "LoRaModemFxns.h"
#include "LoggerBase.h"


// ==========================================================================
//  Data Logging Options
// ==========================================================================
/** Start [logging_options] */
// The name of this program file
const char* sketchName = "TheThingsNetwork.ino";
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
// Stonefly 0.1
const int8_t sdCardPwrPin   = 32;  // MCU SD card power pin
const int8_t sdCardSSPin    = 29;  // SD card chip select/slave select pin
const int8_t sensorPowerPin = 22;  // MCU pin controlling main sensor power
/** End [logging_options] */


// ==========================================================================
//  LoRa Modem Options
// ==========================================================================

// Network connection information

// Your OTAA connection credentials, if applicable
// The App EUI (also called the Join EUI or the Network ID)
// This must be exactly 16 hex characters (8 bytes = 64 bits)
const char appEui[] = "YourAppEUI";
// The App Key (also called the network key)
// This must be exactly 32 hex characters (16 bytes = 128 bits)
const char appKey[] = "YourAppKey";

const int32_t modemBaud = 115200;

// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
// and-global breakout bk-7080a
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
const int8_t lora_wake_edge = 0;

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialBee, Serial);
LoRa_AT        lora_modem(debugger);
#else
LoRa_AT lora_modem(SerialBee);
#endif

LoRaStream loraStream(lora_modem);

loraModemTTN ttn_modem(modemVccPin, modemSleepRqPin, modemStatusPin, lora_wake_pin, lora_wake_pullup,
                       lora_wake_edge);


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
//  Cayenne Low Power Protocol setup
// ==========================================================================

// Initialize a buffer for the Cayenne LPP message
CayenneLPP lpp(128);

// Initialize a buffer for decoding Cayenne LPP messages
// DynamicJsonDocument jsonBuffer(1024); // ArduinoJson 6
JsonDocument jsonBuffer;  // ArduinoJson 6

// From ArduinoJson Website:
// https://arduinojson.org/v7/how-to/upgrade-from-v6/
// ArduinoJson 7 is significantly bigger ⚠️
// ArduinoJson 6 had a strong focus on code size because 8-bit microcontrollers
// were still dominant at the time. ArduinoJson 7 loosened the code size
// constraint to focus on ease of use. As a result, version 7 is significantly
// bigger than version 6.
// If your program targets 8-bit microcontrollers, I recommend keeping
// version 6.


// ==========================================================================
//  Other on-board sensors
// ==========================================================================

// Create the SHT object
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// Create the battery monitor object
Adafruit_MAX17048 maxlipo;

const int8_t alsDataPin = 74;


// ==========================================================================
//  The Logger Object[s]
// ==========================================================================
/** Start [loggers] */
// Create a new logger instance
Logger dataLogger(LoggerID, loggingInterval);
/** End [loggers] */


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
// Just a function to pretty-print the modbus hex frames
// This is purely for debugging
void printFrameHex(byte modbusFrame[], int frameLength) {
    for (int i = 0; i < frameLength; i++) {
        if (modbusFrame[i] < 16) Serial.print("0");
        Serial.print(modbusFrame[i], HEX);
    }
    Serial.println();
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
    analogRead(_batteryPin);  // priming reading
    float rawBattery = analogRead(_batteryPin);
    MS_DBG(F("Raw battery pin reading in bits:"), rawBattery);
    // convert bits to volts
    sensorValue_battery = (3.3 / 1023.) * 4.7 * rawBattery;
    MS_DBG(F("Battery in Volts:"), sensorValue_battery);
    return sensorValue_battery;
}
/** End [working_functions] */


// ==========================================================================
//  Arduino Setup Function
// ==========================================================================
void setup() {
// Wait for USB connection to be established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this
// could prevent the script from starting
#if defined SERIAL_PORT_USBVIRTUAL
    while (!SERIAL_PORT_USBVIRTUAL && (millis() < 10000L)) {}
#endif

    // Set console baud rate
    Serial.begin(serialBaud);
    delay(10);

    Wire.begin();

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
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();

    pinMode(20, OUTPUT);  // for proper operation of the onboard flash memory
                          // chip's ChipSelect

    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    Logger::setRTCTimeZone(0);
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, greenLED);

    // Begin the logger
    dataLogger.begin();

    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4) {
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
                F("Couldnt find Adafruit MAX17048?\nMake sure a battery "
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

    // Power on, set up, and connect the LoRa modem
    ttn_modem.modemPowerOn();
    ttn_modem.setupModemTTN(lora_modem);
    ttn_modem.modemConnect(lora_modem, appEui, appKey);

    // Sync the clock if it isn't valid or we have battery to spare
    if (getBatteryVoltage() > 3.55 || !dataLogger.isRTCSane()) {
        // get the epoch time from the LoRa network
        uint32_t epochTime = ttn_modem.modemGetTime(lora_modem);

        // set the RTC time from the epoch
        if (epochTime != 0) {
            Serial.print(F("Setting RTC epoch to "));
            Serial.println(epochTime);
            dataLogger.setNowUTCEpoch(epochTime);
        }
    }

    // put the modem to sleep
    ttn_modem.modemSleep(lora_modem);

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
        header += "mDOT RSSI,";
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
        header += "Encoded LPP Buffer";
        dataLogger.logToSD(header);
        dataLogger.turnOffSDcard(true);
        // true = wait for internal housekeeping after write
    }

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
        dataLogger.turnOnSDcard(false);
        dataLogger.watchDogTimer.resetWatchDog();

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

        // wake up the modem
        ttn_modem.modemWake(lora_modem);
        dataLogger.watchDogTimer.resetWatchDog();

        // set lpp buffer pointer back to the head of the buffer
        lpp.reset();
        // Create a JsonArray object for decoding/debugging
        JsonObject root = jsonBuffer.to<JsonObject>();

        String csvOutput = "";

        // get the time from the on-board RTC
        // Add the time to the Cayenne LPP Buffer
        lpp.addUnixTime(1, Logger::markedUTCEpochTime);
        // Add to the CSV
        csvOutput += dataLogger.rtc.stringTime8601TZ();
        csvOutput += ",";
        csvOutput += Logger::markedUTCEpochTime;
        dataLogger.watchDogTimer.resetWatchDog();

        // Get modem signal quality
        // NOTE: This is trivial, because the quality is added automatically
        int rssi = lora_modem.getSignalQuality();
        Serial.print(F("Signal quality: "));
        Serial.println(rssi);
        // Add the RSSI to the Cayenne LPP Buffer
        // lpp.addGenericSensor(2, rssi);
        // Add to the CSV
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
        // Add the temperature and humidity to the Cayenne LPP Buffer
        lpp.addTemperature(3, temp.temperature);
        lpp.addRelativeHumidity(4, humidity.relative_humidity);
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
            // stage in m (resolution 1mm)
            // lpp.addDistance(5, sdi12_results[0]);
            // distance in m (resolution 1mm)
            lpp.addDistance(6, sdi12_results[1]);
            // temperature in °C (resolution 0.1°C)
            lpp.addTemperature(7, sdi12_results[2]);
            // reliability in dB (resolution 0.1db)
            // lpp.addGenericSensor(8, sdi12_results[3]);
            // error code
            // lpp.addGenericSensor(9, sdi12_results[4]);
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
        lpp.addLuminosity(10, lux_val);
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
        lpp.addVoltage(11, cellVoltage);
        lpp.addPercentage(12, cellPercent);
        lpp.addGenericSensor(13, chargeRate);
        // Add to the CSV
        csvOutput += ",";
        csvOutput += String(cellVoltage, 3);
        csvOutput += ",";
        csvOutput += String(cellPercent, 1);
        csvOutput += ",";
        csvOutput += String(chargeRate, 1);
        dataLogger.watchDogTimer.resetWatchDog();

        // decode and print the buffer we just created
        Serial.println("Cayenne LPP Buffer:");
        printFrameHex(lpp.getBuffer(), lpp.getSize());
        Serial.println("\nDecoded Buffer:");
        lpp.decodeTTN(lpp.getBuffer(), lpp.getSize(), root);
        serializeJsonPretty(root, Serial);
        Serial.println();
        csvOutput += ",";
        for (int i = 0; i < lpp.getSize(); i++) {
            if (lpp.getBuffer()[i] < 16) { csvOutput += "0"; }
            csvOutput += String(lpp.getBuffer()[i], HEX);
        }
        dataLogger.watchDogTimer.resetWatchDog();

        // turn off the sensors since we have all data
        sensorPowerOff();

        // Save data to the SD Card
        dataLogger.logToSD(csvOutput);
        // Cut power from the SD card
        dataLogger.turnOffSDcard(true);
        dataLogger.watchDogTimer.resetWatchDog();


        // Send out the Cayenne LPP buffer
        if (loraStream.write(lpp.getBuffer(), lpp.getSize()) == lpp.getSize()) {
            Serial.println(F("  Successfully sent data"));
            dataLogger.watchDogTimer.resetWatchDog();
            if ((Logger::markedLocalEpochTime != 0 &&
                 Logger::markedLocalEpochTime % 86400 == 43200) ||
                !dataLogger.isRTCSane()) {
                Serial.println(F("Running a daily clock sync..."));
                // get the epoch time from the LoRa network
                uint32_t epochTime = ttn_modem.modemGetTime(lora_modem);
                // set the RTC time from the epoch
                if (epochTime != 0) {
                    Serial.print(F("Setting RTC epoch to "));
                    Serial.println(epochTime);
                    dataLogger.setNowUTCEpoch(epochTime);
                }
                dataLogger.watchDogTimer.resetWatchDog();
            }
        } else {
            Serial.println(F("--Failed to send data!"));
            bool res = lora_modem.isNetworkConnected();
            Serial.print(F("Network status: "));
            Serial.println(res ? "connected" : "not connected");
            dataLogger.watchDogTimer.resetWatchDog();
        }

        while (loraStream.available()) { Serial.write(loraStream.read()); }

        // put the modem to sleep
        ttn_modem.modemSleep(lora_modem);
    }

    // Call the processor sleep
    dataLogger.systemSleep();
}
