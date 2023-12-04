#include "DS18B20.h"
#include "DS18B20_defines.h"

using namespace g3rb3n;

DS18B20::DS18B20(uint8_t _pin, bool parasitic)
    : wire(new OneWire(_pin)),
      parasitic(parasitic)
{
}

/*
 * Find chip, set chip type and read scratch pad.
 */
ds18b20_error_t DS18B20::begin()
{
    startErrorCode = findChip();
    if (startErrorCode != DS18B20_OK)
        return startErrorCode;

    startErrorCode = setChipType();
    if (startErrorCode != DS18B20_OK)
        return startErrorCode;

    startErrorCode = readScratchpad();
    if (startErrorCode != DS18B20_OK)
        return startErrorCode;

    return startErrorCode;
}

/*
 * Use one wire to find chip and read ROM.
 */
ds18b20_error_t DS18B20::findChip()
{
    int count = 0;
    while (!wire->search(rom))
    {
        wire->reset_search();
        delay(250);
        yield();
        if (count++ >= 10)
            return DS18B20_ERROR_NOT_FOUND;
    }
    if (OneWire::crc8(rom, 7) != rom[7])
        return DS18B20_ERROR_ADDRESS_CRC;
    return DS18B20_OK;
}

/*
 * Set chip type based on family.
 */
ds18b20_error_t DS18B20::setChipType()
{
    // the first ROM byte indicates which chip
    switch (rom[0])
    {
    case 0x10:
        type_s = 1;
        break;
    case 0x28:
        type_s = 0;
        break;
    case 0x22:
        type_s = 0;
        break;
    default:
        return DS18B20_ERROR_CHIP_NOT_SUPPORTED;
    }
    return DS18B20_OK;
}

/*
 * Reads the entire scratchpad including the CRC byte.
 */
ds18b20_error_t DS18B20::readScratchpad()
{
    uint8_t error = sendCommand(DS18B20_COMMAND_READ_SCRATCHPAD);
    if (error)
        return error;
    wire->read_bytes(data, 9);
    if (data[DS18B20_SCRATCHPAD_BYTE_RESERVERD_FF] != 0xFF)
        return DS18B20_ERROR_DATA;
    if (OneWire::crc8(data, 8) != data[8])
        return DS18B20_ERROR_DATA_CRC;
#ifdef DS18B20_DEBUG
    for (int i = 0; i < 9; ++i)
    {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
#endif
    return DS18B20_OK;
}

/*
 * Writes data into scratchpad bytes 2, 3, and 4 (T H , T L , and configuration registers).
 */
ds18b20_error_t DS18B20::writeScratchpad()
{
    uint8_t error = sendCommand(DS18B20_COMMAND_WRITE_SCRATCHPAD);
    if (error)
        return error;
    wire->write_bytes(data + DS18B20_SCRATCHPAD_WRITEABLE_START, DS18B20_SCRATCHPAD_WRITEABLE_LENGTH, parasitic);
    wire->reset();
    readScratchpad();
    return DS18B20_OK;
}

/*
 * Copies T H , T L , and configuration register data from the scratchpad to EEPROM.
 */
ds18b20_error_t DS18B20::saveScratchpad()
{
    return sendCommand(DS18B20_COMMAND_SAVE_SCRATCHPAD);
}

/*
 * Recalls T H , T L , and configuration register data from EEPROM to the scratchpad.
 */
ds18b20_error_t DS18B20::loadScratchpad()
{
    return sendCommand(DS18B20_COMMAND_LOAD_SCRATCHPAD);
}

/*
 * Return the chip configuration byte.
 */
uint8_t DS18B20::configuration()
{
    return data[DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER];
}

/*
 * Return the chip family byte.
 */
uint8_t DS18B20::family()
{
    return rom[DS18B20_ROM_BYTE_FAMILY];
}

/*
 * Return the chip serial number.
 */
String DS18B20::serialNumber()
{
    String ret;
    for (uint8_t i = DS18B20_ROM_BYTE_SERIAL_START; i <= DS18B20_ROM_BYTE_SERIAL_END; ++i)
        ret += String(rom[i], HEX);
    return ret;
}

/*
 * Return the bit resolution.
 */
uint8_t DS18B20::resolution()
{
    if (type_s)
        return 9;
    uint8_t cfg = (data[4] & DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER_RESOLUTION_BITS);
    if (cfg == 0x00)
        return 9;
    if (cfg == 0x20)
        return 10;
    if (cfg == 0x40)
        return 11;
    return 12;
}

/*
 * Set the bit resolution.
 */
uint8_t DS18B20::resolution(uint8_t resolution)
{
    uint8_t cfg;
    switch (resolution)
    {
    case 9:
        cfg = 0x00;
        break;
    case 10:
        cfg = 0x20;
        break;
    case 11:
        cfg = 0x40;
        break;
    case 12:
        cfg = 0x60;
        break;
    default:
        return DS18B20_ERROR_UNKNOWN_RESOLUTION;
    }
    data[DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER] &= ~DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER_RESOLUTION_BITS;
    data[DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER] |= cfg;
    return writeScratchpad();
}

/*
 * Returns the measuring time in milliseconds, based on the bit resolution.
 */
uint16_t DS18B20::measuringTimeMillis()
{
    uint8_t res = resolution();
    switch (res)
    {
    case 9:
        return 94; // 9 bit resolution, 93.75 ms
    case 10:
        return 188; // 10 bit res, 187.5 ms
    case 11:
        return 375; // 11 bit res, 375 ms
    default:
        return 750; // Default is 12 bit resolution, 750 ms conversion time
    }
    return 750;
}

/* Convert the data to actual temperature
 * because the result is a 16 bit signed integer, it should
 * be stored to an "int16_t" type, which is always 16 bits
 * even when compiled on a 32 bit processor.
 */
uint16_t DS18B20::temperatureBytesToRaw()
{
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s)
    {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10)
        {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    }
    else
    {
        uint8_t cfg = (data[4] & DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER_RESOLUTION_BITS);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00)
            raw = raw & ~7; // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20)
            raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40)
            raw = raw & ~1; // 11 bit res, 375 ms
                            // Default is 12 bit resolution, 750 ms conversion time
    }
    return raw;
}

