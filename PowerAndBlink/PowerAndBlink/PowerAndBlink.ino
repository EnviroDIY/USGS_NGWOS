#include <Arduino.h>

void blinkLEDS(uint8_t pin, int8_t n_blinks = 5, uint32_t blink_pause = 200) {
    pinMode(pin, OUTPUT);
    for (uint8_t i = n_blinks; i; i--) {
        digitalWrite(pin, HIGH);
        delay(blink_pause);
        digitalWrite(pin, LOW);
        delay(blink_pause);
    }
}

// put your setup code here, to run once:
void setup() {
// Wait for USB connection to be established by PC
// NOTE:  Only use this when debugging - if not connected to a PC, this
// could prevent the script from starting
#if defined SERIAL_PORT_USBVIRTUAL
    while (!SERIAL_PORT_USBVIRTUAL && (millis() < 5000L)) {
        // wait
    }
#endif
    Serial.begin(115200);
    Serial.println("Welcome to the EnviroDIY Stonefly testing program!");

    Serial.println("Holding the SDI-12 data line LOW with pin 3 so the device "
                   "can connect to the Vega Tools App");
    pinMode(3, OUTPUT);
    digitalWrite(3, LOW);
    Serial.println("Holding with power off for 30 seconds");
    pinMode(22, OUTPUT);
    digitalWrite(22, LOW);
    for (int8_t i = 30; i; i--) {
        Serial.print(". ");
        delay(1000);
    }
    Serial.println();

    Serial.println("Powering the VegaPuls with pin 22");
    Serial.println("The VegaPuls will remain powered until the board is turned "
                   "off or reset");
    pinMode(22, OUTPUT);
    digitalWrite(22, HIGH);
}

// put your main code here, to run repeatedly:
void loop() {
    Serial.println("Now blinking the Green LED");
    blinkLEDS(8);
    Serial.println("Now blinking the Red LED");
    blinkLEDS(9);
    Serial.println("Now blinking the Orange LED");
    blinkLEDS(13);
    Serial.println("Now blinking the Green SD Card LED");
    blinkLEDS(30);
    Serial.println("Now blinking the Red SD Card LED");
    blinkLEDS(31);
}
