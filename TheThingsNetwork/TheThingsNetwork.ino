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

// Create the RTC object
RV8803 rtc;

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


// Create the SHT object
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// Create the battery monitor object
Adafruit_MAX17048 maxlipo;

// The pin of the SDI-12 sensor power
const int8_t sdiPowerPin = 22;
// The pin of the SDI-12 data bus
const int8_t sdiDataPin = 3;
// The Vega's Address
const char vega_address = '0';
// Define the SDI-12 bus
SDI12 mySDI12(sdiDataPin);
// Extra time needed for the sensor to wake (0-100ms)
const uint32_t wake_delay = 10;
// array to hold sdi-12 results
float sdi12_results[10];

int8_t alsDataPin = 74;

// Your OTAA connection credentials, if applicable
// The App EUI (also called the Join EUI or the Network ID)
// This must be exactly 16 hex characters (8 bytes = 64 bits)
const char appEui[] = "YourAppEUI";
// The App Key (also called the network key)
// This must be exactly 32 hex characters (16 bytes = 128 bits)
const char appKey[] = "YourAppKey";

// the pin on your Arduino that will turn on power to the module
const int8_t power_pin_for_module = 18;
// the pin on your Arduino that will wake the module from pin sleep
const int8_t arduino_wake_pin = 23;
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
    if (sdiPowerPin >= 0) {
        Serial.print("Powering SDI-12 sensors with pin ");
        Serial.println(sdiPowerPin);
        pinMode(sdiPowerPin, OUTPUT);
        digitalWrite(sdiPowerPin, HIGH);
    }
}
void sensorPowerOff() {
    if (sdiPowerPin >= 0) {
        Serial.print("Cutting power SDI-12 sensors with pin ");
        Serial.println(sdiPowerPin);
        pinMode(sdiPowerPin, OUTPUT);
        digitalWrite(sdiPowerPin, LOW);
    }
}


