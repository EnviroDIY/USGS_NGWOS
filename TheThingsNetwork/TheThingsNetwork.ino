#include <Arduino.h>

// Select your LoRa Module:
#define LORA_AT_MDOT

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
#define SerialAT SerialBee

// Define the serial console for debug prints, if needed
#define LORA_AT_DEBUG Serial

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define LORA_AUTOBAUD_MIN 9600
#define LORA_AUTOBAUD_MAX 115200

// Include the LoRa library
#include <LoRa_AT.h>

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
StreamDebugger debugger(SerialAT, SerialMon);
LoRa_AT        modem(debugger);
#else
LoRa_AT modem(SerialAT);
#endif

LoRaStream loraStream(modem);

// put function declarations here:
int myFunction(int, int);

void setup() {
    while (!Serial)
        ;

    // Set console baud rate
    SerialMon.begin(115200);
    delay(10);

    // !!!!!!!!!!!
    // Set your reset, enable, power pins here
    if (power_pin_for_module >= 0) {
        SerialMon.print("Powering LoRa module with pin ");
        SerialMon.println(power_pin_for_module);
        pinMode(power_pin_for_module, OUTPUT);
        digitalWrite(power_pin_for_module, HIGH);
    }
    if (arduino_wake_pin >= 0) {
        SerialMon.print("Waking LoRa module with pin ");
        SerialMon.println(arduino_wake_pin);
        pinMode(arduino_wake_pin, OUTPUT);
        digitalWrite(arduino_wake_pin, HIGH);
    }
    // !!!!!!!!!!!

    SerialMon.println(F("Wait..."));
    delay(6000);

    // Set GSM module baud rate
    LoRa_AT_AutoBaud(SerialAT, LORA_AUTOBAUD_MIN, LORA_AUTOBAUD_MAX);
    // SerialAT.begin(9600);
}