/*
 * Read the temperature, uses delay to wait for measurement.
 */
Return<float> DS18B20::temperature()
{
    startMeasurement();
    delay(measuringTimeMillis());
    return lastTemperature();
}

/*
 * Read the temperature from the chip.
 */
Return<float> DS18B20::lastTemperature()
{
    #ifdef DS18B20_DEBUG
        Serial.println("lastTemperature");
    #endif
    if (!present)
        return Return<float>::error(DS18B20_ERROR_NOT_PRESENT);
    uint8_t code = readScratchpad();
    if (code != 0)
        return Return<float>::error(code);
    uint16_t raw = temperatureBytesToRaw();
    float celsius = (float)raw / 16.0;
    return celsius;
}

/*
 * DS18B20 transmits conversion status to master (not applicable for parasite-powered DS18B20s).
 * For parasite-powered DS18B20s, the master must enable a strong pullup on the 1-Wire bus during temperature
 * conversions and copies from the scratchpad to EEPROM. No other bus activity may take place during this time.
 */
ds18b20_error_t DS18B20::startMeasurement()
{
    #ifdef DS18B20_DEBUG
        Serial.println("startMeasurement");
    #endif
    if (startErrorCode != 0)
        begin();
    uint8_t error = sendCommand(DS18B20_COMMAND_CONVERT_TEMPERATURE);
    #ifdef DS18B20_DEBUG
        Serial.print("startMeasurement code ");
        Serial.println(error);
    #endif
    if (error)
        return error;
    measuringState = true;
    measurementStarted = millis();
    return DS18B20_OK;
}

/*
 * Measurement is ongoing
 */
bool DS18B20::measuring()
{
    return measuringState;
}

/*
 * Handle function for async behaviour. Resets measuring flag is measuring time is passed.
 */
bool DS18B20::handle()
{
    if (measuringState && ((millis() - measurementStarted) > (measuringTimeMillis() + (repowered ? 1000 : 0))))
    {
        measuringState = false;
        repowered = false;
    }
    return true;
}

/*
 * Send a command byte to the chip.
 */
ds18b20_error_t DS18B20::sendCommand(uint8_t cmd)
{
    #ifdef DS18B20_DEBUG
        Serial.print("sendCommand ");
        Serial.println(cmd, HEX);
    #endif
    bool presentNow = wire->reset();
    if (!present && presentNow)
        repowered = true;
    present = presentNow;
    if (!present)
        return DS18B20_ERROR_NOT_PRESENT;
    wire->select(rom);
    wire->write(cmd, parasitic);
    return DS18B20_OK;
}
