/**
 * @file LoggerBase.cpp
 * @copyright Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino.
 * This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Implements the Logger class.
 */

#include "LoggerBase.h"
// For all i2c communication, including with the real time clock
#include <Wire.h>

#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR)) && \
    not defined(SDI12_INTERNAL_PCINT)
// Unless we're forcing use of internal interrupts, use EnableInterrupt for AVR
// boards
/// Required define for Enable Interrupts prevent compiler/linker crashes
#define LIBCALL_ENABLEINTERRUPT
// To handle external and pin change interrupts
#include <EnableInterrupt.h>

#elif not defined(__AVR__) && not defined(ARDUINO_ARCH_AVR)
// For compatibility with non AVR boards, we need these macros
#define enableInterrupt(pin, userFunc, mode) \
    attachInterrupt(digitalPinToInterrupt(pin), userFunc, mode)
#define disableInterrupt(pin) detachInterrupt(digitalPinToInterrupt(pin))
#endif


// Initialize the static timezone
int8_t Logger::_loggerTimeZone = 0;
// Initialize the static time adjustment
int8_t Logger::_loggerRTCOffset = 0;
// Initialize the static timestamps
uint32_t Logger::markedLocalEpochTime = 0;
uint32_t Logger::markedUTCEpochTime   = 0;
// Initialize the testing/logging flags
volatile bool Logger::isLoggingNow = false;
volatile bool Logger::isTestingNow = false;
volatile bool Logger::startTesting = false;

// Initialize the RTC
RV8803 Logger::rtc;

// Constructors
Logger::Logger(const char* loggerID, uint16_t loggingIntervalMinutes) {
    // Set parameters from constructor
    setLoggerID(loggerID);
    setLoggingInterval(loggingIntervalMinutes);

    // Set the testing/logging flags to false
    isLoggingNow = false;
    isTestingNow = false;
    startTesting = false;
}
// Destructor
Logger::~Logger() {}


// ===================================================================== //
// Public functions to get and set basic logging paramters
// ===================================================================== //

// Sets the logger ID
void Logger::setLoggerID(const char* loggerID) {
    _loggerID = loggerID;
}

// Sets/Gets the logging interval
void Logger::setLoggingInterval(uint16_t loggingIntervalMinutes) {
    _loggingIntervalMinutes = loggingIntervalMinutes;
}


// Adds the sampling feature UUID
void Logger::setSamplingFeatureUUID(const char* samplingFeatureUUID) {
    _samplingFeatureUUID = samplingFeatureUUID;
}

// Sets up a pin controlling the power to the SD card
void Logger::setSDCardPwr(int8_t SDCardPowerPin) {
    _SDCardPowerPin = SDCardPowerPin;
    if (_SDCardPowerPin >= 0) {
        pinMode(_SDCardPowerPin, OUTPUT);
        digitalWrite(_SDCardPowerPin, LOW);
        MS_DBG(F("Pin"), _SDCardPowerPin, F("set as SD Card Power Pin"));
    }
}
// NOTE:  Structure of power switching on SD card taken from:
// https://thecavepearlproject.org/2017/05/21/switching-off-sd-cards-for-low-power-data-logging/
void Logger::turnOnSDcard(bool waitToSettle) {
    if (_SDCardPowerPin >= 0) {
        digitalWrite(_SDCardPowerPin, HIGH);
        // TODO(SRGDamia1):  figure out how long to wait
        if (waitToSettle) { delay(6); }
    }
}
void Logger::turnOffSDcard(bool waitForHousekeeping) {
    if (_SDCardPowerPin >= 0) {
        // TODO(SRGDamia1): set All SPI pins to INPUT?
        // TODO(SRGDamia1): set ALL SPI pins HIGH (~30k pull-up)
        pinMode(_SDCardPowerPin, OUTPUT);
        digitalWrite(_SDCardPowerPin, LOW);
        // TODO(SRGDamia1):  wait in lower power mode
        if (waitForHousekeeping) {
            // Specs say up to 1s for internal housekeeping after each write
            delay(1000);
        }
    }
}


// Sets up a pin for the slave select (chip select) of the SD card
void Logger::setSDCardSS(int8_t SDCardSSPin) {
    _SDCardSSPin = SDCardSSPin;
    if (_SDCardSSPin >= 0) {
        pinMode(_SDCardSSPin, OUTPUT);
        MS_DBG(F("Pin"), _SDCardSSPin, F("set as SD Card Slave/Chip Select"));
    }
}


