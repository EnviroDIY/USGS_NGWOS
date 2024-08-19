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
LoRa_AT        modem(debugger);
#else
LoRa_AT modem(SerialBee);
#endif

LoRaStream loraStream(modem);

void modemPowerOn() {
    if (power_pin_for_module >= 0) {
        Serial.print("Powering LoRa module with pin ");
        Serial.println(power_pin_for_module);
        pinMode(power_pin_for_module, OUTPUT);
        digitalWrite(power_pin_for_module, HIGH);

        Serial.println(F("Wait..."));
        delay(1000L);
    }
    if (arduino_wake_pin >= 0) {
        Serial.print("Waking LoRa module with pin ");
        Serial.println(arduino_wake_pin);
        pinMode(arduino_wake_pin, OUTPUT);
        digitalWrite(arduino_wake_pin, HIGH);
        Serial.println(F("Wait..."));
        delay(1000L);
    }
}

bool setupModem() {
    bool success = true;
    Serial.println(F("Initializing modem..."));
    success &= modem.init();

    String name = modem.getDevEUI();
    Serial.print(F("Device EUI: "));
    Serial.println(name);

    String modemInfo = modem.getModuleInfo();
    Serial.print(F("Module Info: "));
    Serial.println(modemInfo);

    _lora_class currClass = CLASS_A;
    if (modem.setClass(currClass)) {
        Serial.print(F("  Set LoRa device class to "));
        Serial.println((char)currClass);
    }

    bool isPublic = true;
    if (modem.setPublicNetwork(isPublic)) {
        Serial.print(F("  Set public network mode to "));
        Serial.println(isPublic ? "public network mode"
                                : "private network mode");
    }

    // get and set the current band to test functionality
    String currBand = modem.getBand();
    Serial.print(F("Device is currently using LoRa band "));
    Serial.println(currBand);

    // Set the frequency sub-band to '2' (used by The Things Network)
    int8_t subBand = 2;
    if (modem.setFrequencySubBand(subBand)) {
        Serial.print(F("  Set frequency sub-band to "));
        Serial.println(subBand);
    } else {
        Serial.println(F("--Failed to set frequency sub-band"));
    }


    // enable adaptive data rate
    // https://www.thethingsnetwork.org/docs/lorawan/adaptive-data-rate/
    bool useADR = true;
    if (modem.setAdaptiveDataRate(useADR)) {
        Serial.print(F("  Set to "));
        Serial.print(useADR ? "use" : "not use");
        Serial.println(F(" adaptive data rate"));
    } else {
        Serial.println(F("--Failed to set adaptive data rate"));
    }

    // Set the ack count to 0 (no confirmation)
    int8_t ackRetries = 0;
    if (modem.setConfirmationRetries(ackRetries)) {
        Serial.print(F("  Set ACK retry count to "));
        Serial.println(ackRetries);
    } else {
        Serial.println(F("--Failed to set ACK retry count"));
    }

    // Don't ask for message confirmation
    modem.requireConfirmation(false);


    return success;
}

bool modemConnect() {
    Serial.println(F("Attempting to join with OTAA..."));
    if (!modem.joinOTAA(appEui, appKey)) { return false; }
    return true;
}

uint32_t modemGetTime(uint8_t nRetries = 5) {
    uint32_t epochTime = 0;
    while ((epochTime < 1577836800 || epochTime > 1893474000) && nRetries) {
        Serial.println(F("Retrieving time as an offset from the epoch"));
        epochTime = modem.getDateTimeEpoch();
        Serial.print(F("  Current Epoch Time: "));
        Serial.println(epochTime);
        nRetries--;
    }
    return epochTime;
}

bool modemSleep() {
    if (arduino_wake_pin >= 0) {
        // test sleeping and waking with the an interrupt pin
        Serial.println(F("Putting modem to sleep until pin interrupt wake"));
        if (modem.pinSleep(lora_wake_pin, lora_wake_pullup, lora_wake_edge)) {
            Serial.println(F("  Put LoRa modem to sleep"));
            return true;
        } else {
            Serial.println(F("--Failed to put LoRa modem to sleep"));
            return false;
        }
    }
    return true;
}

