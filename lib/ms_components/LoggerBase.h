/**
 * @file LoggerBase.h
 * @copyright Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino.
 * This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Contains the LoggerBase class which handles basic logging functions.
 *
 * Basic logger functions include communicating with a real-time clock, saving
 * to an SD card, and publishing data to remotes via a loggerModem connection.
 */

// Header Guards
#ifndef SRC_LOGGERBASE_H_
#define SRC_LOGGERBASE_H_

// Debugging Statement
// #define MS_LOGGERBASE_DEBUG

#ifdef MS_LOGGERBASE_DEBUG
#define MS_DEBUGGING_STD "LoggerBase"
#endif

#ifdef MS_LOGGERBASE_DEBUG_DEEP
#define MS_DEBUGGING_DEEP "LoggerBase"
#endif

// Included Dependencies
#include "ModSensorDebugger.h"
#undef MS_DEBUGGING_STD
#undef MS_DEBUGGING_DEEP


// Bring in the libraries to handle the processor sleep/standby modes
// The SAMD library can also the built-in clock on those modules
#include "WatchDogSAMD.h"

// Bring in the library to communicate with an external high-precision real time
// clock.
#include <SparkFun_RV8803.h>

#ifndef EPOCH_TIME_OFF
/**
 * @brief January 1, 2000 00:00:00 in "epoch" time
 *
 * Need this b/c the date/time class in Sodaq_DS3231 treats a 32-bit long
 * timestamp as time from 2000-jan-01 00:00:00 instead of the standard (unix)
 * epoch beginning 1970-jan-01 00:00:00.
 */
#define EPOCH_TIME_OFF 946684800
#endif

#include <SdFat.h>  // To communicate with the SD card


/**
 * @brief The "Logger" Class handles low power sleep for the main processor,
 * interfacing with the real-time clock and modem, writing to the SD card, and
 * passing data on to the data publishers.
 *
 * A logger is a device that can control all functions of the modem sensors and
 * that are attached to it and save the values of all variables measured by
 * those sensors to an attached SD card.  It must also work with a real-time
 * clock to give timestamps to values.  It may also work with a loggerModem for
 * internet connection and send out data over the internet through one or more
 * data publishers.
 *
 * In this library, all loggers are Arduino-style small processor circuit
 * boards.
 *
 * @ingroup base_classes
 */
class Logger {

 public:
    /**
     * @brief Construct a new Logger object.
     *
     * @param loggerID A name for the logger - unless otherwise specified, files
     * saved to the SD card will be named with the logger id and the date the
     * file was started.
     * @param loggingIntervalMinutes The frequency in minutes at which data
     * should be logged
     * @param inputArray A variableArray object instance providing data to be
     * logged.  This is NOT an array of variables, but an object of the variable
     * array class.
     */
    Logger(const char* loggerID, uint16_t loggingIntervalMinutes);
    /**
     * @brief Destroy the Logger object - takes no action.
     */
    virtual ~Logger();

    // ===================================================================== //
    /**
     * @anchor logger_basic_parameters
     * @name Basic Logging Parameters
     * Public functions to get and set basic logging paramters
     */
    /**@{*/
    // ===================================================================== //
    /**
     * @brief Set the Logger ID.
     *
     * Unless otherwise specified, files saved to the SD card will be named with
     * the logger id and the date the file was started.
     *
     * @param loggerID A pointer to the logger ID
     */
    void setLoggerID(const char* loggerID);
    /**
     * @brief Get the Logger ID.
     *
     * @return **const char\*** A pointer to the logger ID
     */
    const char* getLoggerID() {
        return _loggerID;
    }

    /**
     * @brief Set the logging interval in minutes.
     *
     * @param loggingIntervalMinutes The frequency with which to update sensor
     * values and write data to the SD card.
     */
    void setLoggingInterval(uint16_t loggingIntervalMinutes);
    /**
     * @brief Get the Logging Interval.
     *
     * @return **uint16_t** The logging interval in minutes
     */
    uint16_t getLoggingInterval() {
        return _loggingIntervalMinutes;
    }