void loop() {
    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println(F("Initializing modem..."));
    if (!modem.restart()) {
        // if (!modem.init()) {
        SerialMon.println(
            F("--Failed to restart modem, delaying 10s and retrying"));
        // restart autobaud in case GSM just rebooted
        LoRa_AT_AutoBaud(SerialAT, LORA_AUTOBAUD_MIN, LORA_AUTOBAUD_MAX);
        return;
    }

    String name = modem.getDevEUI();
    SerialMon.print(F("Device EUI: "));
    SerialMon.println(name);
    delay(500L);

    String modemInfo = modem.getModuleInfo();
    SerialMon.print(F("Module Info: "));
    SerialMon.println(modemInfo);
    delay(500L);

    _lora_class currClass = CLASS_A;
    if (modem.setClass(currClass)) {
        SerialMon.print(F("  Set LoRa device class to "));
        SerialMon.println((char)currClass);
    }
    delay(500L);

    bool isPublic = true;
    if (modem.setPublicNetwork(isPublic)) {
        SerialMon.print(F("  Reset public network mode to "));
        SerialMon.println(isPublic ? "public network mode"
                                   : "private network mode");
    }
    delay(500L);

    // get and set the current band to test functionality
    String currBand = modem.getBand();
    SerialMon.print(F("Device is currently using LoRa band "));
    SerialMon.println(currBand);
    delay(500L);

    // get and set the ack status to test functionality
    int8_t ackRetries = 1;
    if (modem.setConfirmationRetries(ackRetries)) {
        SerialMon.print(F("  Set ACK retry count to "));
        SerialMon.println(ackRetries);
    } else {
        SerialMon.println(F("--Failed to set ACK retry count"));
    }
    delay(500L);

    SerialMon.println(F("Attempting to join with OTAA..."));
    if (!modem.joinOTAA(appEui, appKey)) {
        SerialMon.println(F("--fail\n"));
        // power cycle
        if (power_pin_for_module >= 0) {
            digitalWrite(power_pin_for_module, LOW);
        }
        delay(10000);
        if (power_pin_for_module >= 0) {
            digitalWrite(power_pin_for_module, HIGH);
            delay(5000);
        }
        return;
    }
    SerialMon.println(F("  success"));
    delay(2000L);

    String devAddr = modem.getDevAddr();
    SerialMon.print(F("Device (Network) Address: "));
    SerialMon.println(devAddr);
    delay(2000L);

    String appEUI = modem.getAppEUI();
    SerialMon.print(F("App EUI (network ID): "));
    SerialMon.println(appEUI);
    delay(2000L);

    String appKey = modem.getAppKey();
    SerialMon.print(F("App Key (network key): "));
    SerialMon.println(appKey);
    delay(2000L);

    bool res = modem.isNetworkConnected();
    SerialMon.print(F("Network status: "));
    SerialMon.println(res ? "connected" : "not connected");
    delay(2000L);
    // we may get downlink from the network link check in the isNetworkConnected
    // fxn
    if (loraStream.available()) {
        SerialMon.print(F("Got downlink data!\n<<<"));
        while (loraStream.available()) { SerialMon.write(loraStream.read()); }
        SerialMon.println(F(">>>"));
    }
    delay(2000L);

    // Send out some data, without confirmation
    String short_message = "hello";
    SerialMon.println(F("Sending a short message without confirmation"));
    modem.requireConfirmation(false);
    if (loraStream.print(short_message) == short_message.length()) {
        SerialMon.println(F("  Successfully sent unconfirmed data"));
    } else {
        SerialMon.println(F("--Failed to send unconfirmed data!"));
        res = modem.isNetworkConnected();
        SerialMon.print(F("Network status: "));
        SerialMon.println(res ? "connected" : "not connected");
    }
    if (loraStream.available()) {
        SerialMon.print(F("Got downlink data!\n<<<"));
        while (loraStream.available()) { SerialMon.write(loraStream.read()); }
        SerialMon.println(F(">>>"));
    }
    delay(2000L);
    // Send out some data, requiring confirmation
    modem.requireConfirmation(true);
    SerialMon.println(F("Sending a short message with confirmation"));
    if (loraStream.print(short_message) == short_message.length()) {
        SerialMon.println(F("  Successfully sent acknowledged data"));
    } else {
        SerialMon.println(F("--Failed to send acknowledged data!"));
        res = modem.isNetworkConnected();
        SerialMon.print(F("Network status: "));
        SerialMon.println(res ? "connected" : "not connected");
    }
    if (loraStream.available()) {
        SerialMon.print(F("Got downlink data!\n<<<"));
        while (loraStream.available()) { SerialMon.write(loraStream.read()); }
        SerialMon.println(F(">>>"));
    }
    delay(2000L);
    // Send out some longer data, without confirmation
    modem.requireConfirmation(false);
    String longer_message = "This is a longer message";
    SerialMon.println(F("Sending a longer message without confirmation"));
    if (loraStream.print(longer_message) == longer_message.length()) {
        SerialMon.println(F("  Successfully sent longer unconfirmed message"));
    } else {
        SerialMon.println(F("--Failed to send longer unconfirmed message!"));
        res = modem.isNetworkConnected();
        SerialMon.print(F("Network status: "));
        SerialMon.println(res ? "connected" : "not connected");
    }
    if (loraStream.available()) {
        SerialMon.print(F("Got downlink data!\n<<<"));
        while (loraStream.available()) { SerialMon.write(loraStream.read()); }
        SerialMon.println(F(">>>"));
    }
    delay(2000L);
    String super_long_message =
        "This is a really long message that I'm using to test and ensure that "
        "packets are being broken up correctly by the send function.  Lorem "
        "ipsum dolor sit amet, consectetur adipiscing elit. Quisque vulputate "
        "dolor vitae ante vehicula molestie. Duis in quam nec dolor varius "
        "lobortis. Proin eget malesuada odio. Etiam condimentum sodales "
        "hendrerit. Curabitur molestie sem vel sagittis commodo. Proin ut "
        "tortor "
        "sodales, molestie nisl eget, ultricies dolor. Donec bibendum, dui nec "
        "pharetra ornare, ex velit ullamcorper mi, eu facilisis purus turpis "
        "quis dolor. Suspendisse rhoncus nisl non justo tempor vulputate. Duis "
        "sit amet metus sit amet leo tristique venenatis nec in libero. "
        "Maecenas "
        "pharetra enim quis ornare aliquet. Aenean pretium cursus magna, "
        "fringilla auctor metus faucibus non. Nulla blandit mauris a quam "
        "tincidunt commodo. Duis dapibus lorem eget augue ornare, id lobortis "
        "quam volutpat.";
    SerialMon.println(F("Sending a super long message without confirmation"));
    if (loraStream.print(super_long_message) == super_long_message.length()) {
        SerialMon.println(F("  Successfully sent super long message"));
    } else {
        SerialMon.println(F("--Failed to send super long message!"));
        res = modem.isNetworkConnected();
        SerialMon.print(F("Network status: "));
        SerialMon.println(res ? "connected" : "not connected");
    }
    if (loraStream.available()) {
        SerialMon.print(F("Got downlink data!\n<<<"));
        while (loraStream.available()) { SerialMon.write(loraStream.read()); }
        SerialMon.println(F(">>>"));
    }
    delay(2000L);

    int rssi = modem.getSignalQuality();
    SerialMon.print(F("Signal quality: "));
    SerialMon.println(rssi);
    delay(2000L);

    SerialMon.println(F("Retrieving time as an offset from the epoch"));
    uint32_t epochTime = modem.getDateTimeEpoch();
    SerialMon.print(F("  Current Epoch Time: "));
    SerialMon.println(epochTime);

#if defined LORA_AT_HAS_BATTERY
    int8_t  chargeState = -99;
    int8_t  percent     = -99;
    int16_t milliVolts  = -9999;
    modem.getBattStats(chargeState, percent, milliVolts);
    SerialMon.print(F("Battery charge state: "));
    SerialMon.println(chargeState);
    SerialMon.print(F("Battery charge 'percent': "));
    SerialMon.println(percent);
    SerialMon.print(F("Battery voltage: "));
    SerialMon.println(milliVolts / 1000.0F);
    delay(2000L);
#endif

#if defined LORA_AT_HAS_TEMPERATURE
    float temp = modem.getTemperature();
    SerialMon.print(F("Chip temperature: "));
    SerialMon.println(temp);
#endif

#if defined LORA_AT_HAS_SLEEP_MODE
    // test sleeping and waking with the UART
    SerialMon.println(F("Testing basic sleep mode with UART wake after 5s"));
    if (modem.uartSleep()) {  // could alo use sleep();
        SerialMon.println(F("  Put LoRa modem to sleep"));
    } else {
        SerialMon.println(F("--Failed to put LoRa modem to sleep"));
    }
    delay(5000L);
    if (modem.testAT()) {
        SerialMon.println(F("  Woke up LoRa modem"));
    } else {
        SerialMon.println(F("--Failed to wake LoRa modem"));
    }

    if (arduino_wake_pin >= 0) {
        // test sleeping and waking with the an interrupt pin
        SerialMon.println(
            F("Testing basic sleep mode with pin interrupt wake after 5s"));
        if (modem.pinSleep(lora_wake_pin, lora_wake_pullup, lora_wake_edge)) {
            SerialMon.println(F("  Put LoRa modem to sleep"));
        } else {
            SerialMon.println(F("--Failed to put LoRa modem to sleep"));
        }
        delay(5000L);
        digitalWrite(arduino_wake_pin, LOW);
        delay(50L);
        digitalWrite(arduino_wake_pin, HIGH);
        if (modem.testAT()) {
            SerialMon.println(F("  Woke up LoRa modem"));
        } else {
            SerialMon.println(F("--Failed to wake LoRa modem"));
        }
    }
#endif

    SerialMon.println(F("End of tests.\n"));
    SerialMon.println(F("----------------------------------------"));
    SerialMon.println(F("----------------------------------------\n"));

    // Just listen forevermore
    while (true) {
        modem.maintain();
        if (loraStream.available()) {
            SerialMon.print(F("Got downlink data!\n<<<"));
            while (loraStream.available()) {
                SerialMon.write(loraStream.read());
            }
            SerialMon.println(F(">>>"));
        }
    }
}