// Sets both pins related to the SD card
void Logger::setSDCardPins(int8_t SDCardSSPin, int8_t SDCardPowerPin) {
    setSDCardPwr(SDCardPowerPin);
    setSDCardSS(SDCardSSPin);
}


// Sets up the wake up pin for an RTC interrupt
// NOTE:  This sets the pin mode but does NOT enable the interrupt!
void Logger::setRTCWakePin(int8_t mcuWakePin, uint8_t wakePinMode) {
    _mcuWakePin = mcuWakePin;
    if (_mcuWakePin >= 0) {
        pinMode(_mcuWakePin, wakePinMode);
        MS_DBG(F("Pin"), _mcuWakePin, F("set as RTC wake up pin"));
    } else {
        MS_DBG(F("Logger mcu will not sleep between readings!"));
    }
}


// Sets up a pin for an LED or other way of alerting that data is being logged
void Logger::setAlertPin(int8_t ledPin) {
    _ledPin = ledPin;
    if (_ledPin >= 0) {
        pinMode(_ledPin, OUTPUT);
        MS_DBG(F("Pin"), _ledPin, F("set as LED alert pin"));
    }
}
void Logger::alertOn() {
    if (_ledPin >= 0) { digitalWrite(_ledPin, HIGH); }
}
void Logger::alertOff() {
    if (_ledPin >= 0) { digitalWrite(_ledPin, LOW); }
}


// Sets up the five pins of interest for the logger
void Logger::setLoggerPins(int8_t mcuWakePin, int8_t SDCardSSPin,
                           int8_t SDCardPowerPin, int8_t ledPin,
                           uint8_t wakePinMode) {
    setRTCWakePin(mcuWakePin, wakePinMode);
    setSDCardSS(SDCardSSPin);
    setSDCardPwr(SDCardPowerPin);
    setAlertPin(ledPin);
}

// ===================================================================== //
// Public functions to access the clock in proper format and time zone
// ===================================================================== //

// Sets the static timezone that the data will be logged in - this must be set
void Logger::setLoggerTimeZone(int8_t timeZone) {
    _loggerTimeZone = timeZone;
    // void setTimeZoneQuarterHours(int8_t quarterHours);
    // Write the time zone to RV8803_RAM as int8_t (signed) in 15 minute
    // increments
    rtc.setTimeZoneQuarterHours(timeZone * 4);
// Some helpful prints for debugging
#ifdef STANDARD_SERIAL_OUTPUT
    const char* prtout1 = "Logger timezone is set to UTC";
    if (_loggerTimeZone == 0) {
        PRINTOUT(prtout1);
    } else if (_loggerTimeZone > 0) {
        PRINTOUT(prtout1, '+', _loggerTimeZone);
    } else {
        PRINTOUT(prtout1, _loggerTimeZone);
    }
#endif
}
int8_t Logger::getLoggerTimeZone(void) {
    return Logger::_loggerTimeZone;
}
// Duplicates for backwards compatibility
void Logger::setTimeZone(int8_t timeZone) {
    setLoggerTimeZone(timeZone);
}
int8_t Logger::getTimeZone(void) {
    return getLoggerTimeZone();
}

// Sets the static timezone that the RTC is programmed in
// I VERY VERY STRONGLY RECOMMEND SETTING THE RTC IN UTC
// You can either set the RTC offset directly or set the offset between the
// RTC and the logger
void Logger::setRTCTimeZone(int8_t timeZone) {
    _loggerRTCOffset = _loggerTimeZone - timeZone;
// Some helpful prints for debugging
#ifdef STANDARD_SERIAL_OUTPUT
    const char* prtout1 = "RTC timezone is set to UTC";
    if ((_loggerTimeZone - _loggerRTCOffset) == 0) {
        PRINTOUT(prtout1);
    } else if ((_loggerTimeZone - _loggerRTCOffset) > 0) {
        PRINTOUT(prtout1, '+', (_loggerTimeZone - _loggerRTCOffset));
    } else {
        PRINTOUT(prtout1, (_loggerTimeZone - _loggerRTCOffset));
    }
#endif
}
int8_t Logger::getRTCTimeZone(void) {
    return Logger::_loggerTimeZone - Logger::_loggerRTCOffset;
}