    /**
     * @brief Set the universally unique identifier (UUID or GUID) of the
     * sampling feature.
     *
     * @param samplingFeatureUUID A pointer to the sampling feature UUID
     */
    void setSamplingFeatureUUID(const char* samplingFeatureUUID);
    /**
     * @brief Get the Sampling Feature UUID.
     *
     * @return **const char\*** The sampling feature UUID
     */
    const char* getSamplingFeatureUUID() {
        return _samplingFeatureUUID;
    }

    /**
     * @brief Set a digital pin number (on the mcu) to use to control power to
     * the SD card and activate it as an output pin.
     *
     * Because this sets the pin mode, this function should only be called
     * during the `setup()` or `loop()` portion of an Arduino program.
     *
     * @warning This functionality is not tested!
     *
     * @param SDCardPowerPin A digital pin number on the mcu controlling power
     * to the SD card.
     */
    void setSDCardPwr(int8_t SDCardPowerPin);
    /**
     * @brief Send power to the SD card by setting the SDCardPowerPin `HIGH`.
     *
     * Optionally waits for the card to "settle."  Has no effect if a pin has
     * not been set to control power to the SD card.
     *
     * @param waitToSettle True to add a short (6ms) delay between powering on
     * the card and beginning initialization.  Defaults to true.
     */
    void turnOnSDcard(bool waitToSettle = true);
    /**
     * @brief Cut power to the SD card by setting the SDCardPowerPin `LOW`.
     *
     * Optionally waits for the card to do "housekeeping" before cutting the
     * power.  Has o effect if a pin has not been set to control power to the SD
     * card.
     *
     * @param waitForHousekeeping True to add a 1 second delay between to allow
     * any on-chip writing to complete before cutting power.  Defaults to true.
     */
    void turnOffSDcard(bool waitForHousekeeping = true);

    /**
     * @brief Set a digital pin number for the slave select (chip select) of the
     * SD card and activate it as an output pin.
     *
     * This over-writes the value (if any) given in the constructor.  The pin
     * mode of this pin will be set as `OUTPUT`.
     *
     * Because this sets the pin mode, this function should only be called
     * during the `setup()` or `loop()` portion of an Arduino program.
     *
     * @param SDCardSSPin The pin on the mcu connected to the slave select of
     * the SD card.
     */
    void setSDCardSS(int8_t SDCardSSPin);

    /**
     * @brief Set both pin numbers related to the SD card and activate them as
     * output pins.
     *
     * These over-write the values (if any) given in the constructor.  The pin
     * mode of these pins will be set as `OUTPUT`.
     *
     * Because this sets the pin mode, this function should only be called
     * during the `setup()` or `loop()` portion of an Arduino program.
     *
     * @param SDCardSSPin The pin on the mcu connected to the slave select of
     * the SD card.
     * @param SDCardPowerPin A digital pin number on the mcu controlling power
     * to the SD card.
     */
    void setSDCardPins(int8_t SDCardSSPin, int8_t SDCardPowerPin);

    //
    /**
     * @brief Set digital pin number for the wake up pin used as an RTC
     * interrupt and activate it in the given pin mode.
     *
     * This over-writes the value (if any) given in the constructor.  Use a
     * value of -1 to prevent the board from attempting to sleep.  If using a
     * SAMD board with the internal RTC, the value of the pin is irrelevant as
     * long as it is positive.
     *
     * Because this sets the pin mode, this function should only be called
     * during the `setup()` or `loop()` portion of an Arduino program.
     *
     * @note  This sets the pin mode but does NOT enable the interrupt!
     *
     * @param mcuWakePin The pin on the mcu to be used to wake the mcu from deep
     * sleep.
     * @param wakePinMode The pin mode to be used for wake up on the clock alert
     * pin.  Must be either `INPUT` OR `INPUT_PULLUP`.  Optional with a default
     * value of `INPUT_PULLUP`.  The DS3231 has an active low interrupt, so the
     * pull-up resistors should be enabled.
     */
    void setRTCWakePin(int8_t mcuWakePin, uint8_t wakePinMode = INPUT_PULLUP);

