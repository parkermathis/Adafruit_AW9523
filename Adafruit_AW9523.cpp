/*!
 *  @file Adafruit_AW9523.cpp
 *
 *  @mainpage Adafruit AW9523 Battery Monitor library
 *
 *  @section intro_sec Introduction
 *
 * 	I2C Driver for the Adafruit AW9523 Battery Monitor library
 *
 * 	This is a library for the Adafruit AW9523 breakout:
 * 	https://www.adafruit.com/product/4712
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *  @section dependencies Dependencies
 *  This library depends on the Adafruit BusIO library
 *
 *  @section author Author
 *
 *  Limor Fried (Adafruit Industries)
 *
 * 	@section license License
 *
 * 	BSD (see license.txt)
 *
 * 	@section  HISTORY
 *
 *     v1.0 - First release
 */

#include "Arduino.h"

#include "Adafruit_AW9523.h"

/*!
 *    @brief  Instantiates a new AW9523 class
 */
Adafruit_AW9523::Adafruit_AW9523(void) {}

Adafruit_AW9523::~Adafruit_AW9523(void) {}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  addr The I2C address for the expander
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @return True if initialization was successful, otherwise false.
 */
bool Adafruit_AW9523::begin(uint8_t addr, TwoWire *wire) {
  if (i2c_dev) {
    delete i2c_dev; // remove old interface
  }

  i2c_dev = new Adafruit_I2CDevice(addr, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  if (!reset()) {
    return false;
  }

  Adafruit_I2CRegister idreg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_CHIPID);
  if (idreg.read() != 0x23) {
    return false;
  }

  configureDirection(0x0); // all inputs!
  openDrainPort0(false);   // push pull default
  interruptEnableGPIO(0);  // no interrupt

  return true;
}

/*!
 *    @brief  Perform a soft reset over I2C
 *    @return True I2C reset command was acknowledged
 */
bool Adafruit_AW9523::reset(void) {
  Adafruit_I2CRegister resetreg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_SOFTRESET);

  return resetreg.write(0);
}

/*!
 *    @brief  Sets output value (1 == high) for all 16 GPIO
 *    @param  pins 16-bits of binary output settings
 *    @return True I2C write command was acknowledged
 */
bool Adafruit_AW9523::outputGPIO(uint16_t pins) {
  Adafruit_I2CRegister output0reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_OUTPUT0, 2, LSBFIRST);
  Adafruit_I2CRegister output1reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_OUTPUT1, 2, LSBFIRST);

  if (!output0reg.write(pins) || !output1reg.write(pins)) {
    return false;
  }
  return true;
}

/*!
 *    @brief  Reads input value (1 == high) for all 16 GPIO
 *    @return 16-bits of binary input (0 == low & 1 == high)
 */
uint16_t Adafruit_AW9523::inputGPIO(void) {
  Adafruit_I2CRegister input0reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INPUT0, 2, LSBFIRST);
  Adafruit_I2CRegister input1reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INPUT1, 2, LSBFIRST);

  uint16_t out = (input1reg.read() << 8) | input0reg.read();
  return out;
}

/*!
 *    @brief  Sets interrupt watch for all 16 GPIO, 1 == enabled
 *    @param  pins 16-bits of pins we may want INT enabled
 *    @return True I2C write command was acknowledged
 */
bool Adafruit_AW9523::interruptEnableGPIO(uint16_t pins) {
  Adafruit_I2CRegister int0reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INTENABLE0, 2, LSBFIRST);
  Adafruit_I2CRegister int1reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INTENABLE1, 2, LSBFIRST);

  if (!int0reg.write(~pins) || !int1reg.write(~(pins >> 8))) {
    return false;
  }
  return true;
}

/*!
 *    @brief  Sets direction for all 16 GPIO, 1 == output, 0 == input
 *    @param  pins 16-bits of pins we want to set direction
 *    @return True I2C write command was acknowledged
 */
bool Adafruit_AW9523::configureDirection(uint16_t pins) {
  Adafruit_I2CRegister conf0reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_CONFIG0, 2, LSBFIRST);
  Adafruit_I2CRegister conf1reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_CONFIG1, 2, LSBFIRST);

  if (!conf0reg.write(~pins) || !conf1reg.write(~(pins >> 8))) {
    return false;
  }

  return true;
}

/*!
 *    @brief  Sets constant-current mode for all 16 GPIO
 *    @param  pins 16-bits of pins we want to set CC mode (1 == enabled)
 *    @return True I2C write command was acknowledged
 */
bool Adafruit_AW9523::configureLEDMode(uint16_t pins) {
  Adafruit_I2CRegister ledmode0reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_LEDMODE0, 2, LSBFIRST);
  Adafruit_I2CRegister ledmode1reg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_LEDMODE1, 2, LSBFIRST);

  if (!ledmode0reg.write(~pins) || !ledmode1reg.write(~(pins >> 8))) {
    return false;
  }

  return true;
}