// This set the offset between the built-in clock and the time zone where
// the data is being recorded.  If your RTC is set in UTC and your logging
// timezone is EST, this should be -5.  If your RTC is set in EST and your
// timezone is EST this does not need to be called.
// You can either set the RTC offset directly or set the offset between the
// RTC and the logger
void Logger::setTZOffset(int8_t offset) {
    _loggerRTCOffset = offset;
    // Some helpful prints for debugging
    if (_loggerRTCOffset == 0) {
        PRINTOUT(F("RTC and Logger are set in the same timezone."));
    } else if (_loggerRTCOffset < 0) {
        PRINTOUT(F("RTC is set"), -1 * _loggerRTCOffset,
                 F("hours ahead of logging timezone"));
    } else {
        PRINTOUT(F("RTC is set"), _loggerRTCOffset,
                 F("hours behind the logging timezone"));
    }
}
int8_t Logger::getTZOffset(void) {
    return Logger::_loggerRTCOffset;
}

// This gets the current epoch time (unix time, ie, the number of seconds
// from January 1, 1970 00:00:00 UTC) and corrects it to the specified time zone

uint32_t Logger::getNowEpoch(void) {
    // Depreciated in 0.33.0, left in for compatiblity
    return getNowLocalEpoch();
}
uint32_t Logger::getNowLocalEpoch(void) {
    // uint32_t getLocalEpoch(bool use1970sEpoch = false);
    // Get the local epoch - without subtracting the time zone
    rtc.updateTime();
    return rtc.getLocalEpoch();
}

uint32_t Logger::getNowUTCEpoch(void) {
    // uint32_t getEpoch(bool use1970sEpoch = false);
    // Get the epoch - with the time zone subtracted (i.e. return UTC epoch)
    rtc.updateTime();
    return rtc.getEpoch();
}
void Logger::setNowUTCEpoch(uint32_t ts) {
    // bool setEpoch(uint32_t value, bool use1970sEpoch = false, int8_t
    // timeZoneQuarterHours = 0);
    // If timeZoneQuarterHours is non-zero, update RV8803_RAM. Add the zone to
    // the epoch before setting
    rtc.setEpoch(ts);
}

// This converts an epoch time (unix time) into a ISO8601 formatted string.
// It assumes the supplied date/time is in the LOGGER's timezone and adds the
// LOGGER's offset as the time zone offset in the string.
String Logger::formatDateTime_ISO8601(uint32_t epochTime) {
    // char* stringTime8601TZ();
    // Return time in ISO 8601 format yyyy-mm-ddThh:mm:ss+/-hh:mm
    rtc.updateTime();
    return String(rtc.stringTime8601TZ());
}


// This sets the real time clock to the given time
bool Logger::setRTClock(uint32_t UTCEpochSeconds) {
    // If the timestamp is zero, just exit
    if (UTCEpochSeconds == 0) {
        PRINTOUT(F("Bad timestamp, not setting clock."));
        return false;
    }

    // The "setTime" is the number of seconds since Jan 1, 1970 in UTC
    // We're interested in the setTime in the logger's and RTC's timezone
    // The RTC's timezone is equal to the logger's timezone minus the offset
    // between the logger and the RTC.
    uint32_t set_rtcTZ = UTCEpochSeconds;
    // NOTE:  We're only looking at local time here in order to print it out for
    // the user
    uint32_t set_logTZ = UTCEpochSeconds +
        ((uint32_t)getLoggerTimeZone()) * 3600;
    MS_DBG(F("    Time for Logger supplied as input:"), set_logTZ, F("->"),
           formatDateTime_ISO8601(set_logTZ));

    // Check the current RTC time
    uint32_t cur_logTZ = getNowLocalEpoch();
    MS_DBG(F("    Current Time on RTC:"), cur_logTZ, F("->"),
           formatDateTime_ISO8601(cur_logTZ));
    MS_DBG(F("    Offset between input and RTC:"), abs(set_logTZ - cur_logTZ));

    // NOTE:  Because we take the time to do some UTC/Local conversions and
    // print stuff out, the clock might end up being set up to a few
    // milliseconds behind the input time.  Given the clock is only accurate to
    // seconds (not milliseconds or less), I don't think this is a problem.

    // If the RTC and NIST disagree by more than 5 seconds, set the clock
    if (abs(set_logTZ - cur_logTZ) > 5) {
        setNowUTCEpoch(set_rtcTZ);
        PRINTOUT(F("Clock set!"));
        return true;
    } else {
        PRINTOUT(F("Clock already within 5 seconds of time."));
        return false;
    }
}

