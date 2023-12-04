#include "DS18B20.h"
#include "Return.h"

using namespace g3rb3n;

#define RESOLUTION 12

DS18B20 sensor(D2);

float value;

String chip(uint8_t family)
{
    switch (family)
    {
    case 0x10:
        return String("DS18S20"); // or old DS1820
    case 0x28:
        return String("DS18B20");
    case 0x22:
        return String("DS1822");
    }
    return String("UNKNOWN");
}

void setup()
{
    Serial.begin(230400);
    Serial.println();
    uint8_t code = sensor.begin();
    if (code > 0)
        Serial.println(String("Error ") + code);
    Serial.println("Found chip family " + chip(sensor.family()));
    Serial.println("Serial " + sensor.serialNumber());
    Serial.println("Resolution " + String(sensor.resolution()));
    Serial.println("Conversion time " + String(sensor.measuringTimeMillis()) + "ms");
    Serial.println();

    sensor.resolution(RESOLUTION);
    Serial.println("Resolution " + String(sensor.resolution()));
}

void loop()
{
    sensor.handle();
    if (sensor.measuring())
        return;
    nonBlockingMeasurementEnd();
    handleInput();
    nonBlockingMeasurementStart();
}

void handleInput()
{
    if (Serial.available())
    {
        char c = Serial.read();
        Serial.print("Got ");
        Serial.println(c);
        if (c < 49 || c > 52)
        {
            Serial.println("!!! Wrong command");
            Serial.println("!!! 1 for 9 bit resolution");
            Serial.println("!!! 2 for 10 bit resolution");
            Serial.println("!!! 3 for 11 bit resolution");
            Serial.println("!!! 4 for 12 bit resolution");
            return;
        }
        uint8_t resolution = c - 49 + 9;
        Serial.print("Set ");
        Serial.println(resolution);
        sensor.resolution(resolution);
        Serial.println("Resolution " + String(sensor.resolution()));
    }
}

void blockingMeasurement()
{
    Return<float> temperature = sensor.temperature();
    if (!temperature.valid())
    {
        Serial.println(String("Error ") + temperature.code());
        return;
    }
    Serial.println(String("Read ") + static_cast<float>(temperature));
}

void nonBlockingMeasurementEnd()
{
    Return<float> temperature = sensor.lastTemperature();
    if (!temperature.valid())
    {
        Serial.println(String("Error ") + temperature.code());
        return;
    }
    Serial.println(String(millis()) + String(" read ") + static_cast<float>(temperature));
}

void nonBlockingMeasurementStart()
{
    sensor.startMeasurement();
}