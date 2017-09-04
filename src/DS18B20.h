#ifndef _DS18B20_H
#define _DS18B20_H

#include "Return.h"
#include <OneWire.h>

namespace g3rb3n
{
  class DS18B20
  {
    private:
      OneWire* wire;
      
    public:
      DS18B20(uint8_t _pin);
      bool begin();
      Return<float> temperature();
  };
}

#endif