// This checks that the logger time is within a "sane" range
bool Logger::isRTCSane(void) {
    uint32_t curRTC = getNowLocalEpoch();
    return isRTCSane(curRTC);
}
bool Logger::isRTCSane(uint32_t epochTime) {
    // Before January 1, 2020 or After January 1, 2030
    if (epochTime < 1577836800 || epochTime > 1893474000) {
        return false;
    } else {
        return true;
    }
}


// This sets static variables for the date/time - this is needed so that all
// data outputs (SD, EnviroDIY, serial printing, etc) print the same time
// for updating the sensors - even though the routines to update the sensors
// and to output the data may take several seconds.
// It is not currently possible to output the instantaneous time an individual
// sensor was updated, just a single marked time.  By custom, this should be
// called before updating the sensors, not after.
void Logger::markTime(void) {
    Logger::markedUTCEpochTime   = getNowUTCEpoch();
    Logger::markedLocalEpochTime = markedUTCEpochTime +
        ((uint32_t)_loggerRTCOffset) * 3600;
}


// This checks to see if the CURRENT time is an even interval of the logging
// rate
bool Logger::checkInterval(void) {
    bool     retval;
    uint32_t checkTime = getNowLocalEpoch();
    MS_DBG(F("Current Unix Timestamp:"), checkTime, F("->"),
           formatDateTime_ISO8601(checkTime));
    MS_DBG(F("Logging interval in seconds:"), (_loggingIntervalMinutes * 60));
    MS_DBG(F("Mod of Logging Interval:"),
           checkTime % (_loggingIntervalMinutes * 60));

    if (checkTime % (_loggingIntervalMinutes * 60) == 0) {
        // Update the time variables with the current time
        markTime();
        MS_DBG(F("Time marked at (unix):"), Logger::markedLocalEpochTime);
        MS_DBG(F("Time to log!"));
        retval = true;
    } else {
        MS_DBG(F("Not time yet."));
        retval = false;
    }
    if (!isRTCSane(checkTime)) {
        PRINTOUT(F("----- WARNING ----- !!!!!!!!!!!!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!! ----- WARNING ----- !!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!!!!!!!!!!!! ----- WARNING ----- "));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(' ');
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("The current clock timestamp is not valid!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(' ');
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("----- WARNING ----- !!!!!!!!!!!!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!! ----- WARNING ----- !!!!!!!!!!"));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
        PRINTOUT(F("!!!!!!!!!!!!!!!!!!!! ----- WARNING ----- "));
        alertOn();
        delay(25);
        alertOff();
        delay(25);
    }
    return retval;
}


// This checks to see if the MARKED time is an even interval of the logging rate
bool Logger::checkMarkedInterval(void) {
    bool retval;
    MS_DBG(F("Marked Time:"), Logger::markedLocalEpochTime,
           F("Logging interval in seconds:"), (_loggingIntervalMinutes * 60),
           F("Mod of Logging Interval:"),
           Logger::markedLocalEpochTime % (_loggingIntervalMinutes * 60));

    if (Logger::markedLocalEpochTime != 0 &&
        (Logger::markedLocalEpochTime % (_loggingIntervalMinutes * 60) == 0)) {
        MS_DBG(F("Time to log!"));
        retval = true;
    } else {
        MS_DBG(F("Not time yet."));
        retval = false;
    }
    return retval;
}


// ============================================================================
//  Public Functions for sleeping the logger
// ============================================================================

// Set up the Interrupt Service Request for waking
// In this case, we're doing nothing, we just want the processor to wake
// This must be a static function (which means it can only call other static
// funcions.)
void Logger::wakeISR(void) {
    MS_DEEP_DBG(F("\nClock interrupt!"));
}


