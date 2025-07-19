/**************************************************************
 * @example{lineno} NGWOS_AWS_MQTT_Certs.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief An program to load certificates for AWS IoT Core on to your modem to
 * use for later connection.
 *
 * This program writes new certificates to the modem, connects to AWS IoT Core,
 * publishes an initial message, and then subscribes to a topic to toggle an
 * LED. After the initial connection, the board will check and try to reconnect
 * every 10 seconds and republish its status every 60 seconds.  If it receives
 * any messages on the subscribed topic, it will toggle the LED state.  The
 * content of any received messages is ignored.
 *
 * You should run this program once to load your certificates and confirm that
 * you can connect to AWS IoT Core over MQTT. Once you have confirmed your
 * certificates are loaded and working, there is no reason to rerun this program
 * unless you have a new modem, reset your modem, or your certificates change.
 * Most modules store the certificates in flash, which has a limited number of
 * read/write cycles. To avoid wearing out the flash unnecessarily, you should
 * only run this program when necessarily, don't re-write the certificates every
 * time you want to connect to AWS IoT Core.
 **************************************************************/

// Select your modem:
#define TINY_GSM_MODEM_SIM7080

#define TINY_GSM_TCP_KEEP_ALIVE 180

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
#define SerialAT SerialBee

// See all AT commands, if wanted
// WARNING: At high baud rates, incoming data may be lost when dumping AT
// commands
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 921600

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include "aws_iot_config.h"

// Your GPRS credentials, if any
const char apn[]      = "hologram";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
// get the broker host/endpoint from AWS IoT Core / Connect / Domain
// Configurations
const char* broker = AWS_IOT_ENDPOINT;
// the secure connection port for MQTT is always 8883
uint16_t port = 8883;
// the client ID should be the name of your "thing" in AWS IoT Core
const char* clientId = THING_NAME;

static const char topicInit[] TINY_GSM_PROGMEM      = THING_NAME "/init";
static const char topicLed[] TINY_GSM_PROGMEM       = THING_NAME "/led";
static const char topicLedStatus[] TINY_GSM_PROGMEM = THING_NAME "/ledStatus";

// The certificates should generally be formatted as ".pem", ".der", or (for
// some modules) ".p7b" files.

// For most modules the actual filename doesn't matter much but it CANNOT
// HAVE SPACES and should be less than 64 characters.
// Some modules will not accept filenames with special characters so avoid
// those, too.
// NOTE: The certificate names as they are downloaded from AWS IoT Core are
// often too long for the modem to handle. Pick something shorter.
const char* root_ca_name     = "AmazonRootCA1.pem";
const char* client_cert_name = THING_NAME "-certificate.pem.crt";
const char* client_key_name  = THING_NAME "-private-key.pem.key";

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClientSecure secureClient(modem, (uint8_t)0);
PubSubClient        mqtt(secureClient);

#define LED_PIN 13
int ledStatus = LOW;

// ======================== CERTIFICATE NAMES ========================

const char* root_ca     = AWS_SERVER_CERTIFICATE;
const char* client_cert = AWS_CLIENT_CERTIFICATE;
const char* client_key  = AWS_CLIENT_PRIVATE_KEY;

uint32_t lastReconnectAttempt = 0;
uint32_t lastStatusPublished  = 0;
bool     setupSuccess         = false;
bool     certificateSuccess   = false;

bool wakeModem() {
    // Set your reset, enable, and power pins here
    int8_t _modemPowerPin   = 18;  // Mayfly 1.1 & Stonefly
    int8_t _modemSleepRqPin = 23;  // Mayfly 1.1 & Stonefly
    int8_t _modemStatusPin  = 19;  // Mayfly 1.1 & Stonefly
    // set pin modes
    pinMode(_modemPowerPin, OUTPUT);
    pinMode(_modemSleepRqPin, OUTPUT);
    pinMode(_modemStatusPin, INPUT);
    // wake settings
    uint32_t _wakeDelay_ms = 1000L;  // SIM7080G
    uint32_t _wakePulse_ms = 1100L;  // SIM7080G
    bool     _wakeLevel =
        HIGH;  // SIM7080G is low, but EnviroDIY LTE Bee inverts it

    // start with the modem powered off
    DBG(F("Starting with modem powered down. Wait..."));
    digitalWrite(_modemSleepRqPin, !_wakeLevel);
    digitalWrite(_modemPowerPin, LOW);
    delay(5000L);

    // power the modem
    DBG(F("Powering modem with pin"), _modemPowerPin, F("and waiting"),
        _wakeDelay_ms, F("ms for power up."));
    digitalWrite(_modemPowerPin, HIGH);

    delay(_wakeDelay_ms);  // SIM7080G wake delay
    // wake the modem
    DBG(F("Sending a"), _wakePulse_ms, F("ms"),
        _wakeLevel ? F("HIGH") : F("LOW"), F("wake-up pulse on pin"),
        _modemSleepRqPin);
    digitalWrite(_modemSleepRqPin, _wakeLevel);
    delay(_wakePulse_ms);  // >1s
    digitalWrite(_modemSleepRqPin, !_wakeLevel);

    return true;
}

