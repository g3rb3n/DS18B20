#include "DS18B20.h"

using namespace g3rb3n;

DS18B20::DS18B20(uint8_t _pin)
:
wire(new OneWire(_pin))
{
  
}

bool DS18B20::begin()
{
  int count = 0;
  byte addr[8];

  while ( !wire->search(addr)) 
  {
    wire->reset_search();
    delay(250);
    if (count++ >= 10)
    {
      return false;
    }
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) 
  {
    return false;
  }

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10: return true;
    case 0x28: return true;
    case 0x22: return true;
    default: return false;
  } 
  return true;
}


Return<float> DS18B20::temperature()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  int count = 0;
  while ( !wire->search(addr)) {
//    Serial.println("No more addresses.");
//    Serial.println();
    wire->reset_search();
    delay(250);
    if (count++ >= 10)
      return Return<float>::error(1);
  }
  
//  Serial.print("ROM =");
//  for( i = 0; i < 8; i++) {
//    Serial.write(' ');
//    Serial.print(addr[i], HEX);
//  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return Return<float>::error(2);
  }
//  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
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
      Serial.println("Device is not a DS18x20 family device.");
      return Return<float>::error(3);
  } 

  wire->reset();
  wire->select(addr);
  wire->write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a wire->depower() here, but the reset will take care of it.
  
  present = wire->reset();
  wire->select(addr);    
  wire->write(0xBE);         // Read Scratchpad

//  Serial.print("  Data = ");
//  Serial.print(present, HEX);
//  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = wire->read();
//    Serial.print(data[i], HEX);
//    Serial.print(" ");
  }
//  Serial.print(" CRC=");
//  Serial.print(OneWire::crc8(data, 8), HEX);
//  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
//  Serial.print("  Temperature = ");
//  Serial.print(celsius);
//  Serial.print(" Celsius, ");
//  Serial.print(fahrenheit);
//  Serial.println(" Fahrenheit");
  return celsius;
}
