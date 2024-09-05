/**
 * @file LoggerModem.h
 * @copyright Stroud Water Research Center
 * Part of the EnviroDIY ModularSensors library for Arduino.
 * This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Contains the loggerModem class and the variable subclasses
 * Modem_RSSI, Modem_SignalPercent, Modem_BatteryState, Modem_BatteryPercent,
 * and Modem_BatteryVoltage - all of which are implemented as "calculated"
 * variables.
 */
/**
 * @defgroup the_modems Supported Modems and Communication Modules
 * All implemented loggerModem classes
 *
 * @copydetails loggerModem
 *
 * @see @ref page_modem_notes
 */

// Header Guards
#ifndef SRC_LOGGERMODEM_H_
#define SRC_LOGGERMODEM_H_

// FOR DEBUGGING
// #define MS_LOGGERMODEM_DEBUG
// #define MS_LOGGERMODEM_DEBUG_DEEP

#ifdef MS_LOGGERMODEM_DEBUG
#define MS_DEBUGGING_STD "LoggerModem"
#endif

// Included Dependencies
#include "ModSensorDebugger.h"
#undef MS_DEBUGGING_STD
#include <Arduino.h>


/* ===========================================================================
 * Functions for the modem class
 * This is basically a wrapper for TinyGsm with power control added
 * ========================================================================= */

// template <class Derived, typename modemType, typename modemClientType>
/**
 * @brief The loggerModem class provides an internet connection for the
 * logger and supplies an Arduino Client instance to use to publish data.
 *
 * A modem is a device that can be controlled by a logger to send out data
 * directly to the world wide web.
 *
 * The loggerModem class wraps the TinyGSM library and adds in the power
 * functions to turn the modem on and off and some error checking.
 *
 * TinyGSM is available here:  https://github.com/vshymanskyy/TinyGSM
 *
 * @ingroup base_classes
 */
class loggerModem {
 public:
    /**
     * @brief Construct a new loggerModem object.
     *
     * @param powerPin @copybrief loggerModem::_powerPin
     * @param statusPin @copybrief loggerModem::_statusPin
     * @param statusLevel @copybrief loggerModem::_statusLevel
     * @param modemResetPin @copybrief loggerModem::_modemResetPin
     * @param resetLevel @copybrief loggerModem::_resetLevel
     * @param resetPulse_ms @copybrief loggerModem::_resetPulse_ms
     * @param modemSleepRqPin @copybrief loggerModem::_modemSleepRqPin
     * @param wakeLevel @copybrief loggerModem::_wakeLevel
     * @param wakePulse_ms @copybrief loggerModem::_wakePulse_ms
     * @param max_status_time_ms @copybrief loggerModem::_statusTime_ms
     * @param max_disconnetTime_ms @copybrief loggerModem::_disconnetTime_ms
     * @param wakeDelayTime_ms @copybrief loggerModem::_wakeDelayTime_ms
     * @param max_atresponse_time_ms @copybrief #_max_atresponse_time_ms
     *
     * @see @ref modem_ctor_variables
     */
    loggerModem(int8_t powerPin, int8_t statusPin, bool statusLevel,
                int8_t modemResetPin, bool resetLevel, uint32_t resetPulse_ms,
                int8_t modemSleepRqPin, bool wakeLevel, uint32_t wakePulse_ms,
                uint32_t max_status_time_ms, uint32_t max_disconnetTime_ms,
                uint32_t wakeDelayTime_ms, uint32_t max_atresponse_time_ms);

    /**
     * @brief Destroy the logger Modem object - no action taken.
     */
    virtual ~loggerModem();

    /**
     * @brief Set an LED to turn on (pin will be `HIGH`) when the modem is on.
     *
     * @param modemLEDPin The digital pin number for the LED
     */
    void setModemLED(int8_t modemLEDPin);

    /**
     * @brief Get the modem name.
     *
     * @return **String** The modem name
     */
    String getModemName(void);

    /**
     * @brief Get a detailed printable description of the modem.
     *
     * @note These values are polled for and cached in memory till needed
     *
     * @return **String** The concatenated name, hardware version, firmware
     * version, and serial number of the modem.
     *
     * @todo Implement this for modems other than the XBee WiFi
     */
    String getModemDevId(void);