bool setModemBaud(uint32_t baud) {
    SerialMon.print(F("Setting modem baud rate to "));
    SerialMon.println(baud);
    if (!modem.setBaud(baud)) {
        SerialMon.println(F("...failed!"));
        return false;
    }
    SerialMon.println(F("...success!"));
    // Set the serial port to the new baud rate
    SerialAT.begin(baud);
    delay(100);
    return modem.init();  // May need to re-init to turn off echo, etc
}

void printModemInfo() {
    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem Info: ");
    SerialMon.println(modemInfo);
    String modemManufacturer = modem.getModemManufacturer();
    SerialMon.print("Modem Manufacturer: ");
    SerialMon.println(modemManufacturer);
    String modemModel = modem.getModemModel();
    SerialMon.print("Modem Model: ");
    SerialMon.println(modemModel);
    String modemRevision = modem.getModemRevision();
    SerialMon.print("Modem Revision: ");
    SerialMon.println(modemRevision);
    String modemSerial = modem.getModemSerialNumber();
    SerialMon.print("Modem Serial: ");
    SerialMon.println(modemSerial);
    String modemIMEI = modem.getIMEI();
    SerialMon.print("Modem IMEI: ");
    SerialMon.println(modemIMEI);
    String modemIMSI = modem.getIMSI();
    SerialMon.print("Modem IMSI: ");
    SerialMon.println(modemIMSI);
}

bool setupCertificates() {
    // ======================== CA CERTIFICATE LOADING ========================
    bool ca_cert_success = true;
    // add the server's certificate authority certificate to the modem
    SerialMon.print("Loading Certificate Authority Certificate");
    ca_cert_success &= modem.loadCertificate(root_ca_name, root_ca,
                                             strlen(root_ca));
    delay(250);
    if (!ca_cert_success) {
        SerialMon.println(" ...failed to load CA certificate!");
        return false;
    }
    SerialMon.println(" ...success");

    // print out the certificate to make sure it matches
    SerialMon.println(
        "Printing Certificate Authority Certificate to confirm it matches");
    modem.printCertificate(root_ca_name, SerialMon);
    delay(1000);

    // convert the certificate to the modem's format
    SerialMon.print("Converting Certificate Authority Certificate");
    ca_cert_success &= modem.convertCACertificate(root_ca_name);
    delay(250);
    if (!ca_cert_success) {
        SerialMon.println(" ...failed to convert CA certificate!");
        return false;
    }
    SerialMon.println(" ...success");

    // ===================== CLIENT CERTIFICATE LOADING =====================
    bool client_cert_success = true;
    // add the client's certificate and private key to the modem
    SerialMon.print("Loading Client Certificate");
    client_cert_success &= modem.loadCertificate(client_cert_name, client_cert,
                                                 strlen(client_cert));
    delay(250);

    // print out the certificate to make sure it matches
    modem.printCertificate(client_cert_name, SerialMon);
    delay(1000);

    SerialMon.print(" and Client Private Key ");
    client_cert_success &= modem.loadCertificate(client_key_name, client_key,
                                                 strlen(client_key));
    delay(250);

    // print out the certificate to make sure it matches
    modem.printCertificate(client_key_name, SerialMon);
    delay(1000);

    if (!client_cert_success) {
        SerialMon.println(" ...failed to load client certificate or key!");
        return false;
    }
    SerialMon.println(" ...success");
    // convert the client certificate pair to the modem's format
    client_cert_success &= modem.convertClientCertificates(client_cert_name,
                                                           client_key_name);
    delay(250);
    if (!client_cert_success) {
        SerialMon.println(" ...failed to convert client certificate and key!");
        return false;
    }
    SerialMon.println(" ...success");

    // ================= SET CERTIFICATES FOR THE CONNECTION =================
    // AWS IoT Core requires mutual authentication
    DBG("Requiring mutual authentication on socket");
    secureClient.setSSLAuthMode(SSLAuthMode::MUTUAL_AUTHENTICATION);
    DBG("Requesting TLS 1.3 on socket");
    secureClient.setSSLVersion(SSLVersion::TLS1_3);
    // attach the uploaded certificates to the secure client
    DBG("Assigning", root_ca_name, "as certificate authority on socket");
    secureClient.setCACertName(root_ca_name);
    DBG("Assigning", client_cert_name, "as client certificate on socket");
    secureClient.setClientCertName(client_cert_name);
    DBG("Assigning", client_key_name, "as client key on socket");
    secureClient.setPrivateKeyName(client_key_name);

    return ca_cert_success & client_cert_success;
}