// Puts the system to sleep to conserve battery life.
// This DOES NOT sleep or wake the sensors!!
void Logger::systemSleep(void) {
    // Don't go to sleep unless there's a wake pin!
    if (_mcuWakePin < 0) {
        MS_DBG(F("Use a non-negative wake pin to request sleep!"));
        return;
    }

    // Disable any previous interrupts
    rtc.disableAllInterrupts();
    // Clear all flags in case any interrupts have occurred.
    rtc.clearAllInterruptFlags();
    // Enable a periodic update for every minute
    rtc.setPeriodicTimeUpdateFrequency(TIME_UPDATE_1_MINUTE);
    // Enable the hardware interrupt
    rtc.enableHardwareInterrupt(UPDATE_INTERRUPT);

    // Set up a pin to hear clock interrupt and attach the wake ISR to it
    pinMode(_mcuWakePin, INPUT_PULLUP);
    enableInterrupt(_mcuWakePin, wakeISR, CHANGE);

    // Send one last message before shutting down serial ports
    MS_DBG(F("Putting processor to sleep.  ZZzzz..."));

// Wait until the serial ports have finished transmitting
// This does not clear their buffers, it just waits until they are finished
// TODO(SRGDamia1):  Make sure can find all serial ports
#if defined(STANDARD_SERIAL_OUTPUT)
    STANDARD_SERIAL_OUTPUT.flush();  // for debugging
#endif
#if defined DEBUGGING_SERIAL_OUTPUT
    DEBUGGING_SERIAL_OUTPUT.flush();  // for debugging
#endif

    // Stop any I2C connections
    // This function actually disables the two-wire pin functionality and
    // turns off the internal pull-up resistors.
    Wire.end();
// Now force the I2C pins to LOW
// I2C devices have a nasty habit of stealing power from the SCL and SDA pins...
// This will only work for the "main" I2C/TWI interface
#ifdef SDA
    pinMode(SDA, OUTPUT);
    digitalWrite(SDA, LOW);
#endif
#ifdef SCL
    pinMode(SCL, OUTPUT);
    digitalWrite(SCL, LOW);
#endif

    // Disable the watch-dog timer
    watchDogTimer.disableWatchDog();

#ifndef USE_TINYUSB
    // Detach the USB, iff not using TinyUSB
    MS_DEEP_DBG(F("USBDevice.detach"));
    USBDevice.detach();
    MS_DEEP_DBG(F("USBDevice.end"));
    USBDevice.end();
    USBDevice.standby();
#endif

#if defined(__SAMD51__)
    // PM_SLEEPCFG_SLEEPMODE_BACKUP = 0x4
    PM->SLEEPCFG.bit.SLEEPMODE = 0x4;
    while (PM->SLEEPCFG.bit.SLEEPMODE != 0x4)
        ;  // Wait for it to take
#else
    // Disable systick interrupt:  See
    // https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode
    // Due to a hardware bug on the SAMD21, the SysTick interrupts become active
    // before the flash has powered up from sleep, causing a hard fault. To
    // prevent this the SysTick interrupts are disabled before entering sleep
    // mode.
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    // Now go to sleep
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#endif

    __DSB();  // Data sync to ensure outgoing memory accesses complete
    __WFI();  // Wait for interrupt (places device in sleep mode)

    // ---------------------------------------------------------------------


    // ---------------------------------------------------------------------
    // -- The portion below this happens on wake up, after any wake ISR's --

#if (SAMD20_SERIES || SAMD21_SERIES)
    // Reattach the USB after waking
    // Enable systick interrupt
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
#endif
    // Reattach the USB
#ifndef USE_TINYUSB
    USBDevice.init();
    USBDevice.attach();
#endif

    // Re-enable the watch-dog timer
    watchDogTimer.enableWatchDog();

// Re-start the I2C interface
#ifdef SDA
    pinMode(SDA, INPUT_PULLUP);  // set as input with the pull-up on
#endif
#ifdef SCL
    pinMode(SCL, INPUT_PULLUP);
#endif
    Wire.begin();
    // Eliminate any potential extra waits in the wire library
    // These waits would be caused by a readBytes or parseX being called
    // on wire after the Wire buffer has emptied.  The default stream
    // functions - used by wire - wait a timeout period after reading the
    // end of the buffer to see if an interrupt puts something into the
    // buffer.  In the case of the Wire library, that will never happen and
    // the timeout period is a useless delay.
    Wire.setTimeout(0);

    // Stop the clock from sending out any interrupts while we're awake.
    // There's no reason to waste thought on the clock interrupt if it
    // happens while the processor is awake and doing other things.
    rtc.disableHardwareInterrupt(UPDATE_INTERRUPT);
    // Detach the from the pin
    disableInterrupt(_mcuWakePin);

    // Wake-up message
    MS_DBG(F("\n\n\n... zzzZZ Processor is now awake!"));

    // The logger will now start the next function after the systemSleep
    // function in either the loop or setup
}


// ===================================================================== //
// Public functions for logging data to an SD card
// ===================================================================== //

