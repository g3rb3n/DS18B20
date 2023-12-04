#ifndef _DS18B20_H
#define _DS18B20_H

#include "Return.h"
#include <OneWire.h>

#define DS18B20_ERROR_NOT_FOUND 1
#define DS18B20_ERROR_ADDRESS_CRC 2
#define DS18B20_ERROR_DATA_CRC 3
#define DS18B20_ERROR_CHIP_NOT_SUPPORTED 4
#define DS18B20_ERROR_NOT_PRESENT 5
#define DS18B20_ERROR_DATA_NOT_READ 6

#define DS18B20_COMMAND_CONVERT_TEMPERATURE 0x44
#define DS18B20_COMMAND_WRITE_SCRATCHPAD 0x4E
#define DS18B20_COMMAND_READ_SCRATCHPAD 0xBE

#define DS18B20_SCRATCHPAD_BYTE_TEMPERATURE_LSB 0
#define DS18B20_SCRATCHPAD_BYTE_TEMPERATURE_MSB 1
#define DS18B20_SCRATCHPAD_BYTE_TH_REGISTER 2
#define DS18B20_SCRATCHPAD_BYTE_TL_REGISTER 3
#define DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER 4
#define DS18B20_SCRATCHPAD_BYTE_RESERVERD_FF 5
#define DS18B20_SCRATCHPAD_BYTE_RESERVERD_ 6
#define DS18B20_SCRATCHPAD_BYTE_RESERVERD_10 7
#define DS18B20_SCRATCHPAD_BYTE_CRC 8
#define DS18B20_SCRATCHPAD_WRITEABLE_START 2
#define DS18B20_SCRATCHPAD_WRITEABLE_LENGTH 3

#define DS18B20_ROM_BYTE_FAMILY 0
#define DS18B20_ROM_BYTE_SERIAL_START 1
#define DS18B20_ROM_BYTE_SERIAL_END 6
#define DS18B20_ROM_BYTE_CRC 7

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

        uint32_t measurementStarted;
        bool _measuring;

        uint8_t readScratchpad();  // Reads the entire scratchpad including the CRC byte.
        uint8_t writeScratchpad(); // Writes data into scratchpad bytes 2, 3, and 4 (T H , T L , and configuration registers).
        uint8_t saveScratchpad();  // Copies T H , T L , and configuration register data from the scratchpad to EEPROM.
        uint8_t loadScratchpad();  // Recalls T H , T L , and configuration register data from EEPROM to the scratchpad.


    public:
        DS18B20(uint8_t _pin, bool parasitic = false);
        uint8_t begin();

        uint8_t family();
        String serial();

        uint8_t configuration();
        uint8_t resolution();
        uint8_t resolution(uint8_t resolution);
        uint16_t measuringTimeMillis();

        // Blocking 
        Return<float> temperature();

        //Non blocking
        Return<float> lastTemperature();
        uint8_t startMeasurement();
        bool measuring();
        bool handle();
    };
}

#endif