    /**
     * @brief Set up the modem before first use.
     *
     * This is used for operations that cannot happen in the modem constructor -
     * they must happen at run time, not at compile time.
     *
     * @return **bool** True if setup was successful
     */
    virtual bool modemSetup(void);
    /**
     * @brief Retained for backwards compatibility; use modemSetup() in new
     * code.
     *
     * @m_deprecated_since{0,24,1}
     *
     * @return **bool** True if setup was successful
     */
    bool setup(void) {
        return modemSetup();
    }

    /**
     * @anchor modem_power_functions
     * @name Functions related to the modem power and activity state
     *
     * These are similar to the like-named Sensor functions.
     */
    /**@{*/
    /**
     * @brief Wake up the modem.
     *
     * This sets pin modes, powers up the modem if necessary, sets time stamps,
     * runs the specific modem's wake function, tests for responsiveness to AT
     * commands, and then re-runs the TinyGSM init() if necessary.  If the modem
     * fails to respond, this attempts a "hard" pin reset if possible.
     *
     * For most modules, this function is created by the #MS_MODEM_WAKE macro.
     *
     * @return **bool** True if the modem is responsive and ready for action.
     */
    virtual bool modemWake(void) = 0;
    /**
     * @brief Retained for backwards compatibility; use modemWake() in new code.
     *
     * @m_deprecated_since{0,24,1}
     *
     * @return **bool** True if wake was sucessful, modem should be ready to
     * communicate
     */
    bool wake(void) {
        return modemWake();
    }

    /**
     * @brief Power the modem by setting the modem power pin high.
     */
    virtual void modemPowerUp(void);
    /**
     * @brief Cut power to the modem by setting the modem power pin low.
     *
     * @note modemPowerDown() simply kills power, while modemSleepPowerDown()
     * allows for graceful shut down.  You should use modemSleepPowerDown()
     * whenever possible.
     */
    virtual void modemPowerDown(void);
    /**
     * @brief Request that the modem enter its lowest possible power state.
     *
     * @return **bool** True if the modem has sucessfully entered low power
     * state
     */
    virtual bool modemSleep(void);
    /**
     * @brief Request that the modem enter its lowest possible power state and
     * then set the power pin low after the modem has indicated it has
     * successfully gone to low power.
     *
     * This allows the modem to shut down all connections cleanly and do any
     * necessary internal housekeeping before stopping power.
     *
     * @return **bool** True if the modem has sucessfully entered low power
     * state _and_ then powered off
     */
    virtual bool modemSleepPowerDown(void);
    /**@}*/

    /**
     * @brief Use the modem reset pin specified in the constructor to perform a
     * "hard" or "panic" reset.
     *
     * This should only be used if the modem is clearly non-responsive.
     *
     * @return **bool** True if the reset succeeded and the modem should now be
     * responsive.  False if the modem remains non-responsive either because the
     * reset failed to fix the communication issue or because a reset is not
     * possible with the current pin/modem configuration.
     */
    virtual bool modemHardReset(void);


    /**
     * @anchor modem_pin_functions
     * @name Pin setting functions
     * Functions to set or re-set the the pin numbers for the connection between
     * the modem module and the logger MCU.
     */
    /**@{*/
    /**
     * @brief Set the pin level to be expected when the on the modem status pin
     * when the modem is active.
     *
     * If this function is not called, the modem status pin is assumed to
     * exactly follow the hardware specifications for that modems raw cellular
     * component.
     *
     * @param level The active level of the pin (`LOW` or `HIGH`)
     */
    void setModemStatusLevel(bool level);

    /**
     * @brief Set the pin level to be used to wake the modem.
     *
     * If this function is not called, the modem status pin is assumed to
     * exactly follow the hardware specifications for that modems raw cellular
     * component.
     *
     * @param level The pin level (`LOW` or `HIGH`) of the pin while waking
     * the modem.
     */
    void setModemWakeLevel(bool level);

    /**
     * @brief Set the pin level to be used to reset the modem.
     *
     * If this function is not called, the modem status pin is assumed to
     * exactly follow the hardware specifications for that modems raw cellular
     * component - nearly always low.
     *
     * @param level The pin level (`LOW` or `HIGH`) of the pin while
     * resetting the modem.
     */
    void setModemResetLevel(bool level);
    /**@}*/

