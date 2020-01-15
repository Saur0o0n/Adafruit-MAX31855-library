/***************************************************
  This is a library for the Adafruit Thermocouple Sensor w/MAX31855K

  Designed specifically to work with the Adafruit Thermocouple Sensor
  ----> https://www.adafruit.com/products/269

  These displays use SPI to communicate, 3 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#ifndef ADAFRUIT_MAX31855_H
#define ADAFRUIT_MAX31855_H

#include <SPI.h>

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

class Adafruit_MAX31855 {
 public:
  Adafruit_MAX31855(int8_t _sclk, int8_t _cs, int8_t _miso);
  Adafruit_MAX31855(int8_t _cs);

<<<<<<< HEAD
  void begin(SPIClass *SPI_pointer = &SPI);
=======
  void begin(SPIClass *SPI_Pointer = &SPI);
>>>>>>> c026395412c302e18e934822e375d4983cd82796
  double readInternal(void);
  double readCelsius(void);
  double readFarenheit(void);
  uint8_t readError();
  // advanced functions
  uint32_t readRaw(void);
  double   decodeCelsius(uint32_t rawData);
  double   decodeInternal(uint32_t rawData);
  double   linearizeCelcius(double internalTemp, double rawTemp);

 private:
  boolean initialized;
  SPIClass * MAXSPI = NULL;

  int8_t sclk, miso, cs;
  uint32_t spiread32(void);
};

#endif
