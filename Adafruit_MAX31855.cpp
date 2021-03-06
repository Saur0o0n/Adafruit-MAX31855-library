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

#include "Adafruit_MAX31855.h"
#ifdef __AVR
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#endif

#include <stdlib.h>
#include <SPI.h>


Adafruit_MAX31855::Adafruit_MAX31855(int8_t _sclk, int8_t _cs, int8_t _miso) {
  sclk = _sclk;
  cs = _cs;
  miso = _miso;

  initialized = false;
}

Adafruit_MAX31855::Adafruit_MAX31855(int8_t _cs) {
  cs = _cs;
  sclk = miso = -1;

  initialized = false;
}

void Adafruit_MAX31855::begin(SPIClass *SPI_pointer) {
  //define pin modes
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);

  MAXSPI = SPI_pointer;

  if (sclk == -1) {
    // hardware SPI
    //start and configure hardware SPI
    MAXSPI->begin();
  } else {
    pinMode(sclk, OUTPUT);
    pinMode(miso, INPUT);
  }
  initialized = true;
}

double Adafruit_MAX31855::readInternal(void) {
  uint32_t v;

  v = spiread32();

  //Serial.print("0x"); Serial.println(v, HEX);		// DEBUG

  if (v & 0x7 || v == 0x0) {
    // uh oh, a serious problem!
    return NAN;
  }

  // ignore bottom 4 bits - they're just thermocouple data
  v >>= 4;

  // pull the bottom 11 bits off
  float internal = v & 0x7FF;
  // check sign bit!
  if (v & 0x800) {
    // Convert to negative value by extending sign and casting to signed type.
    int16_t tmp = 0xF800 | (v & 0x7FF);
    internal = tmp;
  }
  internal *= 0.0625; // LSB = 0.0625 degrees
  //Serial.print("\tInternal Temp: "); Serial.println(internal);
  return internal;
}

double Adafruit_MAX31855::readCelsius(void) {
  return decodeCelsius(spiread32());
}

uint8_t Adafruit_MAX31855::readError() {
  return spiread32() & 0x7;
}

double Adafruit_MAX31855::readFarenheit(void) {
  float f = readCelsius();
  f *= 9.0;
  f /= 5.0;
  f += 32;
  return f;
}

// Begin: Additional functions
//
uint32_t Adafruit_MAX31855::readRaw(void) {
  // spiread32() made Public
  // returns the unprocessed 32 bits of the MAX31855's internal memory
  int32_t v = spiread32();

  if (v & 0x7 || v == 0x0) {
    // we can't return NAN for integer - return 0
    return 0;
  }
  return v;
}

double Adafruit_MAX31855::decodeCelsius(uint32_t rawData){
  int32_t v = rawData;

  //v = spiread32();

  //Serial.print("0x"); Serial.println(v, HEX);		// DEBUG

  if (v & 0x7) {
    // uh oh, a serious problem!
    return NAN;
  }

  if (v & 0x80000000) {
    // Negative value, drop the lower 18 bits and explicitly extend sign bits.
    v = 0xFFFFC000 | ((v >> 18) & 0x00003FFF);
  }
  else {
    // Positive value, just drop the lower 18 bits.
    v >>= 18;
  }
  //Serial.println(v, HEX);

  double centigrade = v;

  // LSB = 0.25 degrees C
  centigrade *= 0.25;
  return centigrade;
}

double Adafruit_MAX31855::decodeInternal(uint32_t rawData) {
  // Same as readInternal() but you pass it the value from readRaw()
  uint32_t v = rawData;

  //v = spiread32();

  if (v & 0x7) {
    // uh oh, a serious problem!
    return NAN;
  }

  // ignore bottom 4 bits - they're just thermocouple data
  v >>= 4;

  // pull the bottom 11 bits off
  float internal = v & 0x7FF;
  // check sign bit!
  if (v & 0x800) {
    // Convert to negative value by extending sign and casting to signed type.
    int16_t tmp = 0xF800 | (v & 0x7FF);
    internal = tmp;
  }
  internal *= 0.0625; // LSB = 0.0625 degrees
  //Serial.print("\tInternal Temp: "); Serial.println(internal);
  return internal;
}