    /**
     * @anchor modem_internet_functions
     * @name Functions for internet connectivity
     */
    /**@{*/
    /**
     * @brief Wait for the modem to successfully register on the cellular
     * network and then request that it establish either EPS or GPRS data
     * connection.
     *
     * @param maxConnectionTime The maximum length of time in milliseconds to
     * wait for network registration and data sconnection.  Defaults to 50,000ms
     * (50s).
     * @return **bool** True if EPS or GPRS data connection has been
     * established.  False if the modem wasunresponsive, unable to register with
     * the cellular network, or unable to establish a EPS or GPRS connection.
     */
    virtual bool connectInternet(uint32_t maxConnectionTime = 50000L) = 0;
    /**
     * @brief Detatch from EPS or GPRS data connection and then deregister from
     * the cellular network.
     */
    virtual void disconnectInternet(void) = 0;


    /**
     * @brief Get the time from NIST via TIME protocol (rfc868).
     *
     * This would be much more efficient if done over UDP, but I'm doing it over
     * TCP because I don't have a UDP library for all the modems.
     *
     * @note The return is the number of seconds since Jan 1, 1970 IN UTC
     *
     * @return **uint32_t** The number of seconds since Jan 1, 1970 IN UTC
     */
    virtual uint32_t getNISTTime(void) = 0;
    /**@}*/

    /**
     * @anchor modem_helper_functions
     * @name Helper functions
     */
    /**@{*/
    /**
     * @brief Turn on the modem LED/alert pin - sets it `HIGH`
     */
    void modemLEDOn(void);
    /**
     * @brief Turn off the modem LED/alert pin - sets it `LOW`
     */
    void modemLEDOff(void);
    /**
     * @brief Set the processor pin modes (input vs output, with and without
     * pull-up) for all pins connected between the modem module and the mcu.
     */
    virtual void setModemPinModes(void);
    /**@}*/

    /**
     * @anchor modem_virtual_functions
     * @name Pure virtual functions for each modem to implement
     */
    /**@{*/
    /**
     * @brief Check whether there is an active internet connection available.
     *
     * @return **bool** True if there is an active data connection to the
     * internet.
     */
    virtual bool isInternetAvailable(void) = 0;
    /**
     * @brief Perform the parts of the modem sleep process that are unique to a
     * specific module, as opposed to the parts of setup that are common to all
     * modem modules.
     *
     * @return **bool** True if the unique part of the sleep function ran
     * sucessfully.
     */
    virtual bool modemSleepFxn(void) = 0;
    /**
     * @brief Perform the parts of the modem wake up process that are unique to
     * a specific module, as opposed to the parts of setup that are common to
     * all modem modules.
     *
     * @return **bool** True if the unique part of the wake function ran
     * sucessfully - does _NOT_ indicate that the modem is now responsive.
     */
    virtual bool modemWakeFxn(void) = 0;
    /**
     * @brief Perform the parts of the modem set up process that are unique to a
     * specific module, as opposed to the parts of setup that are common to all
     * modem modules.
     *
     * For most modules, this function is created by the #MS_MODEM_EXTRA_SETUP
     * macro which runs the TinyGSM modem init() and client init() functions.
     *
     * @return **bool** True if the extra setup succeeded.
     */
    virtual bool extraModemSetup(void) = 0;
    /**
     * @brief Check if the modem was awake using all possible means.
     *
     * If possible, we always want to check if the modem was awake before
     * attempting to wake it up.  Most cellular modules are woken and put to
     * sleep by identical pulses on a sleep or "power" pin.  We don't want to
     * accidently pulse an already on modem to off.
     *
     * For most modules, this function is created by the #MS_IS_MODEM_AWAKE
     * macro.
     *
     * @note It's possible that the status pin is on, but the modem is actually
     * mid-shutdown.  In that case, we'll mistakenly skip re-waking it.  This
     * only applies to modules with a pulse wake (ie, non-zero wake time).  For
     * all modules that do pulse on, where possible I've selected a pulse time
     * that is sufficient to wake but not quite long enough to put it to sleep
     * and am using AT commands to sleep.  This *should* keep everything lined
     * up.
     *
     * @return **bool** True if the modem is already awake.
     */
    virtual bool isModemAwake(void) = 0;
    /**@}*/

    /**
     * @brief Convert the 4 bytes returned on the NIST daytime protocol to the
     * number of seconds since January 1, 1970 in UTC.
     *
     * NIST supplies a 4 byte response to any TCP connection made on port 37.
     * This is the 32-bit number of seconds since January 1, 1970 00:00:00 UTC.
     * The server closes the TCP connection immediately after sending the data,
     * so there is no need to close it
     *
     * @param nistBytes 4 bytes from NIST
     * @return **uint32_t** the number of seconds since January 1, 1970 00:00:00
     * UTC
     */
    static uint32_t parseNISTBytes(byte nistBytes[4]);

