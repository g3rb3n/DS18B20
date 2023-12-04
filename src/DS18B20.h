#ifndef _DS18B20_H
#define _DS18B20_H

#include "Return.h"
#include <OneWire.h>


typedef uint8_t ds18b20_error_t;

namespace g3rb3n
{
    class DS18B20
    {
    private:
        bool parasitic;
        OneWire *wire;

        uint8_t type_s;
        uint8_t startErrorCode = -1;
        uint8_t rom[8];

        uint8_t data[9];

        uint32_t measurementStarted = 0;
        bool measuringState = false;
        bool present = false;
        bool repowered = false;

        ds18b20_error_t readScratchpad();  // Reads the entire scratchpad including the CRC byte.
        ds18b20_error_t writeScratchpad(); // Writes data into scratchpad bytes 2, 3, and 4 (T H , T L , and configuration registers).
        ds18b20_error_t saveScratchpad();  // Copies T H , T L , and configuration register data from the scratchpad to EEPROM.
        ds18b20_error_t loadScratchpad();  // Recalls T H , T L , and configuration register data from EEPROM to the scratchpad.

        ds18b20_error_t sendCommand(uint8_t cmd);

        ds18b20_error_t findChip();
        ds18b20_error_t setChipType();

        uint16_t temperatureBytesToRaw();

    public:
        DS18B20(uint8_t _pin, bool parasitic = false);
        ds18b20_error_t begin();

        uint8_t family();
        String serialNumber();

        uint8_t configuration();
        uint8_t resolution();
        uint8_t resolution(uint8_t resolution);
        uint16_t measuringTimeMillis();

        // Blocking 
        Return<float> temperature();

        //Non blocking
        Return<float> lastTemperature();
        ds18b20_error_t startMeasurement();
        bool measuring();
        bool handle();
    };
}

#endif