// This sets a file name, if you want to decide on it in advance
void Logger::setFileName(String& fileName) {
    _fileName = fileName;
}
// Same as above, with a character array (overload function)
void Logger::setFileName(const char* fileName) {
    auto StrName = String(fileName);
    setFileName(StrName);
}


// This generates a file name from the logger id and the current date
// This will be used if the setFileName function is not called before
// the begin() function is called.
void Logger::generateAutoFileName(void) {
    // Generate the file name from logger ID and date
    auto fileName = String(_loggerID);
    fileName += "_";
    fileName += formatDateTime_ISO8601(getNowLocalEpoch()).substring(0, 10);
    fileName += ".csv";
    setFileName(fileName);
    _fileName = fileName;
}

// Protected helper function - This checks if the SD card is available and ready
bool Logger::initializeSDCard(void) {
    // If we don't know the slave select of the sd card, we can't use it
    if (_SDCardSSPin < 0) {
        PRINTOUT(F("Slave/Chip select pin for SD card has not been set."));
        PRINTOUT(F("Data will not be saved!"));
        return false;
    }
    // Initialise the SD card
    if (!sd.begin(_SDCardSSPin, SPI_FULL_SPEED)) {
        PRINTOUT(F("Error: SD card failed to initialize or is missing."));
        PRINTOUT(F("Data will not be saved!"));
        return false;
    } else {
        // skip everything else if there's no SD card, otherwise it mighthang
        MS_DBG(F("Successfully connected to SD Card with card/slave select on "
                 "pin"),
               _SDCardSSPin);
        return true;
    }
}


// Protected helper function - This sets a timestamp on a file
void Logger::setFileTimestamp(File& fileToStamp, uint8_t stampFlag) {
    rtc.updateTime();
    fileToStamp.timestamp(stampFlag, rtc.getYear(), rtc.getMonth(),
                          rtc.getDate(), rtc.getHours(), rtc.getMinutes(),
                          rtc.getSeconds());
}


// Protected helper function - This opens or creates a file, converting a string
// file name to a character file name
bool Logger::openFile(String& filename, bool createFile) {
    // Initialise the SD card
    // skip everything else if there's no SD card, otherwise it might hang
    if (!initializeSDCard()) return false;

    // Convert the string filename to a character file name for SdFat
    unsigned int fileNameLength = filename.length() + 1;
    char         charFileName[fileNameLength];
    filename.toCharArray(charFileName, fileNameLength);

    // First attempt to open an already existing file (in write mode), so we
    // don't try to re-create something that's already there.
    // This should also prevent the header from being written over and over
    // in the file.
    if (logFile.open(charFileName, O_WRITE | O_AT_END)) {
        MS_DBG(F("Opened existing file:"), filename);
        // Set access date time
        setFileTimestamp(logFile, T_ACCESS);
        return true;
    } else if (createFile) {
        // Create and then open the file in write mode
        if (logFile.open(charFileName, O_CREAT | O_WRITE | O_AT_END)) {
            MS_DBG(F("Created new file:"), filename);
            // Set creation date time
            setFileTimestamp(logFile, T_CREATE);
            // Set access date time
            setFileTimestamp(logFile, T_ACCESS);
            return true;
        } else {
            // Return false if we couldn't create the file
            MS_DBG(F("Unable to create new file:"), filename);
            return false;
        }
    } else {
        // Return false if we couldn't access the file (and were not told to
        // create it)
        MS_DBG(F("Unable to to write to file:"), filename);
        return false;
    }
}


// These functions create a file on the SD card with the given filename and
// set the proper timestamps to the file.
// The filename may either be the one set by
// setFileName(String)/setFileName(void) or can be specified in the function. If
// specified, it will also write a header to the file based on the sensors in
// the group. This can be used to force a logger to create a file with a
// secondary file name.
bool Logger::createLogFile(String& filename) {
    // Attempt to create and open a file
    if (openFile(filename, true)) {
        // Close the file to save it (only do this if we'd opened it)
        logFile.close();
        PRINTOUT(F("Data will be saved as"), _fileName);
        return true;
    } else {
        PRINTOUT(F("Unable to create a file to save data to!"));
        return false;
    }
}
bool Logger::createLogFile() {
    if (_fileName == "") generateAutoFileName();
    return createLogFile(_fileName);
}