    /**
     * @anchor modem_ctor_variables
     * @name Member variables set in the constructor
     * These are all related to expected modem response times and the pin
     * connections between the modem module and the logger MCU.
     */
    /**@{*/
    /**
     * @brief The digital pin number of the mcu pin controlling power to the
     * modem (active `HIGH`).
     *
     * Should be set to a negative number if the modem should be continuously
     * powered or the power cannot be controlled by the MCU.
     */
    int8_t _powerPin;
    /**
     * @brief The digital pin number of the mcu pin connected to the modem
     * status output pin.
     *
     * Should be set to a negative number if the modem status pin cannot be
     * read.
     */
    int8_t _statusPin;
    /**
     * @brief The level (`LOW` or `HIGH`) of the #_statusPin when the modem
     * is active.
     */
    bool _statusLevel;
    /**
     * @brief The digital pin number of the pin on the mcu attached the the hard
     * or panic reset pin of the modem.
     *
     * Should be set to a negative number if the modem reset pin is not
     * connected to the MCU.
     */
    int8_t _modemResetPin;
    /**
     * @brief The level (`LOW` or `HIGH`) of the #_modemResetPin which will
     * cause the modem to reset.
     */
    bool _resetLevel;
    /**
     * @brief The length of time in milliseconds at #_resetLevel needed on
     * #_modemResetPin to reset the modem.
     */
    uint32_t _resetPulse_ms;
    /**
     * @brief The digital pin number of a pin on the mcu used to request the
     * modem enter its lowest possible power state.
     *
     * Should be set to a negative number if there is no pin usable for deep
     * sleep modes or it is not accessible to the MCU.
     */
    int8_t _modemSleepRqPin;
    /**
     * @brief The level (`LOW` or `HIGH`) on the #_modemSleepRqPin used to
     * **wake** the modem.
     */
    bool _wakeLevel;
    /**
     * @brief The length of pulse in milliseconds at #_wakeLevel needed on the
     * #_modemSleepRqPin to wake the modem.
     *
     * Set to 0 if the pin must be continuously held at #_wakeLevel to keep the
     * modem active.
     */
    uint32_t _wakePulse_ms;
    /**
     * @brief The time in milliseconds between when #modemWake() is run and when
     * the #_statusPin is expected to be at #_statusLevel.
     */
    uint32_t _statusTime_ms;
    /**
     * @brief The maximum length of time in milliseconds between when the modem
     * is requested to enter lowest power state (#modemSleep()) and when it
     * should have completed necessary steps to shut down.
     */
    uint32_t _disconnetTime_ms;
    /**
     * @brief The time in milliseconds between when the modem is powered and
     * when it is able to receive a wake command.
     *
     * That is, the time that should be allowed between #modemPowerUp() and
     * #modemWake().
     */
    uint32_t _wakeDelayTime_ms;
    /**
     * @brief The time in milliseconds between when the modem is awake and when
     * its serial ports reach full functionality and are ready to accept AT
     * commands.
     *
     * That is, the time that should be allowed between #modemWake() and
     * init().  If the modem does not respond within this time frame (plus a
     * 500ms buffer) a #modemHardReset() will be attempted.
     */
    uint32_t _max_atresponse_time_ms;
    /**@}*/

    /**
     * @anchor modem_flag_variables
     * @name Flags and other member variables only used internally
     */
    /**@{*/
    /**
     * @brief The digital pin number of a pin on the mcu used to indicate the
     * modem's current activity state.
     *
     * Intended to be a pin attached to a LED.
     *
     * Should be set to a negative number if no LED is available.
     */
    int8_t _modemLEDPin;

    /**
     * @brief The processor elapsed time when the power was turned on for the
     * modem.
     *
     * The #_millisPowerOn value is set in the modemPowerUp()
     * function.  It is un-set in the modemSleepPowerDown() function.
     */
    uint32_t _millisPowerOn = 0;