    /**
     * @brief Set the digital pin number to put out an alert that a measurement
     * is being logged and activate it as an output pin.
     *
     * The pin mode of this pin will be set as `OUTPUT`.  This is intended to be
     * a pin with a LED on it so you can see the light come on when a
     * measurement is being taken.
     *
     * Because this sets the pin mode, this function should only be called
     * during the `setup()` or `loop()` portion of an Arduino program.
     *
     * @param ledPin The pin on the mcu to be held `HIGH` while sensor data is
     * being collected and logged.
     */
    void setAlertPin(int8_t ledPin);
    /**
     * @brief Set the alert pin high.
     */
    void alertOn();
    /**
     * @brief Set the alert pin low.
     */
    void alertOff();

    /**
     * @brief Set the digital pin numbers and activate pin modes for the five
     * pins of interest for the logger
     *
     * Because this sets the pin mode, this function should only be called
     * during the `setup()` or `loop()` portion of an Arduino program.
     *
     * @param mcuWakePin The pin on the mcu to listen to for a value-change
     * interrupt to wake from deep sleep.  The mode of this pin will be set to
     * `wakePinMode`.
     * @param SDCardSSPin The pin on the mcu connected to the slave select of
     * the SD card.  The pin mode of this pin will be set as `OUTPUT`.
     * @param SDCardPowerPin A digital pin number on the mcu controlling power
     * to the SD card.  The pin mode of this pin will be set as `OUTPUT`.
     * @param ledPin The pin on the mcu to be held `HIGH` while sensor data is
     * being collected and logged.  The pin mode of this pin will be set as
     * `OUTPUT`.
     * @param wakePinMode The pin mode to be used for wake up on the
     * `mcuWakePin` (clock alert) pin.  Must be either `INPUT` OR
     * `INPUT_PULLUP`. Optional with a default value of `INPUT_PULLUP`.  The
     * DS3231 has an active low interrupt, so the pull-up resistors should be
     * enabled.
     */
    void setLoggerPins(int8_t mcuWakePin, int8_t SDCardSSPin,
                       int8_t SDCardPowerPin, int8_t ledPin,
                       uint8_t wakePinMode = INPUT_PULLUP);

 protected:
    // Initialization variables
    /**
     * @brief The logger id
     */
    const char* _loggerID = "MyLogger";
    /**
     * @brief The logging interval in minutes
     */
    uint16_t _loggingIntervalMinutes = 5;
    /**
     * @brief Digital pin number on the mcu controlling the SD card slave
     * select.
     */
    int8_t _SDCardSSPin = -1;
    /**
     * @brief Digital pin number on the mcu controlling SD card power
     */
    int8_t _SDCardPowerPin = -1;
    /**
     * @brief Digital pin number on the mcu receiving interrupts to wake from
     * deep-sleep.
     */
    int8_t _mcuWakePin = -1;
    /**
     * @brief Digital pin number on the mcu used to output an alert that the
     * logger is measuring.
     *
     * Expected to be connected to a LED.
     */
    int8_t _ledPin = -1;
    /**
     * @brief Digital pin number on the mcu receiving interrupts to enter
     * testing mode.
     *
     * Expected to be connected to a user button.
     */
    int8_t _buttonPin = -1;

    /**
     * @brief The sampling feature UUID
     */
    const char* _samplingFeatureUUID = nullptr;
    // ^^ Start with no feature UUID
    /**@}*/

    // ===================================================================== //
    /**
     * @anchor logger_time
     * @name Clock and Timezones
     * Public functions to access the clock in proper format and time zone
     */
    /**@{*/
    // ===================================================================== //

 public:
    /**
     * @brief Set the static timezone that the data will be logged in.
     *
     * @note This must be set.
     *
     * @param timeZone The timezone data shold be saved to the SD card in.  This
     * need not be the same as the timezone of the real time clock.
     */
    static void setLoggerTimeZone(int8_t timeZone);
    /**
     * @brief Get the Logger Time Zone.
     *
     * @return **int8_t** The timezone data is be saved to the SD card in.  This
     * is not be the same as the timezone of the real time clock.
     */
    static int8_t getLoggerTimeZone(void);
    /**
     * @brief Retained for backwards compatibility; use setLoggerTimeZone(int8_t
     * timeZone) in new code.
     *
     * @m_deprecated_since{0,22,4}
     *
     * @param timeZone The timezone data shold be saved to the SD card in.  This
     * need not be the same as the timezone of the real time clock.
     */
    static void setTimeZone(int8_t timeZone);
    /**
     * @brief Retained for backwards compatibility; use getLoggerTimeZone() in
     * new code.
     *
     * @m_deprecated_since{0,22,4}
     *
     * @return **int8_t** The timezone data is be saved to the SD card in.  This
     * is not be the same as the timezone of the real time clock.
     */
    static int8_t getTimeZone(void);

