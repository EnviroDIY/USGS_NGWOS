
void modemPowerOn(int8_t _power_pin_for_module) {
    if (_power_pin_for_module >= 0) {
        Serial.print("Powering LoRa module with pin ");
        Serial.println(_power_pin_for_module);
        pinMode(_power_pin_for_module, OUTPUT);
        digitalWrite(_power_pin_for_module, HIGH);

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

bool setupModemTTN(LoRa_AT& _lora_modem) {
    bool success = true;
    Serial.println(F("Initializing modem..."));
    success &= _lora_modem.init();

    String name = _lora_modem.getDevEUI();
    Serial.print(F("Device EUI: "));
    Serial.println(name);

    String modemInfo = _lora_modem.getModuleInfo();
    Serial.print(F("Module Info: "));
    Serial.println(modemInfo);

    _lora_class currClass = CLASS_A;
    if (_lora_modem.setClass(currClass)) {
        Serial.print(F("  Set LoRa device class to "));
        Serial.println((char)currClass);
    }

    bool isPublic = true;
    if (_lora_modem.setPublicNetwork(isPublic)) {
        Serial.print(F("  Set public network mode to "));
        Serial.println(isPublic ? "public network mode"
                                : "private network mode");
    }

    // get and set the current band to test functionality
    String currBand = _lora_modem.getBand();
    Serial.print(F("Device is currently using LoRa band "));
    Serial.println(currBand);

    // Set the frequency sub-band to '2' (used by The Things Network)
    int8_t subBand = 2;
    if (_lora_modem.setFrequencySubBand(subBand)) {
        Serial.print(F("  Set frequency sub-band to "));
        Serial.println(subBand);
    } else {
        Serial.println(F("--Failed to set frequency sub-band"));
    }

    // enable adaptive data rate
    // https://www.thethingsnetwork.org/docs/lorawan/adaptive-data-rate/
    bool useADR = true;
    if (_lora_modem.setAdaptiveDataRate(useADR)) {
        Serial.print(F("  Set to "));
        Serial.print(useADR ? "use" : "not use");
        Serial.println(F(" adaptive data rate"));
    } else {
        Serial.println(F("--Failed to set adaptive data rate"));
    }

    // Set the ack count to 0 (no confirmation)
    int8_t ackRetries = 0;
    if (_lora_modem.setConfirmationRetries(ackRetries)) {
        Serial.print(F("  Set ACK retry count to "));
        Serial.println(ackRetries);
    } else {
        Serial.println(F("--Failed to set ACK retry count"));
    }

    // Don't ask for message confirmation
    _lora_modem.requireConfirmation(false);

    return success;
}

bool modemConnect(LoRa_AT& _lora_modem, const char* _appEui,
                  const char* appKey) {
    Serial.println(F("Attempting to join with OTAA..."));
    if (!_lora_modem.joinOTAA(_appEui, appKey)) { return false; }
    return true;
}

uint32_t modemGetTime(LoRa_AT& _lora_modem, uint8_t nRetries = 5) {
    uint32_t epochTime = 0;
    while ((epochTime < 1577836800 || epochTime > 1893474000) && nRetries) {
        Serial.println(F("Retrieving time as an offset from the epoch"));
        epochTime = _lora_modem.getDateTimeEpoch(UNIX);
        Serial.print(F("  Current Epoch Time: "));
        Serial.println(epochTime);
        nRetries--;
    }
    return epochTime;
}

bool modemSleep(const int8_t _arduino_wake_pin, const int8_t _lora_wake_pin,
                const int8_t _lora_wake_pullup, const int8_t _lora_wake_edge) {
    if (_arduino_wake_pin >= 0) {
        // test sleeping and waking with the an interrupt pin
        Serial.println(F("Putting modem to sleep until pin interrupt wake"));
        if (lora_modem.pinSleep(_lora_wake_pin, _lora_wake_pullup,
                                _lora_wake_edge)) {
            Serial.println(F("  Put LoRa modem to sleep"));
            return true;
        } else {
            Serial.println(F("--Failed to put LoRa modem to sleep"));
            return false;
        }
    }
    return true;
}

bool modemWake(const int8_t _arduino_wake_pin) {
    if (_arduino_wake_pin >= 0) {
        delay(5000L);
        digitalWrite(_arduino_wake_pin, LOW);
        delay(50L);
        digitalWrite(_arduino_wake_pin, HIGH);
        if (lora_modem.testAT()) {
            Serial.println(F("  Woke up LoRa modem"));
            return true;
        } else {
            Serial.println(F("--Failed to wake LoRa modem"));
            return false;
        }
    }
    return true;
}