    /**
     * @brief The processor elapsed time when the a connection to the NIST time
     * server was last attempted.
     *
     * NIST documentation is very clear that it must not be contacted more than
     * once every 4 seconds.
     */
    uint32_t _lastNISTrequest = 0;
    /**
     * @brief Flag.  True indicates that the modem has already successfully
     * completed setup.
     */
    bool _hasBeenSetup = false;
    /**
     * @brief Flag.  True indicates that the pins on the mcu attached to the
     * modem are set to the correct mode (ie, input vs output).
     */
    bool _pinModesSet = false;
    /**@}*/

    // NOTE:  These must be static so that the modem variables can call the
    // member functions that return them.  (Non-static member functions cannot
    // be called without an object.)
    /**
     * @anchor modem_static_variables
     * @name Static member variables used to hold modem metadata
     */
    /**@{*/
    /**
     * @brief The last stored RSSI value
     *
     * Set by #getModemSignalQuality() or updateModemMetadata().
     * Returned by #getModemRSSI().
     */
    static int16_t _priorRSSI;
    /**
     * @brief The last stored signal strength percent value
     *
     * Set by #getModemSignalQuality() or updateModemMetadata().
     * Returned by #getModemSignalPercent().
     */
    static int16_t _priorSignalPercent;
    /**
     * @brief The last stored modem chip temperature value
     *
     * Set by #getModemChipTemperature() or updateModemMetadata().
     * Returned by #getModemTemperature().
     */
    static float _priorModemTemp;
    /**
     * @brief The last stored modem battery state value
     *
     * Set by #getModemBatteryStats() or updateModemMetadata().
     * Returned by #getModemBatteryChargeState().
     */
    static float _priorBatteryState;
    /**
     * @brief The last stored modem battery percent value
     *
     * Set by #getModemBatteryStats() or updateModemMetadata().
     * Returned by #getModemBatteryChargePercent().
     */
    static float _priorBatteryPercent;
    /**
     * @brief The last stored modem battery voltage value
     *
     * Set by #getModemBatteryStats() or updateModemMetadata().
     * Returned by #getModemBatteryVoltage().
     */
    static float _priorBatteryVoltage;
    // static float _priorActivationDuration;
    // static float _priorPoweredDuration;
    /**@}*/

    /**
     * @brief The modem name
     *
     * Set in the init() portion of the #modemSetup().
     * Returned by #getModemName().
     */
    String _modemName = "unspecified modem";

    // modemType gsmModem;
    // modemClientType gsmClient;

    // @TODO: Implement these for all modems; most support it.

    /**
     * @brief The modem hardware version.
     *
     * Set in #modemSetup().
     * Returned as a portion of the #getModemDevId().
     *
     * @todo Implement this for modems other than the XBee WiFi
     */
    String _modemHwVersion;
    /**
     * @brief The modem firmware version.
     *
     * Set in #modemSetup().
     * Returned as a portion of the #getModemDevId().
     *
     * @todo Implement this for modems other than the XBee WiFi
     */
    String _modemFwVersion;
    /**
     * @brief The modem serial number.
     *
     * Set in #modemSetup().
     * Returned as a portion of the #getModemDevId().
     *
     * @todo Implement this for modems other than the XBee WiFi
     */
    String _modemSerialNumber;

    /**
     * @brief An 8-bit code for the enabled modem polling variables
     *
     * Setting a bit to 0 will disable polling, to 1 will enable it.  By default
     * no polling is enabled to save time and power by not requesting
     * unnecessary information from the modem.  When modem measured variables
     * are attached to a modem, polling for those results is automatically
     * enabled.
     *
     * Bit | Variable Class | Relevent Define
     * ----|----------------|----------------
     *  0  | #Modem_RSSI | #MODEM_RSSI_ENABLE_BITMASK
     *  1  | #Modem_SignalPercent | #MODEM_PERCENT_SIGNAL_ENABLE_BITMASK
     *  2  | #Modem_BatteryState | #MODEM_BATTERY_STATE_ENABLE_BITMASK
     *  3  | #Modem_BatteryPercent | #MODEM_BATTERY_PERCENT_ENABLE_BITMASK
     *  4  | #Modem_BatteryVoltage | #MODEM_BATTERY_VOLTAGE_ENABLE_BITMASK
     *  5  | #Modem_Temp | #MODEM_TEMPERATURE_ENABLE_BITMASK
     */
    uint8_t _pollModemMetaData = 0;
};

// typedef float (loggerModem::_*loggerGetValueFxn)(void);

// #include <LoggerModem.tpp>
#endif  // SRC_LOGGERMODEM_H_
