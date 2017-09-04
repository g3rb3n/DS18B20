#include <DS18B20.h>

using namespace g3rb3n;

DS18B20 ds(D1);

float value;

void setup() 
{
  Serial.begin(230400);
  Serial.println();
}

void loop()
{
  Return<float> temperature = ds.temperature();
  if (!temperature.valid())
  {
    Serial.println(String("Error ") + temperature.code());
    return;
  }
  Serial.println(String("Read ") + temperature);
}