String createStatusMessage() {
    String msgStatus = "{\"clientId\":\"" THING_NAME "\"";
    msgStatus += ",\"LED status\":\"" + String(ledStatus) + "\"";
    String modemIMEI = modem.getIMEI();
    msgStatus += ",\"modemIMEI\":\"" + modemIMEI + "\"";
    String modemSimCCID = modem.getSimCCID();
    msgStatus += ",\"modemSimCCID\":\"" + modemSimCCID + "\"";
    uint16_t modemService = modem.getSignalQuality();
    msgStatus += ",\"modemSignalQuality\":\"" + String(modemService) + "\"";
    String time = modem.getGSMDateTime(TinyGSMDateTimeFormat::DATE_FULL);
    msgStatus += ",\"modemTime\":\"" + time + "\"";
    msgStatus += "}";
    return msgStatus;
}

String createInitMessage() {
    String msgInit = "{\"clientId\":\"" THING_NAME "\"";

    String modemInfo = modem.getModemInfo();
    msgInit += ",\"modemInfo\":\"" + modemInfo + "\"";
    String modemManufacturer = modem.getModemManufacturer();
    msgInit += ",\"modemManufacturer\":\"" + modemManufacturer + "\"";
    String modemModel = modem.getModemModel();
    msgInit += ",\"modemModel\":\"" + modemModel + "\"";
    String modemRevision = modem.getModemRevision();
    msgInit += ",\"modemRevision\":\"" + modemRevision + "\"";
    String modemSerial = modem.getModemSerialNumber();
    msgInit += ",\"modemSerial\":\"" + modemSerial + "\"";
    String modemIMEI = modem.getIMEI();
    msgInit += ",\"modemIMEI\":\"" + modemIMEI + "\"";
    String modemIMSI = modem.getIMSI();
    msgInit += ",\"modemIMSI\":\"" + modemIMSI + "\"";
    String modemSimCCID = modem.getSimCCID();
    msgInit += ",\"modemSimCCID\":\"" + modemSimCCID + "\"";
    uint16_t modemService = modem.getSignalQuality();
    msgInit += ",\"modemSignalQuality\":\"" + String(modemService) + "\"";
    String time = modem.getGSMDateTime(TinyGSMDateTimeFormat::DATE_FULL);
    msgInit += ",\"modemTime\":\"" + time + "\"";
    msgInit += "}";

    return msgInit;
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
    SerialMon.print("Message arrived [");
    SerialMon.print(topic);
    SerialMon.print("]: ");
    SerialMon.write(payload, len);
    SerialMon.println();

    // Only proceed if incoming message's topic matches
    if (String(topic) == topicLed) {
        ledStatus = !ledStatus;
        digitalWrite(LED_PIN, ledStatus);

        // Create a status message to send to the broker
        String msgStatus = createStatusMessage();
        mqtt.publish(topicLedStatus, msgStatus.c_str());
    }
}

bool mqttConnect() {
    SerialMon.print("Connecting to ");
    SerialMon.print(broker);
    SerialMon.print(" with client ID ");
    SerialMon.println(clientId);

    // Connect to MQTT Broker
    bool status = mqtt.connect(clientId);

    if (status == false) {
        SerialMon.println(" ...failed to connect to AWS IoT MQTT broker!");
        return false;
    }
    SerialMon.println(" ...success");

    // Create a init message to send to the broker
    String msgInit = createInitMessage();

    // Make sure the MQTT buffer is large enough to hold the
    // initial message and the topic name.
    uint16_t neededBuffer = MQTT_MAX_HEADER_SIZE + 2 +
        strnlen(topicInit, mqtt.getBufferSize()) + msgInit.length() + 1;
    if (mqtt.getBufferSize() < neededBuffer) {
        SerialMon.print("Increasing MQTT buffer size from ");
        SerialMon.print(mqtt.getBufferSize());
        SerialMon.print(" to ");
        SerialMon.println(neededBuffer);
        mqtt.setBufferSize(neededBuffer);
    }

    SerialMon.print("Publishing a message to ");
    SerialMon.println(topicInit);
    SerialMon.print("Message content: ");
    SerialMon.println(msgInit);

    bool got_pub = mqtt.publish(topicInit, msgInit.c_str());
    SerialMon.println(got_pub ? "published" : "failed to publish");
    SerialMon.print("Subscribing to ");
    SerialMon.println(topicLed);
    bool got_sub = mqtt.subscribe(topicLed);
    SerialMon.println(got_sub ? "subscribed" : "failed to subscribe");

    return mqtt.connected();
}

bool mqttPublishStatus() {
    SerialMon.print("Publishing a message to ");
    SerialMon.println(topicLedStatus);

    // Create a status message to send to the broker
    String msgStatus = createStatusMessage();
    bool   got_pub   = mqtt.publish(topicLedStatus, msgStatus.c_str());

    SerialMon.println(got_pub ? "published" : "failed to publish");
    return got_pub;
}

