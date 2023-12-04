#include "DS18B20.h"
#include "Return.h"

using namespace g3rb3n;

#define RESOLUTION 12

DS18B20 sensor(D2);

float value;

void setup()
{
    Serial.begin(230400);
    Serial.println();
    uint8_t code = sensor.begin();
    if (code > 0)
        Serial.println(String("Error ") + code);
}

void loop()
{
    sensor.handle();
    if (sensor.measuring())
        return;
    nonBlockingMeasurementEnd();
    nonBlockingMeasurementStart();
}

void nonBlockingMeasurementEnd()
{
    Return<float> temperature = sensor.lastTemperature();
    if (temperature.valid())
        Serial.println(String(millis()) + String(" read ") + static_cast<float>(temperature));
    else
        Serial.println(String("Error ") + temperature.code());
}

void nonBlockingMeasurementStart()
{
    sensor.startMeasurement();
}