/*!
 *    @brief  Sets constant-current setting for one pin
 *    @param  pin GPIO to set, from 0 to 15 inclusive
 *    @param  val Ratio to set, from 0 (off) to 255 (max current)
 */
void Adafruit_AW9523::analogWrite(uint8_t pin, uint8_t val) {
  uint8_t reg;

  // See Table 13. 256 step dimming control register
  if ((pin >= 0) && (pin <= 7)) {
    reg = 0x24 + pin;
  }
  if ((pin >= 8) && (pin <= 11)) {
    reg = 0x20 + pin - 8;
  }
  if ((pin >= 12) && (pin <= 15)) {
    reg = 0x2C + pin - 12;
  }

  Adafruit_I2CRegister ledCCreg = Adafruit_I2CRegister(i2c_dev, reg);

  ledCCreg.write(val);
}

/*!
 *    @brief  Sets digital output for one pin
 *    @param  pin GPIO to set, from 0 to 15 inclusive
 *    @param  val True for high value, False for low value
 */
void Adafruit_AW9523::digitalWrite(uint8_t pin, bool val) {
  Adafruit_I2CRegister output0reg =
      Adafruit_I2CRegister(i2c_dev, AW9523_REG_OUTPUT0, 2, LSBFIRST);
  Adafruit_I2CRegisterBits outbit =
      Adafruit_I2CRegisterBits(&output0reg, 1, pin); // # bits, bit_shift

  outbit.write(val);
}

/*!
 *    @brief  Reads digital input for one pin
 *    @param  pin GPIO to set, from 0 to 15 inclusive
 *    @returns True for high value read, False for low value read
 */
bool Adafruit_AW9523::digitalRead(uint8_t pin) {

  Adafruit_I2CRegister inputreg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INPUT0, 2, LSBFIRST);
  uint8_t outPin = pin;

  if (pin >= 8) {
    inputreg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INPUT1, 2, LSBFIRST);
    outPin = pin - 8;
  }

  Adafruit_I2CRegisterBits inbit = Adafruit_I2CRegisterBits(&inputreg, 1, outPin); // # bits, bit_shift
  return inbit.read();
}

/*!
 *    @brief  Sets interrupt enable for one pin
 *    @param  pin GPIO to set, from 0 to 15 inclusive
 *    @param  en True to enable Interrupt detect, False for ignore
 */
void Adafruit_AW9523::enableInterrupt(uint8_t pin, bool en) {
  Adafruit_I2CRegister intenablereg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INTENABLE0, 2, LSBFIRST);

  if (pin >= 8) {
    intenablereg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_INTENABLE1, 2, LSBFIRST);
  }
      
  Adafruit_I2CRegisterBits irqbit = Adafruit_I2CRegisterBits(&intenablereg, 1, pin); // # bits, bit_shift
  irqbit.write(!en);
}

/*!
 *    @brief  Sets pin mode / direction for one pin
 *    @param  pin GPIO to set, from 0 to 15 inclusive
 *    @param  mode Can be INPUT, OUTPUT for GPIO digital, or AW9523_LED_MODE for
 * constant current LED drive
 */
void Adafruit_AW9523::pinMode(uint8_t pin, uint8_t mode) {
  // GPIO Direction
  Adafruit_I2CRegister confreg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_CONFIG0, 2, LSBFIRST);
  // GPIO mode or LED mode?
  Adafruit_I2CRegister ledmodereg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_LEDMODE0, 2, LSBFIRST);
  uint8_t outPin = pin;

  if (pin >= 8) {
    confreg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_CONFIG1, 2, LSBFIRST);
    ledmodereg = Adafruit_I2CRegister(i2c_dev, AW9523_REG_LEDMODE1, 2, LSBFIRST);
    outPin = pin - 8;
  }
      
  Adafruit_I2CRegisterBits confbit = Adafruit_I2CRegisterBits(&confreg, 1, outPin); // # bits, bit_shift
  Adafruit_I2CRegisterBits modebit = Adafruit_I2CRegisterBits(&ledmodereg, 1, outPin); // # bits, bit_shift

  if (mode == OUTPUT) {
    confbit.write(0);
    modebit.write(1);
  }
  if (mode == INPUT) {
    confbit.write(1);
    modebit.write(1);
  }

  if (mode == AW9523_LED_MODE) {
    confbit.write(0);
    modebit.write(0);
  }
}

/*!
 *    @brief  Turns on/off open drain output for ALL port 0 pins (GPIO 0-7)
 *    @param  od True to enable open drain, False for push-pull
 *    @return True I2C write command was acknowledged
 */
bool Adafruit_AW9523::openDrainPort0(bool od) {
  Adafruit_I2CRegister gcrreg =
      Adafruit_I2CRegister(i2c_dev, AW9523_REG_GCR, 1);

  Adafruit_I2CRegisterBits opendrain =
      Adafruit_I2CRegisterBits(&gcrreg, 1, 4); // # bits, bit_shift

  return opendrain.write(!od);
}