bool modemWake() {
    if (arduino_wake_pin >= 0) {
        delay(5000L);
        digitalWrite(arduino_wake_pin, LOW);
        delay(50L);
        digitalWrite(arduino_wake_pin, HIGH);
        if (modem.testAT()) {
            Serial.println(F("  Woke up LoRa modem"));
            return true;
        } else {
            Serial.println(F("--Failed to wake LoRa modem"));
            return false;
        }
    }
    return true;
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


struct startMeasurementResult {  // Structure declaration
    String  returned_address;
    uint8_t meas_time_s;
    int     numberResults;
};

struct getResultsResult {  // Structure declaration
    uint8_t resultsReceived;
    uint8_t maxDataCommand;
    bool    addressMatch;
    bool    crcMatch;
    bool    errorCode;
    bool    success;
};

getResultsResult getResults(char address, int resultsExpected,
                            bool verify_crc = false, bool printCommands = true,
                            int8_t error_result_number = 0,
                            float  no_error_value      = 0) {
    uint8_t resultsReceived = 0;
    uint8_t cmd_number      = 0;
    // The maximum number of characters that can be returned in the <values>
    // part of the response to a D command is either 35 or 75. If the D command
    // is issued to retrieve data in response to a concurrent measurement
    // command, or in response to a high-volume ASCII measurement command, the
    // maximum is 75. The maximum is also 75 in response to a continuous
    // measurement command. Otherwise, the maximum is 35.
    int max_sdi_response = 76;
    // max chars in a unsigned 64 bit number
    int max_sdi_digits = 21;

    String compiled_response = "";

    bool success = true;

    // Create the return struct
    getResultsResult return_result;
    return_result.resultsReceived = 0;
    return_result.maxDataCommand  = 0;
    return_result.addressMatch    = true;
    return_result.crcMatch        = true;
    return_result.errorCode       = false;
    return_result.success         = true;

    while (resultsReceived < resultsExpected && cmd_number <= 9) {
        mySDI12.clearBuffer();
        String command = "";
        command += address;
        command += "D";
        command += cmd_number;
        command +=
            "!";  // SDI-12 command to get data [address][D][dataOption][!]
        mySDI12.sendCommand(command, wake_delay);

        if (printCommands) {
            Serial.print(">>>");
            Serial.println(command);
        }
        char resp_buffer[max_sdi_response] = {'\0'};

        // read bytes into the char array until we get to a new line (\r\n)
        size_t bytes_read = mySDI12.readBytesUntil('\n', resp_buffer,
                                                   max_sdi_response);

        size_t data_bytes_read = bytes_read -
            1;  // subtract one for the /r before the /n
        String sdiResponse = String(resp_buffer);
        compiled_response += sdiResponse;
        sdiResponse.trim();
        if (printCommands) {
            Serial.print("<<<");
            Serial.println(sdiResponse);
        }
        // read and clear anything else from the buffer
        int extra_chars = 0;
        while (mySDI12.available()) {
            Serial.write(mySDI12.read());
            extra_chars++;
        }
        if (extra_chars > 0) {
            Serial.print(extra_chars);
            Serial.println(" additional characters received.");
        }
        mySDI12.clearBuffer();

        // check the address, break if it's incorrect
        char returned_address = resp_buffer[0];
        if (returned_address != address) {
            if (printCommands) {
                Serial.println("Wrong address returned!");
                Serial.print("Expected ");
                Serial.print(String(address));
                Serial.print(" Got ");
                Serial.println(String(returned_address));
                Serial.println(String(resp_buffer));
            }
            success                    = false;
            return_result.addressMatch = false;
            break;
        }

        // check the crc, break if it's incorrect
        if (verify_crc) {
            bool crcMatch   = mySDI12.verifyCRC(sdiResponse);
            data_bytes_read = data_bytes_read - 3;
            if (crcMatch) {
                if (printCommands) { Serial.println("CRC valid"); }
            } else {
                if (printCommands) { Serial.println("CRC check failed!"); }
                return_result.crcMatch = false;
                success                = false;
                break;
            }
        }

        bool    gotResults                   = false;
        char    float_buffer[max_sdi_digits] = {'\0'};
        char*   dec_pl                       = float_buffer;
        uint8_t fb_pos                       = 0;  // start at start of buffer
        bool    finished_last_number         = false;
        // iterate through the char array and to check results
        // NOTE: start at 1 since we already looked at the address!
        for (size_t i = 1; i < data_bytes_read; i++) {
            // Get the character at position
            char c = resp_buffer[i];
            // if we didn't get something number-esque or we're at the end of
            // the buffer, assume the last number finished and parse it
            //(c != '-' && (c < '0' || c > '9') && c != '.')
            if (c == '-' || (c >= '0' && c <= '9') || c == '.') {
                // if there's a number, a decimal, or a negative sign next in
                // the buffer, add it to the float buffer.
                float_buffer[fb_pos] = c;
                fb_pos++;
                float_buffer[fb_pos] = '\0';  // null terminate the buffer
                finished_last_number = false;
            } else {
                finished_last_number = true;
            }
            // if we've gotten to the end of a number or the end of the buffer,
            // parse the character
            if ((finished_last_number || i == data_bytes_read - 1) &&
                strnlen(float_buffer, max_sdi_digits) > 0) {
                float result                   = atof(float_buffer);
                sdi12_results[resultsReceived] = result;
                if (printCommands) {
                    Serial.print("Result ");
                    Serial.print(resultsReceived);
                    Serial.print(", Raw value: ");
                    Serial.print(float_buffer);
                    dec_pl              = strchr(float_buffer, '.');
                    size_t len_post_dec = 0;
                    if (dec_pl != nullptr) {
                        len_post_dec = strnlen(dec_pl, max_sdi_digits) - 1;
                    }
                    Serial.print(", Len after decimal: ");
                    Serial.print(len_post_dec);
                    Serial.print(", Parsed value: ");
                    Serial.println(String(result, len_post_dec));
                }
                // add how many results we have
                if (result != -9999) {
                    gotResults = true;
                    resultsReceived++;
                }
                // check for a failure error code at the end
                if (error_result_number >= 1) {
                    if (resultsReceived == error_result_number &&
                        result != no_error_value) {
                        success                 = false;
                        return_result.errorCode = true;
                        if (printCommands) {
                            Serial.print("Got a failure code of ");
                            Serial.println(String(
                                result, strnlen(dec_pl, max_sdi_digits) - 1));
                        }
                    }
                }

                // empty the buffer
                float_buffer[0] = '\0';
                fb_pos          = 0;
            }
        }

        if (!gotResults) {
            if (printCommands) {
                Serial.println(
                    ("  No results received, will not continue requests!"));
            }
            break;
        }  // don't do another loop if we got nothing

        if (printCommands) {
            Serial.print("Total Results Received: ");
            Serial.print(resultsReceived);
            Serial.print(", Remaining: ");
            Serial.println(resultsExpected - resultsReceived);
        }

        cmd_number++;
    }

    mySDI12.clearBuffer();

    if (printCommands) {
        Serial.print("After ");
        Serial.print(cmd_number);
        Serial.print(" data commands got ");
        Serial.print(resultsReceived);
        Serial.print(" results of the expected ");
        Serial.print(resultsExpected);
        Serial.print(" expected. This is a ");
        Serial.println(resultsReceived == resultsExpected ? "success."
                                                          : "failure.");
    }

    success &= resultsReceived == resultsExpected;
    return_result.resultsReceived = resultsReceived;
    return_result.maxDataCommand  = cmd_number;
    return_result.success         = success;
    return return_result;
}

startMeasurementResult startMeasurement(char   address,
                                        bool   is_concurrent = false,
                                        bool   request_crc   = false,
                                        String meas_type     = "",
                                        bool   printCommands = true) {
    // Create the return struct
    startMeasurementResult return_result;
    return_result.returned_address = "";
    return_result.meas_time_s      = 0;
    return_result.numberResults    = 0;

    mySDI12.clearBuffer();

    String command = "";
    command += address;  // All commands start with the address
    command += is_concurrent ? "C" : "M";  // C for concurrent, M for standard
    command += request_crc ? "C" : "";  // add an additional C to request a CRC
    command += meas_type;               // Measurement type, "" or 0-9
    command += "!";                     // All commands end with "!"
    mySDI12.sendCommand(command, wake_delay);
    if (printCommands) {
        Serial.print(">>>");
        Serial.println(command);
    }

    // wait for acknowlegement with format [address][ttt (3 char,
    // seconds)][number of measurments available, 0-9]
    String sdiResponse = mySDI12.readStringUntil('\n');
    sdiResponse.trim();
    if (printCommands) {
        Serial.print("<<<");
        Serial.println(sdiResponse);
    }
    mySDI12.clearBuffer();

    // check the address, return if it's incorrect
    String returned_address = sdiResponse.substring(0, 1);
    char   ret_addr_array[2];
    returned_address.toCharArray(ret_addr_array, sizeof(ret_addr_array));
    return_result.returned_address = ret_addr_array[0];
    if (returned_address != String(address)) {
        if (printCommands) {
            Serial.println("Wrong address returned!");
            Serial.print("Expected ");
            Serial.print(String(address));
            Serial.print(" Got ");
            Serial.println(returned_address);
        }
        return return_result;
    }

    // find out how long we have to wait (in seconds).
    uint8_t meas_time_s       = sdiResponse.substring(1, 4).toInt();
    return_result.meas_time_s = meas_time_s;
    if (printCommands) {
        Serial.print("expected measurement time: ");
        Serial.print(meas_time_s);
        Serial.print(" s, ");
    }

    // Set up the number of results to expect
    int numResults              = sdiResponse.substring(4).toInt();
    return_result.numberResults = numResults;
    if (printCommands) {
        Serial.print("Number Results: ");
        Serial.println(numResults);
    }

    return return_result;
}

/**
 * @brief gets identification information from a sensor, and prints it to the
 * serial port
 *
 * @param i a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 * @param printCommands true to print the raw output and input from the command
 */
bool printInfo(char i, bool printCommands = true) {
    mySDI12.clearBuffer();
    String command = "";
    command += i;
    command += "I!";
    mySDI12.sendCommand(command, wake_delay);
    if (printCommands) {
        Serial.print(">>>");
        Serial.println(command);
    }
    delay(100);

    String sdiResponse = mySDI12.readStringUntil('\n');
    sdiResponse.trim();
    // allccccccccmmmmmmvvvxxx...xx<CR><LF>
    if (printCommands) {
        Serial.print("<<<");
        Serial.println(sdiResponse);
    }

    Serial.print("Address: ");
    Serial.print(sdiResponse.substring(0, 1));  // address
    Serial.print(", SDI-12 Version: ");
    Serial.print(sdiResponse.substring(1, 3).toFloat() /
                 10);  // SDI-12 version number
    Serial.print(", Vendor ID: ");
    Serial.print(sdiResponse.substring(3, 11));  // vendor id
    Serial.print(", Sensor Model: ");
    Serial.print(sdiResponse.substring(11, 17));  // sensor model
    Serial.print(", Sensor Version: ");
    Serial.print(sdiResponse.substring(17, 20));  // sensor version
    Serial.print(", Sensor ID: ");
    Serial.print(sdiResponse.substring(20));  // sensor id
    Serial.println();

    if (sdiResponse.length() < 3) { return false; };
    return true;
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
    modemPowerOn();
    setupModem();
    modemConnect();

    // get the epoch time from the LoRa network
    uint32_t epochTime = modemGetTime();

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
    modemSleep();
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
    modemWake();

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
    int rssi = modem.getSignalQuality();
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


    // Send out some the data
    if (loraStream.write(lpp.getBuffer(), lpp.getSize()) == lpp.getSize()) {
        Serial.println(F("  Successfully sent data"));
    } else {
        Serial.println(F("--Failed to send data!"));
        bool res = modem.isNetworkConnected();
        Serial.print(F("Network status: "));
        Serial.println(res ? "connected" : "not connected");
    }

    while (loraStream.available()) { Serial.write(loraStream.read()); }

    // put the modem to sleep
    modemSleep();
    sensorPowerOff();
    // wait 1 minute and repeat
    delay(60000L);
}
