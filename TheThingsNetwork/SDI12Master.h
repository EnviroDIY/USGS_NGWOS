// Header Guards
#ifndef SDI_12_MASTER_H_
#define SDI_12_MASTER_H_

#include <Arduino.h>
// The SDI-12 library for the Vega Puls
#include <SDI12.h>


// Extra time needed for the sensor to wake (0-100ms)
const uint32_t wake_delay = 10;

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

getResultsResult getResults(SDI12& _SDI12Internal, char address,
                            int resultsExpected, float sdi12_results[10],
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
        _SDI12Internal.clearBuffer();
        String command = "";
        command += address;
        command += "D";
        command += cmd_number;
        command +=
            "!";  // SDI-12 command to get data [address][D][dataOption][!]
        _SDI12Internal.sendCommand(command, wake_delay);

        if (printCommands) {
            Serial.print(">>>");
            Serial.println(command);
        }
        char resp_buffer[max_sdi_response] = {'\0'};

        // read bytes into the char array until we get to a new line (\r\n)
        size_t bytes_read = _SDI12Internal.readBytesUntil('\n', resp_buffer,
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
        while (_SDI12Internal.available()) {
            Serial.write(_SDI12Internal.read());
            extra_chars++;
        }
        if (extra_chars > 0) {
            Serial.print(extra_chars);
            Serial.println(" additional characters received.");
        }
        _SDI12Internal.clearBuffer();

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
            bool crcMatch   = _SDI12Internal.verifyCRC(sdiResponse);
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

    _SDI12Internal.clearBuffer();

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

startMeasurementResult startMeasurement(SDI12& _SDI12Internal, char address,
                                        bool   is_concurrent = false,
                                        bool   request_crc   = false,
                                        String meas_type     = "",
                                        bool   printCommands = true) {
    // Create the return struct
    startMeasurementResult return_result;
    return_result.returned_address = "";
    return_result.meas_time_s      = 0;
    return_result.numberResults    = 0;

    _SDI12Internal.clearBuffer();

    String command = "";
    command += address;  // All commands start with the address
    command += is_concurrent ? "C" : "M";  // C for concurrent, M for standard
    command += request_crc ? "C" : "";  // add an additional C to request a CRC
    command += meas_type;               // Measurement type, "" or 0-9
    command += "!";                     // All commands end with "!"
    _SDI12Internal.sendCommand(command, wake_delay);
    if (printCommands) {
        Serial.print(">>>");
        Serial.println(command);
    }

    // wait for acknowlegement with format [address][ttt (3 char,
    // seconds)][number of measurments available, 0-9]
    String sdiResponse = _SDI12Internal.readStringUntil('\n');
    sdiResponse.trim();
    if (printCommands) {
        Serial.print("<<<");
        Serial.println(sdiResponse);
    }
    _SDI12Internal.clearBuffer();

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
bool printInfo(SDI12& _SDI12Internal, char i, bool printCommands = true) {
    _SDI12Internal.clearBuffer();
    String command = "";
    command += i;
    command += "I!";
    _SDI12Internal.sendCommand(command, wake_delay);
    if (printCommands) {
        Serial.print(">>>");
        Serial.println(command);
    }
    delay(100);

    String sdiResponse = _SDI12Internal.readStringUntil('\n');
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

#endif