// These functions write a file on the SD card with the given filename and
// set the proper timestamps to the file.
// The filename may either be the one set by
// setFileName(String)/setFileName(void) or can be specified in the function. If
// the file does not already exist, the file will be created. This can be used
// to force a logger to write to a file with a secondary file name.
bool Logger::logToSD(String& filename, String& rec) {
    // First attempt to open the file without creating a new one
    if (!openFile(filename, false)) {
        PRINTOUT(F("Could not write to existing file on SD card, attempting to "
                   "create a file!"));
        // Next try to create the file, bail if we couldn't create it
        // This will not attempt to generate a new file name or add a header!
        if (!openFile(filename, true)) {
            PRINTOUT(F("Unable to write to SD card!"));
            return false;
        }
    }

    // If we could successfully open or create the file, write the data to it
    logFile.println(rec);
    // Echo the line to the serial port
    PRINTOUT(F("\n \\/---- Line Saved to SD Card ----\\/"));
    PRINTOUT(rec);

    // Set write/modification date time
    setFileTimestamp(logFile, T_WRITE);
    // Set access date time
    setFileTimestamp(logFile, T_ACCESS);
    // Close the file to save it
    logFile.close();
    return true;
}
bool Logger::logToSD(String& rec) {
    // Get a new file name if the name is blank
    if (_fileName == "") generateAutoFileName();
    return logToSD(_fileName, rec);
}

// ===================================================================== //
// Convience functions to call several of the above functions
// ===================================================================== //
void Logger::begin() {
    MS_DBG(F("Logger ID is:"), _loggerID);
    MS_DBG(F("Logger is set to record at"), _loggingIntervalMinutes,
           F("minute intervals."));

#if defined(ARDUINO_ARCH_SAMD)
    MS_DBG(F("Disabling the USB on stnadby to lower sleep current"));
    USB->DEVICE.CTRLA.bit.ENABLE = 0;  // Disable the USB peripheral
    while (USB->DEVICE.SYNCBUSY.bit.ENABLE)
        ;                                // Wait for synchronization
    USB->DEVICE.CTRLA.bit.RUNSTDBY = 0;  // Deactivate run on standby
    USB->DEVICE.CTRLA.bit.ENABLE   = 1;  // Enable the USB peripheral
    while (USB->DEVICE.SYNCBUSY.bit.ENABLE)
        ;  // Wait for synchronization
#endif

    MS_DBG(F(
        "Setting up a watch-dog timer to fire after 15 minutes of inactivity"));
    watchDogTimer.setupWatchDog((uint32_t)(5 * 60 * 3));
    // Enable the watchdog
    watchDogTimer.enableWatchDog();
    watchDogTimer.resetWatchDog();

    // Set the pins for I2C
    MS_DBG(F("Setting I2C Pins to INPUT_PULLUP"));
#ifdef SDA
    pinMode(SDA, INPUT_PULLUP);  // set as input with the pull-up on
#endif
#ifdef SCL
    pinMode(SCL, INPUT_PULLUP);
#endif
    MS_DBG(F("Beginning wire (I2C)"));
    Wire.begin();
    watchDogTimer.resetWatchDog();

    // Eliminate any potential extra waits in the wire library
    // These waits would be caused by a readBytes or parseX being called
    // on wire after the Wire buffer has emptied.  The default stream
    // functions - used by wire - wait a timeout period after reading the
    // end of the buffer to see if an interrupt puts something into the
    // buffer.  In the case of the Wire library, that will never happen and
    // the timeout period is a useless delay.
    Wire.setTimeout(0);

    // Set all of the pin modes
    // NOTE:  This must be done here at run time not at compile time
    setLoggerPins(_mcuWakePin, _SDCardSSPin, _SDCardPowerPin, _ledPin);

    MS_DBG(F("Beginning RV-8803 real time clock"));
    rtc.begin();
    rtc.set24Hour();

    watchDogTimer.resetWatchDog();

    // Print out the current time
    PRINTOUT(F("Current RTC time is:"),
             formatDateTime_ISO8601(getNowUTCEpoch()));
    PRINTOUT(F("Current localized logger time is:"),
             formatDateTime_ISO8601(getNowLocalEpoch()));

    // Reset the watchdog
    watchDogTimer.resetWatchDog();

    if (_samplingFeatureUUID != nullptr) {
        PRINTOUT(F("Sampling feature UUID is:"), _samplingFeatureUUID);
    }

    PRINTOUT(F("Logger portion of setup finished.\n"));
}