double Adafruit_MAX31855::linearizeCelcius(double internalTemp, double rawTemp) {
  //////////////////////////////////////////////////////////////////////////
  // Function:       Takes the thermocouple temperature measurement and
  //                   corrects the linearity errors.
  // Input:
  //   double internalTemp  = value from readInternal()
  //   double rawTemp       = value from readCelsius()
  //
  // Returns:
  //   double               = Corrected Celsius thermocouple temperature
  //////////////////////////////////////////////////////////////////////////
  // Adapted from https://github.com/heypete/MAX31855-Linearization
  // Equations and coefficients from: https://srdata.nist.gov/its90/main/its90_main_page.html
  // Retrieved 2018-09-01

  // Initialize variables.
  int i = 0; // Counter for arrays
  //double internalTemp = thermocouple.readInternal(); // Read the internal temperature of the MAX31855.
  //double rawTemp = thermocouple.readCelsius(); // Read the temperature of the thermocouple. This temp is compensated for cold junction temperature.
  double thermocoupleVoltage = 0.0;
  double internalVoltage = 0.0;
  double correctedTemp = 0.0;
  double totalVoltage = 0.0;

  // Check to make sure thermocouple is working correctly.
  if (isnan(rawTemp)) {
    //Serial.println(F("Something wrong with thermocouple!"));
    return NAN;
  }
  else {
    // Steps 1 & 2. Subtract cold junction temperature from the raw thermocouple temperature.
    //              Convert to mV
    thermocoupleVoltage = (rawTemp - internalTemp) * 0.041276; // C * mv/C = mV

    // Step 3. Calculate the cold junction equivalent thermocouple voltage.

    // Lightened in relation to this post:
    // https://forums.adafruit.com/viewtopic.php?f=19&t=32086&start=75#p579783
    internalVoltage = internalTemp * 0.04073 ; // (mV) MAX38155 comp scale is 40.73 uV/C

    // Step 4. Add the cold junction equivalent thermocouple voltage calculated in step 3 to the thermocouple voltage calculated in step 2.
    totalVoltage = thermocoupleVoltage + internalVoltage;

    // Step 5. Use the result of step 4 and the NIST voltage-to-temperature (inverse) coefficients to calculate the cold junction compensated, linearized temperature value.
    // The equation is in the form correctedTemp = d_0 + d_1*E + d_2*E^2 + ... + d_n*E^n, where E is the totalVoltage in mV and correctedTemp is in degrees C.
    // NIST uses different coefficients for different temperature subranges: (-200 to 0C), (0 to 500C) and (500 to 1372C).
    if (totalVoltage < 0) { // Temperature is between -200 and 0C.
      double d[] = {0.0000000E+00, 2.5173462E+01, -1.1662878E+00, -1.0833638E+00, -8.9773540E-01, -3.7342377E-01, -8.6632643E-02, -1.0450598E-02, -5.1920577E-04, 0.0000000E+00};

      int dLength = sizeof(d) / sizeof(d[0]);
      for (i = 0; i < dLength; i++) {
        correctedTemp += d[i] * pow(totalVoltage, i);
      }
    }
    else if (totalVoltage < 20.644) { // Temperature is between 0C and 500C.
      double d[] = {0.000000E+00, 2.508355E+01, 7.860106E-02, -2.503131E-01, 8.315270E-02, -1.228034E-02, 9.804036E-04, -4.413030E-05, 1.057734E-06, -1.052755E-08};
      int dLength = sizeof(d) / sizeof(d[0]);
      for (i = 0; i < dLength; i++) {
        correctedTemp += d[i] * pow(totalVoltage, i);
      }
    }
    else if (totalVoltage < 54.886 ) { // Temperature is between 500C and 1372C.
      double d[] = { -1.318058E+02, 4.830222E+01, -1.646031E+00, 5.464731E-02, -9.650715E-04, 8.802193E-06, -3.110810E-08, 0.000000E+00, 0.000000E+00, 0.000000E+00};
      int dLength = sizeof(d) / sizeof(d[0]);
      for (i = 0; i < dLength; i++) {
        correctedTemp += d[i] * pow(totalVoltage, i);
      }
    } else { // NIST only has data for K-type thermocouples from -200C to +1372C. If the temperature is not in that range, set temp to impossible value.
      // Error handling should be improved.
      Serial.print(F("Temperature is out of range. This should never happen."));
      correctedTemp = NAN;
    }
    return correctedTemp;
    //Serial.print(F("Corrected Temp = "));  // DEBUG
    //Serial.println(correctedTemp, 5);  // DEBUG
    //Serial.println(F(""));  // DEBUG

  }
}
// End: Additional Functions
//

uint32_t Adafruit_MAX31855::spiread32(void) {
  int i;
  uint32_t d = 0;

  // backcompatibility!
  if (! initialized) {
    begin();
  }

  digitalWrite(cs, LOW);
  delay(1);

  if(sclk == -1) {
    // hardware SPI

    MAXSPI->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    d = MAXSPI->transfer(0);
    d <<= 8;
    d |= MAXSPI->transfer(0);
    d <<= 8;
    d |= MAXSPI->transfer(0);
    d <<= 8;
    d |= MAXSPI->transfer(0);

    MAXSPI->endTransaction();
  } else {
    // software SPI

    digitalWrite(sclk, LOW);
    delay(1);

    for (i=31; i>=0; i--) {
      digitalWrite(sclk, LOW);
      delay(1);
      d <<= 1;
      if (digitalRead(miso)) {
	d |= 1;
      }

      digitalWrite(sclk, HIGH);
      delay(1);
    }
  }

  digitalWrite(cs, HIGH);
  //Serial.println(d, HEX);
  return d;
}