    /**
     * @brief Set the static timezone that the RTC is programmed in.
     *
     * @note I VERY, VERY STRONGLY RECOMMEND SETTING THE RTC IN UTC
     *
     * @param timeZone The timezone of the real-time clock (RTC)
     */
    static void setRTCTimeZone(int8_t timeZone);
    /**
     * @brief Get the timezone of the real-time clock (RTC).
     *
     * @return **int8_t** The timezone of the real-time clock (RTC)
     */
    static int8_t getRTCTimeZone(void);

    /**
     * @brief Set the offset between the built-in clock and the time zone
     * where the data is being recorded.
     *
     * If your RTC is set in UTC and your logging timezone is EST, this should
     * be -5.  If your RTC is set in EST and your timezone is EST this does not
     * need to be called.
     *
     * @param offset The difference between the timezone of the RTC and the
     * saved data
     */
    static void setTZOffset(int8_t offset);
    /**
     * @brief Get the offset between the built-in clock and the time zone
     * where the data is being recorded.
     *
     * @return **int8_t** The offset between the built-in clock and the time
     * zone where the data is being recorded.
     */
    static int8_t getTZOffset(void);

    /**
     * @brief The RTC object.
     *
     * @note Only one RTC may be used.  Either the built-in RTC of a SAMD board
     * *OR* a DS3231
     */
    static RV8803 rtc;

    /**
     * @brief Get the current epoch time from the RTC (unix time, ie, the
     * number of seconds from January 1, 1970 00:00:00) and correct it to the
     * logging time zone.
     *
     * @return **uint32_t**  The number of seconds from January 1, 1970 in the
     * logging time zone.
     *
     * @m_deprecated_since{0,33,0}
     */
    static uint32_t getNowEpoch(void);

    /**
     * @brief Get the current epoch time from the RTC (unix time, ie, the
     * number of seconds from January 1, 1970 00:00:00) and correct it to the
     * logging time zone.
     *
     * @return **uint32_t**  The number of seconds from January 1, 1970 in the
     * logging time zone.
     */
    static uint32_t getNowLocalEpoch(void);

    /**
     * @brief Get the current Universal Coordinated Time (UTC) epoch time from
     * the RTC (unix time, ie, the number of seconds from January 1, 1970
     * 00:00:00 UTC)
     *
     * @return **uint32_t**  The number of seconds from 1970-01-01T00:00:00Z0000
     */
    static uint32_t getNowUTCEpoch(void);
    /**
     * @brief Set the real time clock to the given number of seconds from
     * January 1, 1970.
     *
     * The validity of the timestamp is not checked in any way!  In practice,
     * setRTClock(ts) should be used to avoid setting the clock to an obviously
     * invalid value.  The input value should be *in the timezone of the RTC.*
     *
     * @param ts The number of seconds since 1970.
     */
    static void setNowUTCEpoch(uint32_t ts);

    /**
     * @brief Convert an epoch time (unix time) into a ISO8601 formatted string.
     *
     * This assumes the supplied date/time is in the LOGGER's timezone and adds
     * the LOGGER's offset as the time zone offset in the string.
     *
     * @param epochTime The number of seconds since 1970.
     * @return **String** An ISO8601 formatted String.
     */
    static String formatDateTime_ISO8601(uint32_t epochTime);

    /**
     * @brief Veify that the input value is sane and if so sets the real time
     * clock to the given time.
     *
     * @param UTCEpochSeconds The number of seconds since 1970 in UTC.
     * @return **bool** True if the input timestamp passes sanity checks **and**
     * the clock has been successfully set.
     */
    bool setRTClock(uint32_t UTCEpochSeconds);