void setup() {
    while (!Serial)
        ;

    // Set console baud rate
    Serial.begin(115200);
    delay(10);

    Wire.begin();


    if (rtc.begin() == false) {
        Serial.println(
            F("The RTC did not begin successfully. Please check the wiring."));
    } else {
        Serial.println(F("RTC online!"));
    }

    if (!sht4.begin()) {
        Serial.println(F("Couldn't find SHT4x"));
    } else {
        Serial.println(F("SHT4x online!"));
    }
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    ;

    if (!maxlipo.begin()) {
        Serial.println(F("Couldnt find Adafruit MAX17048?\nMake sure a battery "
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
    sensorPowerOn();

    // Set LoRa module baud rate
    SerialBee.begin(115200);
    // Power on, set up, and connect the LoRa modem
    modemPowerOn(power_pin_for_module);
    setupModemTTN(lora_modem);
    modemConnect(lora_modem, appEui, appKey);

    // get the epoch time from the LoRa network
    uint32_t epochTime = modemGetTime(lora_modem);

    // set the RTC time from the epoch
    if (epochTime != 0) {
        Serial.print(F("Setting RTC epoch to "));
        Serial.println(epochTime);
        rtc.setTimeZoneQuarterHours(0);
        rtc.setEpoch(epochTime);
    }

    // Confirm the date and time using the ISO 8601 timestamp
    String currentTime = rtc.stringTime8601();
    Serial.println(currentTime);

    // Print SDI-12 sensor info
    printInfo(vega_address, false);

    // put the modem to sleep
    modemSleep(arduino_wake_pin, lora_wake_pin, lora_wake_pullup,
               lora_wake_edge);
    sensorPowerOff();
}

void loop() {
    sensorPowerOn();
    delay(5200L);  // time for the VegaPuls to warm up
    // set lpp buffer pointer back to the head of the buffer
    lpp.reset();
    // Create a JsonArray object for decoding/debugging
    JsonObject root = jsonBuffer.to<JsonObject>();

    // wake up the modem
    modemWake(arduino_wake_pin);

    // Confirm the date and time using the ISO 8601 timestamp
    String currentTime = rtc.stringTime8601();
    Serial.println(currentTime);

    // get the time from the on-board RTC
    uint32_t epochTime = rtc.getEpoch();
    Serial.print(F("Epoch Time: "));
    Serial.println(epochTime);
    // Add the time to the Cayenne LPP Buffer
    lpp.addUnixTime(1, epochTime);

    // Get modem signal quality
    // NOTE: This is trivial, because the quality is added automatically
    int rssi = lora_modem.getSignalQuality();
    Serial.print(F("Signal quality: "));
    Serial.println(rssi);
    // Add the RSSI to the Cayenne LPP Buffer
    lpp.addGenericSensor(2, rssi);

    // Get temperature and humidity from the SHT40
    sensors_event_t humidity;
    sensors_event_t temp;
    sht4.getEvent(&humidity,
                  &temp);  // populate temp and humidity objects with fresh data
    Serial.print(F("Humidity: "));
    Serial.println(humidity.relative_humidity);
    Serial.print(F("Temperature: "));
    Serial.println(temp.temperature);
    // Add the temperature and humidity to the Cayenne LPP Buffer
    lpp.addTemperature(3, temp.temperature);
    lpp.addRelativeHumidity(4, humidity.relative_humidity);

    // Get SDI-12 Data
    startMeasurementResult startResult = startMeasurement(vega_address, false,
                                                          true, "", true);
    if (startResult.numberResults > 0) {
        uint32_t timerStart = millis();
        // wait up to 1 second longer than the specified return time
        while ((millis() - timerStart) <
               (static_cast<uint32_t>(startResult.meas_time_s) + 1) * 1000) {
            if (mySDI12.available()) {
                break;
            }  // sensor can interrupt us to let us know it is done early
        }
        String interrupt_response = mySDI12.readStringUntil('\n');

        getResults(vega_address, startResult.numberResults, true, true, 4, 0);
        lpp.addDistance(5, sdi12_results[0]);  // stage in m (resolution 1mm)
        lpp.addDistance(6,
                        sdi12_results[1]);  // distance in m (resolution 1mm)
        lpp.addTemperature(
            7, sdi12_results[2]);  // temperature in °C (resolution 0.1°C)
        lpp.addGenericSensor(
            8, sdi12_results[3]);  // reliability in dB (resolution 0.1db)
        lpp.addGenericSensor(9, sdi12_results[4]);  // error code
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

    // Read the battery monitor
    float cellVoltage = maxlipo.cellVoltage();
    float cellPercent = maxlipo.cellPercent();
    float chargeRate  = maxlipo.chargeRate();
    Serial.print(F("Batt Voltage: "));
    Serial.print(cellVoltage, 3);
    Serial.println(" V");
    Serial.print(F("Batt Percent: "));
    Serial.print(cellPercent, 1);
    Serial.println(" %");
    Serial.print(F("(Dis)Charge rate : "));
    Serial.print(chargeRate, 1);
    Serial.println(" %/hr");
    lpp.addVoltage(11, cellVoltage);
    lpp.addPercentage(12, cellPercent);
    lpp.addGenericSensor(13, chargeRate);

    // decode and print the buffer we just created
    Serial.println("Cayenne LPP Buffer:");
    printFrameHex(lpp.getBuffer(), lpp.getSize());
    Serial.println("\nDecoded Buffer:");
    lpp.decodeTTN(lpp.getBuffer(), lpp.getSize(), root);
    serializeJsonPretty(root, Serial);
    Serial.println();


    // Send out the Cayenne LPP buffer
    if (loraStream.write(lpp.getBuffer(), lpp.getSize()) == lpp.getSize()) {
        Serial.println(F("  Successfully sent data"));
    } else {
        Serial.println(F("--Failed to send data!"));
        bool res = lora_modem.isNetworkConnected();
        Serial.print(F("Network status: "));
        Serial.println(res ? "connected" : "not connected");
    }

    while (loraStream.available()) { Serial.write(loraStream.read()); }

    // put the modem to sleep
    modemSleep(arduino_wake_pin, lora_wake_pin, lora_wake_pullup,
               lora_wake_edge);
    sensorPowerOff();
    // wait 1 minute and repeat
    delay(60000L);
}
