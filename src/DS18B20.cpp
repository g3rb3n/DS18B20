#include "DS18B20.h"

using namespace g3rb3n;

DS18B20::DS18B20(uint8_t _pin, bool parasitic)
    : wire(new OneWire(_pin)),
      parasitic(parasitic)
{
}

uint8_t DS18B20::begin()
{
    startErrorCode = 0;

    int count = 0;
    while (!wire->search(rom))
    {
        wire->reset_search();
        delay(250);
        yield();
        if (count++ >= 10)
        {
#ifdef DS18B20_DEBUG
            Serial.println("Did not find DS18B20");
#endif
            startErrorCode = DS18B20_ERROR_NOT_FOUND;
            return startErrorCode;
        }
    }

#ifdef DS18B20_DEBUG
    Serial.print("ROM = ");
    for (uint8_t i = 0; i < 8; i++)
    {
        Serial.write(' ');
        Serial.print(rom[i], HEX);
    }
    Serial.println();
    Serial.print("Family = ");
    Serial.print(rom[0], HEX);
    Serial.println();
    Serial.print("Serial = ");
    for (uint8_t i = 1; i < 7; i++)
    {
        Serial.write(' ');
        Serial.print(rom[i], HEX);
    }
    Serial.println();
#endif

    if (OneWire::crc8(rom, 7) != rom[7])
    {
#ifdef DS18B20_DEBUG
        Serial.println("ROM CRC is not valid!");
#endif
        startErrorCode = DS18B20_ERROR_ADDRESS_CRC;
        return startErrorCode;
    }

    // the first ROM byte indicates which chip
    switch (rom[0])
    {
    case 0x10:
        //      Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
    case 0x28:
        //      Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
    case 0x22:
        //      Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
    default:
#ifdef DS18B20_DEBUG
        Serial.println("Device is not a DS18x20 family device.");
#endif
        startErrorCode = DS18B20_ERROR_CHIP_NOT_SUPPORTED;
        return startErrorCode;
    }
    startErrorCode = readScratchpad();
    if (startErrorCode != 0)
        return startErrorCode;
    return startErrorCode;
}

// Reads the entire scratchpad including the CRC byte.
uint8_t DS18B20::readScratchpad()
{
    uint8_t present = 0;

    present = wire->reset();
    if (!present)
    {
        Serial.println("Sensor not present");
        return DS18B20_ERROR_NOT_PRESENT;
    }
    wire->select(rom);
    wire->write(DS18B20_COMMAND_READ_SCRATCHPAD); // Read Scratchpad

#ifdef DS18B20_DEBUG
    Serial.print("  Data = ");
    Serial.print(present, HEX);
    Serial.print(" ");
#endif
    for (uint8_t i = 0; i < 9; i++)
    { // we need 9 bytes
        data[i] = wire->read();
#ifdef DS18B20_DEBUG
        Serial.print(data[i], HEX);
        Serial.print(" ");
#endif
    }

#ifdef DS18B20_DEBUG
    Serial.print(" CRC=");
    Serial.print(OneWire::crc8(data, 8), HEX);
    Serial.println();
#endif

    if (OneWire::crc8(data, 8) != data[8])
    {
#ifdef DS18B20_DEBUG
        Serial.println("CRC is not valid!");
#endif
        return DS18B20_ERROR_DATA_CRC;
    }
    return 0;
}

uint8_t DS18B20::configuration()
{
    return data[DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER];
}

uint8_t DS18B20::family()
{
    return rom[DS18B20_ROM_BYTE_FAMILY];
}

String DS18B20::serial()
{
    String ret;
    for (uint8_t i = DS18B20_ROM_BYTE_SERIAL_START; i <= DS18B20_ROM_BYTE_SERIAL_END; ++i)
        ret += String(rom[i], HEX);
    return ret;
}

uint8_t DS18B20::resolution()
{
    if (type_s)
        return 9;
    uint8_t cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00)
        return 9;
    if (cfg == 0x20)
        return 10;
    if (cfg == 0x40)
        return 11;
    return 12;
}

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
        return 255;
    }
    data[DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER] &= 0x0F;
    data[DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER] |= cfg;
#ifdef DS18B20_DEBUG
    Serial.println(data[DS18B20_SCRATCHPAD_BYTE_CONFIGURATION_REGISTER]);
#endif
    return writeScratchpad();
}

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

// Writes data into scratchpad bytes 2, 3, and 4 (T H , T L , and configuration registers).
uint8_t DS18B20::writeScratchpad()
{
    wire->reset();
    wire->select(rom);
    wire->write(DS18B20_COMMAND_WRITE_SCRATCHPAD, parasitic);
    wire->write_bytes(data + DS18B20_SCRATCHPAD_WRITEABLE_START, DS18B20_SCRATCHPAD_WRITEABLE_LENGTH, parasitic);
    wire->reset();
    readScratchpad();
    return 0;
}

// Copies T H , T L , and configuration register data from the scratchpad to EEPROM.
uint8_t DS18B20::saveScratchpad()
{
    return 0;
}

// Recalls T H , T L , and configuration register data from EEPROM to the scratchpad.
uint8_t DS18B20::loadScratchpad()
{
    return 0;
}

Return<float> DS18B20::temperature()
{
    uint8_t i;
    float celsius;

    if (startErrorCode != 0)
    {
        begin();
        return Return<float>::error(startErrorCode);
    }

    wire->reset();
    wire->select(rom);
    wire->write(DS18B20_COMMAND_CONVERT_TEMPERATURE, parasitic); // start conversion, with parasite power on at the end

    delay(measuringTimeMillis()); // Use the documented timing

    // we might do a wire->depower() here, but the reset will take care of it.
    uint8_t code = readScratchpad();
    if (code != 0)
        return Return<float>::error(code);

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
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
        uint8_t cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00)
            raw = raw & ~7; // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20)
            raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40)
            raw = raw & ~1; // 11 bit res, 375 ms
                            // Default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
#ifdef DS18B20_DEBUG
    Serial.print("  Temperature = ");
    Serial.print(celsius);
    Serial.print(" Celsius, ");
    Serial.print(celsius * 1.8 + 32.0);
    Serial.println(" Fahrenheit");
#endif
    return celsius;
}

Return<float> DS18B20::lastTemperature()
{
    float celsius;
    // we might do a wire->depower() here, but the reset will take care of it.
    uint8_t code = readScratchpad();
    if (code != 0)
        return Return<float>::error(code);

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
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
        uint8_t cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00)
            raw = raw & ~7; // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20)
            raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40)
            raw = raw & ~1; // 11 bit res, 375 ms
                            // Default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
#ifdef DS18B20_DEBUG
    Serial.print("  Temperature = ");
    Serial.print(celsius);
    Serial.print(" Celsius, ");
    Serial.print(celsius * 1.8 + 32.0);
    Serial.println(" Fahrenheit");
#endif
    return celsius;
}

uint8_t DS18B20::startMeasurement()
{

    if (startErrorCode != 0)
    {
        begin();
        return startErrorCode;
    }
    wire->reset();
    wire->select(rom);
    wire->write(DS18B20_COMMAND_CONVERT_TEMPERATURE, parasitic); // start conversion, with parasite power on at the end
    _measuring = true;
    measurementStarted = millis();
    return 0;
}

bool DS18B20::measuring()
{
    return _measuring;
}

bool DS18B20::handle()
{
    if (_measuring && millis() - measurementStarted > measuringTimeMillis())
    {
        _measuring = false;
    }
    return true;
}