    /**
     * @brief Check that the current time on the RTC is within a "sane" range.
     *
     * To be sane the clock  must be between 2020 and 2030.
     *
     * @return **bool** True if the current time on the RTC passes sanity range
     * checking
     */
    static bool isRTCSane(void);
    /**
     * @brief Check that a given epoch time (seconds since 1970) is within a
     * "sane" range.
     *
     * To be sane the clock  must be between 2020 and 2025.
     *
     * @param epochTime The epoch time to be checked.
     * @return **bool** True if the given time passes sanity range checking.
     */
    static bool isRTCSane(uint32_t epochTime);

    /**
     * @brief Set static variables for the date/time
     *
     * This is needed so that all data outputs (SD, EnviroDIY, serial printing,
     * etc) print the same time for updating the sensors - even though the
     * routines to update the sensors and to output the data may take several
     * seconds.  It is not currently possible to output the instantaneous time
     * an individual sensor was updated, just a single marked time.  By custom,
     * this should be called before updating the sensors, not after.
     */
    static void markTime(void);

    /**
     * @brief Check if the CURRENT time is an even interval of the logging rate
     *
     * @return **bool** True if the current time on the RTC is an even interval
     * of the logging rate.
     */
    bool checkInterval(void);

    /**
     * @brief Check if the MARKED time is an even interval of the logging rate -
     * That is the value saved in the static variable markedLocalEpochTime.
     *
     * This should be used in conjunction with markTime() to ensure that all
     * data outputs from a single data update session (SD, EnviroDIY, serial
     * printing, etc) have the same timestamp even though the update routine may
     * take several (or many) seconds.
     *
     * @return **bool** True if the marked time is an even interval of the
     * logging rate.
     */
    bool checkMarkedInterval(void);

 protected:
    /**
     * @brief The static timezone data is being logged in.
     *
     * @note All logger objects, if multiple are used, will be in the same
     * timezone.
     */
    static int8_t _loggerTimeZone;
    /**
     * @brief The static difference between the timezone of the RTC and the
     * timezone data is being logged in.
     *
     * @note All logger objects, if multiple are used, will be will have the
     * same offset.
     */
    static int8_t _loggerRTCOffset;
    /**@}*/

    // ===================================================================== //
    /**
     * @anchor logger_sleep
     * @name Clock and Timezones
     * Public Functions for sleeping the logger
     */
    /**@{*/
    // ===================================================================== //

 public:
    /**
     * @brief Set up the Interrupt Service Request for waking
     *
     * In this case, we're doing nothing, we just want the processor to wake.
     * This must be a static function (which means it can only call other static
     * funcions.)
     */
    static void wakeISR(void);

    /**
     * @brief Put the mcu to sleep to conserve battery life and handle
     * post-interrupt wake actions
     *
     * @note This DOES NOT sleep or wake the sensors!!
     */
    void systemSleep(void);
    /**
     * @brief A watch-dog implementation to use to reboot the system in case of
     * lock-ups
     */
    extendedWatchDogSAMD watchDogTimer;
    /**@}*/

    // ===================================================================== //
    /**
     * @anchor logger_sdcard
     * @name SD Cards and Saving Data
     * Public functions for logging data to an SD card
     */
    /**@{*/
    // ===================================================================== //

 public:
    /**
     * @brief Set the file name, if you want to decide on it in advance.
     *
     * If the file name is set using this function, the same file name will be
     * used for every single file created by the logger.
     *
     * @param fileName The file name
     */
    void setFileName(const char* fileName);
    /**
     * @brief Set the file name, if you want to decide on it in advance.
     *
     * If the file name is set using this function, the same file name will be
     * used for every single file created by the logger.
     *
     * @param fileName  The file name
     */
    void setFileName(String& fileName);

    /**
     * @brief Get the current filename.
     *
     * This may be a single filename set using the setFileName(name) function or
     * an auto-generated filename which is a concatenation of the logger id and
     * the date when the file was started.
     *
     * @return **String** The name of the file data is currently being saved to.
     */
    String getFileName(void) {
        return _fileName;
    }