bool setupModem() {
    bool success = true;

    // Attempt to autobaud the modem
    TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.print("Initializing modem...");
    if (!modem.init()) {  // modem.restart();
        SerialMon.println(" ...failed to initialize modem!");
        delay(15000L);
        return false;
    }
    SerialMon.println(" ...success");

    // Max out the baud rate, if desired
    // NOTE: Do this **AFTER** the modem has been restarted - many modules
    // revert to default baud rates when reset or powered off.
    success &= setModemBaud(921600L);

    printModemInfo();

    return success;
}

bool setupNetwork() {
    // enable/force time sync with NTP server
    // This is **REQUIRED** for validated SSL connections
    DBG("Enabling time sync with NTP server");
    modem.NTPServerSync("pool.ntp.org", -4);
    return true;
}

bool getInternetConnection() {
    // Make sure we're registered on the network
    if (!modem.isNetworkConnected()) {
        SerialMon.println("Network disconnected");
        SerialMon.println("Waiting up to 5 minutes for network connection...");
        if (!modem.waitForNetwork(300000L, true)) {
            SerialMon.println(" ...failed to reconnect to network!");
            delay(15000L);
            return false;
        }
        if (modem.isNetworkConnected()) {
            SerialMon.println("Network connected");
        }
    }

    // Make sure GPRS/EPS is connected
    if (!modem.isGprsConnected()) {
        SerialMon.println("GPRS disconnected!");
        SerialMon.print(F("Connecting to "));
        SerialMon.println(apn);
        if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
            SerialMon.println(" ...failed to connect to GPRS!");
            delay(15000L);
            return false;
        }
        if (modem.isGprsConnected()) { SerialMon.println("GPRS reconnected"); }
    }

    // check and print the signal quality for debugging
    uint16_t modemService = modem.getSignalQuality();
    SerialMon.print("Signal Quality: ");
    SerialMon.println(modemService);

    // check and print the current network time to ensure that the modem has
    // synchronized with the NTP server
    String time = modem.getGSMDateTime(TinyGSMDateTimeFormat::DATE_FULL);
    DBG("Current Network Time:", time);

    return true;
}


void setup() {
    // Set console baud rate
    SerialMon.begin(921600);
    delay(10);

    while (!SerialMon && millis() < 10000L) {}

    pinMode(LED_PIN, OUTPUT);

    // MQTT Broker setup
    // NOTE: This is only configuring the server and callback within the
    // PubSubClient object.
    // It does not take any action.
    mqtt.setServer(broker, port);
    mqtt.setCallback(mqttCallback);

    wakeModem();

    DBG("Wait...");
    delay(500L);

    SerialMon.println("Setting up modem...");
    setupSuccess = setupModem();
    if (!setupSuccess) {
        SerialMon.println(" ...failed to set up modem!");
        delay(15000L);
        return;
    }
    SerialMon.println(" ...success");

    SerialMon.println("Loading and configuring certificates...");
    certificateSuccess = setupCertificates();
    if (!certificateSuccess) {
        SerialMon.println(" ...failed to set up certificates!");
        delay(15000L);
        return;
    }
    SerialMon.println(" ...success");

    SerialMon.println("Setting up network...");
    if (!setupNetwork()) {
        SerialMon.println(" ...failed to set up network!");
        delay(15000L);
        return;
    }
    SerialMon.println(" ...success");

    getInternetConnection();

    delay(500);
    DBG("Finished setup");
}

void loop() {
    if (!setupSuccess) {
        SerialMon.println("Modem setup failed, re-trying...");
        setupSuccess = setupModem();
        if (!setupSuccess) {
            SerialMon.println(" ...failed to set up modem!");
            delay(15000L);
            return;
        }
        SerialMon.println(" ...success");
    }

    if (!certificateSuccess) {
        SerialMon.println(
            "Modem certificate configuration failed, re-trying...");
        certificateSuccess = setupCertificates();
        if (!certificateSuccess) {
            SerialMon.println(" ...failed to set up certificates!");
            delay(15000L);
            return;
        }
        SerialMon.println(" ...success");
    }

    // Re-test and reconnect if necessary every 10 seconds
    if (millis() - lastReconnectAttempt > 10000L) {
        lastReconnectAttempt = millis();
        if (!mqtt.connected()) {
            SerialMon.println("=== MQTT NOT CONNECTED ===");
            if (getInternetConnection()) { mqttConnect(); }
        }
    }

    // publish the current LED status every 60 seconds
    if (millis() - lastStatusPublished > 60000L) {
        lastStatusPublished = millis();
        if (mqtt.connected()) { mqttPublishStatus(); }
    }

    mqtt.loop();
}
