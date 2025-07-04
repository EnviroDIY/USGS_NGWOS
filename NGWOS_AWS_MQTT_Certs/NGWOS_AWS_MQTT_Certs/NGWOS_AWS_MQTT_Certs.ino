/**************************************************************
 * @example{lineno} NGWOS_AWS_MQTT_Certs.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief An program to load certificates for AWS IoT Core on to your modem to
 * use for later connection.
 *
 * You should run this program once to load your certificates and confirm that
 * you can connect to AWS IoT Core over MQTT.  Once you have confirmed your
 * certificates are loaded and working, there is no reason to rerun this program
 * unless you have a new modem, reset your modem, or your certificates change.
 * Most modules store the certificates in flash, which has a limited number of
 * read/write cycles. To avoid wearing out the flash unnecessarily, you should
 * only run this program when necessarily, don't re-write the certificates every
 * time you want to connect to AWS IoT Core.
 *
 * @note This only works for modules that have support for both using and
 * **loading** certificates in TinyGSM. Modules that support SSL, but not
 *writing certificates, cannot use this example!
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

// set GSM PIN, if any
#define GSM_PIN ""

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

static const char topicInit[] TINY_GSM_PROGMEM = THING_NAME "/init";
static const char msgInit[] TINY_GSM_PROGMEM   = "{\"" THING_NAME
                                               "\":\"connected\"}";


// The certificates should generally be formatted as ".pem", ".der", or (for
// some modules) ".p7b" files.

// For most modules the actual filename doesn't matter much but it CANNOT
// HAVE SPACES and should be less than 64 characters.
// NOTE: The certificate names as they are downloaded from AWS IoT Core
// are often too long for the modem to handle. Pick something shorter.
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

TinyGsmClientSecure secureClient(modem);
PubSubClient        mqtt(secureClient);

#define LED_PIN 13
int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;

boolean mqttConnect() {
    SerialMon.print("Connecting to ");
    SerialMon.print(broker);
    SerialMon.print(" with client ID ");
    SerialMon.println(clientId);

    // Connect to MQTT Broker
    boolean status = mqtt.connect(clientId);

    if (status == false) {
        SerialMon.println(" ...failed to connect to AWS IoT MQTT broker!");
        return false;
    }
    SerialMon.println(" ...success");

    SerialMon.print("Publishing a message to ");
    SerialMon.println(topicInit);
    bool got_pub = mqtt.publish(topicInit, msgInit);
    SerialMon.println(got_pub ? "published" : "failed to publish");

    return mqtt.connected();
}


void setup() {
    // Set console baud rate
    SerialMon.begin(921600);
    delay(10);

    while (!SerialMon && millis() < 10000L) {}

    pinMode(LED_PIN, OUTPUT);

    // !!!!!!!!!!!
    // Set your reset, enable, power pins here
    // pins
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

    DBG("Wait...");
    delay(500L);

    // Set GSM module baud rate
    TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    // SerialAT.begin(57600);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.print("Initializing modem...");
    if (!modem.restart()) {  // modem.init();
        SerialMon.println(" ...failed to initialize modem!");
        delay(10000);
        return;
    }
    SerialMon.println(" ...success");

    // Max out the baud rate, if desired
    // NOTE: Do this **AFTER** the modem has been restarted - many modules
    // revert to default baud rates when reset or powered off. 921600, 460800,
    // 230400, 115200
    modem.setBaud(921600);
    SerialAT.end();
    delay(100);
    SerialAT.begin(921600);
    delay(100);
    modem.init();  // May need to re-init to turn off echo, etc

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

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }

    // ======================== CERTIFICATE NAMES ========================
    // The certificates are stored in the "certificates.h" file

    const char* root_ca     = AWS_SERVER_CERTIFICATE;
    const char* client_cert = AWS_CLIENT_CERTIFICATE;
    const char* client_key  = AWS_CLIENT_PRIVATE_KEY;
    // ======================== CA CERTIFICATE LOADING ========================
    bool ca_cert_success = true;
    // add the server's certificate authority certificate to the modem
    SerialMon.print("Loading Certificate Authority Certificate");
    ca_cert_success &= modem.loadCertificate(root_ca_name, root_ca,
                                             strlen(root_ca));
    if (!ca_cert_success) {
        SerialMon.println(" ...failed to load CA certificate!");
        delay(10000);
        return;
    }
    SerialMon.println(" ...success");
    // print out the certificate to make sure it matches
    SerialMon.println(
        "Printing Certificate Authority Certificate to confirm it matches");
    modem.printCertificate(root_ca_name, SerialMon);
    // convert the certificate to the modem's format
    SerialMon.print("Converting Certificate Authority Certificate");
    ca_cert_success &= modem.convertCACertificate(root_ca_name);
    if (!ca_cert_success) {
        SerialMon.println(" ...failed to convert CA certificate!");
        delay(10000);
        return;
    }
    SerialMon.println(" ...success");

    // ===================== CLIENT CERTIFICATE LOADING =====================
    bool client_cert_success = true;
    // add the client's certificate and private key to the modem
    SerialMon.print("Loading Client Certificate");
    client_cert_success &= modem.loadCertificate(client_cert_name, client_cert,
                                                 strlen(client_cert));
    // print out the certificate to make sure it matches
    modem.printCertificate(client_cert_name, SerialMon);
    delay(1000);
    SerialMon.print(" and Client Private Key ");
    client_cert_success &= modem.loadCertificate(client_key_name, client_key,
                                                 strlen(client_key));
    // print out the certificate to make sure it matches
    modem.printCertificate(client_key_name, SerialMon);
    if (!client_cert_success) {
        SerialMon.println(" ...failed to load client certificate or key!");
        delay(10000);
        return;
    }
    SerialMon.println(" ...success");
    // convert the client certificate pair to the modem's format
    client_cert_success &= modem.convertClientCertificates(client_cert_name,
                                                           client_key_name);
    if (!client_cert_success) {
        SerialMon.println(" ...failed to convert client certificate and key!");
        delay(10000);
        return;
    }
    SerialMon.println(" ...success");
    delay(1000);

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

    // =================== WAIT FOR NETWORK REGISTRATION ===================
    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork(300000L)) {
        SerialMon.println(" ...failed to connect to network!");
        delay(10000);
        return;
    }
    SerialMon.println(" ...success");

    if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

    // ====================== MAKE DATA CONNECTION =======================
    // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.println(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(" ...failed to connect to GPRS!");
        delay(10000);
        return;
    }
    SerialMon.println(" ...success");

    if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }

    // enable/force time sync with NTP server
    // This is **REQUIRED** for validated SSL connections
    DBG("Enabling time sync with NTP server");
    modem.NTPServerSync("pool.ntp.org", -5);

    String time = modem.getGSMDateTime(TinyGSMDateTimeFormat::DATE_FULL);
    DBG("Current Network Time:", time);

    // MQTT Broker setup
    mqtt.setServer(broker, port);

    delay(500);
    DBG("Finished setup");
}

void loop() {
    // Make sure we're still registered on the network
    if (!modem.isNetworkConnected()) {
        SerialMon.println("Network disconnected");
        if (!modem.waitForNetwork(180000L, true)) {
            SerialMon.println(" ...failed to reconnect to network!");
            delay(10000);
            return;
        }
        if (modem.isNetworkConnected()) {
            SerialMon.println("Network re-connected");
        }

        // and make sure GPRS/EPS is still connected
        if (!modem.isGprsConnected()) {
            SerialMon.println("GPRS disconnected!");
            SerialMon.print(F("Connecting to "));
            SerialMon.println(apn);
            if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
                SerialMon.println(" ...failed to reconnect to GPRS!");
                delay(10000);
                return;
            }
            if (modem.isGprsConnected()) {
                SerialMon.println("GPRS reconnected");
            }
        }
    }

    if (!mqtt.connected()) {
        SerialMon.println("=== MQTT NOT CONNECTED ===");
        // Reconnect every 10 seconds
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 30000L) {
            lastReconnectAttempt = t;
            if (mqttConnect()) { lastReconnectAttempt = 0; }
        }
        delay(5000L);
        return;
    }

    mqtt.loop();
}