    /**
     * @brief Create a file on the SD card and set the created, modified, and
     * accessed timestamps in that file.
     *
     * The filename will be the value specified in the function.  If desired, a
     * header will also be written to the file based on the variable information
     * from the variable array.  This can be used to force a logger to create a
     * file with a secondary file name.
     *
     * @param filename The name of the file to create
     * @return **bool** True if the file was successfully created.
     */
    bool createLogFile(String& filename);
    /**
     * @brief Create a file on the SD card and set the created, modified, and
     * accessed timestamps in that file.
     *
     * The filename will be the one set by setFileName(String) or generated
     * using the logger id and the date.  If desired, a header will also be
     * written to the file based on the variable information from the variable
     * array.
     *
     * @return **bool** True if the file was successfully created.
     */
    bool createLogFile();

    /**
     * @brief Open a file with the given name on the SD card and append the
     * given line to the bottom of it.
     *
     * If a file with the specified name does not already exist, attempt to
     * create the file and add a header to it.  Set the modified and accessed
     * timestamps of the file to the current time.
     *
     * @param filename The name of the file to write to
     * @param rec The line to be written to the file
     * @return **bool** True if the file was successfully accessed or created
     * _and_ data appended to it.
     */
    bool logToSD(String& filename, String& rec);
    /**
     * @brief Open a file named with the current internal filename value and
     * append the given line to the bottom of it.
     *
     * If a file with the with the intenal filename does not already exist,
     * attempt to create a file with that name and add a header to it.  Set the
     * modified and accessed timestamps of the file to the current time.
     *
     * @param rec The line to be written to the file
     * @return **bool** True if the file was successfully accessed or created
     * _and_ data appended to it.
     */
    bool logToSD(String& rec);

    // The SD card and file
    /**
     * @brief An internal reference to SdFat for SD card control
     */
    SdFat sd;
    /**
     * @brief An internal reference to an SdFat file instance
     */
    File logFile;
    /**
     * @brief An internal reference to the current filename
     */
    String _fileName = "";
    // ^^ Initialize with no file name

    /**
     * @brief Check if the SD card is available and ready to write to.
     *
     * We run this check before every communication with the SD card to prevent
     * hanging.
     *
     * @return **bool** True if the SD card is ready
     */
    bool initializeSDCard(void);

    /**
     * @brief Generate a file name from the logger id and the current date.
     *
     * @note This cannot be called until *after* the RTC is started
     */
    void generateAutoFileName(void);

    /**
     * @brief Set a timestamp on a file.
     *
     * @param fileToStamp The filename to change the timestamp of
     * @param stampFlag The "flag" of the timestamp to change - should be
     * T_CREATE, T_WRITE, or T_ACCESS
     */
    void setFileTimestamp(File& fileToStamp, uint8_t stampFlag);

    /**
     * @brief Open or creates a file, converting a string file name to a
     * character file name.
     *
     * @param filename The name of the file to open
     * @param createFile True to create the file if it did not already exist
     * @return **bool** True if a file was successfully opened or created.
     */
    bool openFile(String& filename, bool createFile);
    /**@}*/

    // ===================================================================== //
    /**
     * @anchor logger_conv
     * @name Do-It-All Convience Functions
     * Convience functions to call several of the above functions
     */
    /**@{*/
    // ===================================================================== //

    /**
     * @brief Set all pin levels and does initial communication with the
     * real-time clock and SD card to prepare the logger for full functionality.
     *
     * This is used for operations that cannot happen in the logger constructor
     * - they must happen at run time, not at compile time.
     */
    virtual void begin();

    /**
     * @brief The static "marked" epoch time for the local timezone.
     */
    static uint32_t markedLocalEpochTime;

    /**
     * @brief The static "marked" epoch time for UTC.
     */
    static uint32_t markedUTCEpochTime;

    // These are flag fariables noting the current state (logging/testing)
    // NOTE:  if the logger isn't currently logging or testing or in the middle
    // of set-up, it's probably sleeping
    // Setting these as volatile because the flags can be changed in ISR's
    /**
     * @brief Internal flag set to true when logger is currently updating
     * sensors or writing to the SD card
     */
    static volatile bool isLoggingNow;
    /**
     * @brief Internal flag set to true when the logger is going through the
     * "testing mode" routine.
     */
    static volatile bool isTestingNow;
    /**
     * @brief Internal flag set to true with then logger should begin the
     * "testing mode" routine when it finishes other operations.
     */
    static volatile bool startTesting;
    /**@}*/
};

#endif  // SRC_LOGGERBASE_H_
