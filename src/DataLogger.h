#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <EEPROM.h>

/**
 * EEPROM Layout:
 * 0-3: Magic Number (0xDEADBEEF)
 * 4-7: Tap Sequence Starts (uint32_t)
 * 8-11: Tap Sequence Ends (uint32_t)
 * 12-15: Manual CW Total Seconds (uint32_t)
 * 16-19: Manual CCW Total Seconds (uint32_t)
 */

class DataLogger {
private:
    static const uint32_t MAGIC_NUMBER = 0xDEADBEEF;
    static const int ADDR_MAGIC = 0;
    static const int ADDR_TAP_STARTS = 4;
    static const int ADDR_TAP_ENDS = 8;
    static const int ADDR_MANUAL_CW = 12;
    static const int ADDR_MANUAL_CCW = 16;

    template<typename T>
    static void eepromUpdate(int addr, T value) {
        EEPROM.put(addr, value);
    }

    template<typename T>
    static T eepromGet(int addr) {
        T value;
        EEPROM.get(addr, value);
        return value;
    }

public:
    static void init() {
        uint32_t magic = eepromGet<uint32_t>(ADDR_MAGIC);
        if (magic != MAGIC_NUMBER) {
            // Initializing EEPROM for the first time
            eepromUpdate(ADDR_MAGIC, MAGIC_NUMBER);
            eepromUpdate(ADDR_TAP_STARTS, (uint32_t)0);
            eepromUpdate(ADDR_TAP_ENDS, (uint32_t)0);
            eepromUpdate(ADDR_MANUAL_CW, (uint32_t)0);
            eepromUpdate(ADDR_MANUAL_CCW, (uint32_t)0);
        }
    }

    static void incrementTapStarts() {
        uint32_t val = eepromGet<uint32_t>(ADDR_TAP_STARTS);
        eepromUpdate(ADDR_TAP_STARTS, val + 1);
    }

    static void incrementTapEnds() {
        uint32_t val = eepromGet<uint32_t>(ADDR_TAP_ENDS);
        eepromUpdate(ADDR_TAP_ENDS, val + 1);
    }

    static void addManualCWSeconds(uint32_t seconds) {
        uint32_t val = eepromGet<uint32_t>(ADDR_MANUAL_CW);
        eepromUpdate(ADDR_MANUAL_CW, val + seconds);
    }

    static void addManualCCWSeconds(uint32_t seconds) {
        uint32_t val = eepromGet<uint32_t>(ADDR_MANUAL_CCW);
        eepromUpdate(ADDR_MANUAL_CCW, val + seconds);
    }

    static void dumpLogs() {
        Serial.println(F("--- EEPROM Logs ---"));
        Serial.print(F("Tap Starts:      ")); Serial.println(eepromGet<uint32_t>(ADDR_TAP_STARTS));
        Serial.print(F("Tap Ends:        ")); Serial.println(eepromGet<uint32_t>(ADDR_TAP_ENDS));
        Serial.print(F("Manual CW:       ")); Serial.print(eepromGet<uint32_t>(ADDR_MANUAL_CW)); Serial.println(F("s"));
        Serial.print(F("Manual CCW:      ")); Serial.print(eepromGet<uint32_t>(ADDR_MANUAL_CCW)); Serial.println(F("s"));
        Serial.println(F("-------------------"));
    }
};

#endif
