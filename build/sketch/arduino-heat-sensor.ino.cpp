#include <Arduino.h>
#line 1 "c:\\Users\\chris\\Documents\\Github\\arduino-heat-sensor\\arduino-heat-sensor.ino"
// Please credit chris.keith@gmail.com .

#include <Wire.h>

//The default I2C address for the THING on the SparkX breakout is 0x69. 0x68 is also possible.
#define DEFAULT_ADDRESS 0x69

//Platform specific configurations

//Define the size of the I2C buffer based on the platform the user has
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)

//I2C_BUFFER_LENGTH is defined in Wire.H
#define I2C_BUFFER_LENGTH BUFFER_LENGTH

#elif defined(__SAMD21G18A__)

//SAMD21 uses RingBuffer.h
#define I2C_BUFFER_LENGTH SERIAL_BUFFER_SIZE

#elif __MK20DX256__
//Teensy

#elif ARDUINO_ARCH_ESP32
//ESP32 based platforms

#else

//The catch-all default is 32
#define I2C_BUFFER_LENGTH 32

#endif
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Registers
#define POWER_CONTROL_REGISTER        0x00
#define RESET_REGISTER                0x01
#define FRAMERATE_REGISTER            0x02
#define INT_CONTROL_REGISTER          0x03
#define STATUS_REGISTER               0x04
#define STATUS_CLEAR_REGISTER         0x05
#define AVERAGE_REGISTER              0x07
#define INT_LEVEL_REGISTER_UPPER_LSB  0x08
#define INT_LEVEL_REGISTER_UPPER_MSB  0x09
#define INT_LEVEL_REGISTER_LOWER_LSB  0x0A
#define INT_LEVEL_REGISTER_LOWER_MSB  0x0B
#define INT_LEVEL_REGISTER_HYST_LSB   0x0C
#define INT_LEVEL_REGISTER_HYST_MSB   0x0D
#define THERMISTOR_REGISTER_LSB       0x0E
#define THERMISTOR_REGISTER_MSB       0x0F
#define INT_TABLE_REGISTER_INT0       0x10
#define RESERVED_AVERAGE_REGISTER     0x1F
#define TEMPERATURE_REGISTER_START    0x80

class GridEYE {
  public:

    // Return values

    //By default use the default I2C addres, and use Wire port
    void begin(uint8_t deviceAddress = DEFAULT_ADDRESS, TwoWire &wirePort = Wire);

	float getPixelTemperature(unsigned char pixelAddr);
	int16_t getPixelTemperatureRaw(unsigned char pixelAddr);
	float getPixelTemperatureFahrenheit(unsigned char pixelAddr);
	
	float getDeviceTemperature();
	int16_t getDeviceTemperatureRaw();
	float getDeviceTemperatureFahrenheit();	
	
	void setFramerate1FPS();
	void setFramerate10FPS();
	bool isFramerate10FPS();
	
	void wake();
	void sleep();
	void standby60seconds();
	void standby10seconds();
	
	void interruptPinEnable();
	void interruptPinDisable();
	void setInterruptModeAbsolute();
	void setInterruptModeDifference();
	bool interruptPinEnabled();
	
	bool interruptFlagSet();
	bool pixelTemperatureOutputOK();
	bool deviceTemperatureOutputOK();
	void clearInterruptFlag();
	void clearPixelTemperatureOverflow();
	void clearDeviceTemperatureOverflow();
	void clearAllOverflow();
	void clearAllStatusFlags();
	
	bool pixelInterruptSet(uint8_t pixelAddr);
	
	void movingAverageEnable();
	void movingAverageDisable();
	bool movingAverageEnabled();
	
	void setUpperInterruptValue(float DegreesC);
	void setUpperInterruptValueRaw(int16_t regValue);
	void setUpperInterruptValueFahrenheit(float DegreesF);
	
	void setLowerInterruptValue(float DegreesC);
	void setLowerInterruptValueRaw(int16_t regValue);
	void setLowerInterruptValueFahrenheit(float DegreesF);
	
	void setInterruptHysteresis(float DegreesC);
	void setInterruptHysteresisRaw(int16_t regValue);
	void setInterruptHysteresisFahrenheit(float DegreesF);
	
	float getUpperInterruptValue();
	int16_t getUpperInterruptValueRaw();
	float getUpperInterruptValueFahrenheit();
	
	float getLowerInterruptValue();
	int16_t getLowerInterruptValueRaw();
	float getLowerInterruptValueFahrenheit();
	
	float getInterruptHysteresis();
	int16_t getInterruptHysteresisRaw();
	float getInterruptHysteresisFahrenheit();
	
	void setRegister(unsigned char reg, unsigned char val);
	int16_t getRegister(unsigned char reg, int8_t len);

    void setI2CAddress(uint8_t addr); //Set the I2C address we read and write to
    uint8_t getI2CAddress(); // for diagnostics

  private:
  
    TwoWire *_i2cPort; //The generic connection to user's chosen I2C hardware
    uint8_t _deviceAddress; //Keeps track of I2C address. setI2CAddress changes this.

};


//Attempt communication with the device
//Return true if we got a 'Polo' back from Marco
void GridEYE::begin(uint8_t deviceAddress, TwoWire &wirePort)
{
  _deviceAddress = deviceAddress;
  _i2cPort = &wirePort;
}

//Change the address we read and write to
void GridEYE::setI2CAddress(uint8_t addr)
{
  _deviceAddress = addr;
}

uint8_t GridEYE::getI2CAddress()
{
  return _deviceAddress;
}

/********************************************************
 * Functions for retreiving the temperature of
 * a single pixel. 
 ********************************************************
 * 
 * getPixelTemperature() - returns float Celsius
 * 
 * getPixelTemperatureFahrenheit() - returns float Fahrenheit
 * 
 * getPixelTemperatureRaw() - returns int16_t contents of
 *    both pixel temperature registers concatinated
 *    
 ********************************************************/

float GridEYE::getPixelTemperature(unsigned char pixelAddr)
{

  // Temperature registers are numbered 128-255
  // Each pixel has a lower and higher register
  unsigned char pixelLowRegister = TEMPERATURE_REGISTER_START + (2 * pixelAddr);
  int16_t temperature = getRegister(pixelLowRegister, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesC = temperature * 0.25;

  return DegreesC;

}

float GridEYE::getPixelTemperatureFahrenheit(unsigned char pixelAddr)
{

  // Temperature registers are numbered 128-255
  // Each pixel has a lower and higher register
  unsigned char pixelLowRegister = TEMPERATURE_REGISTER_START + (2 * pixelAddr);
  int16_t temperature = getRegister(pixelLowRegister, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesF = (temperature * 0.25) * 1.8 + 32;

  return DegreesF;

}

int16_t GridEYE::getPixelTemperatureRaw(unsigned char pixelAddr)
{

  // Temperature registers are numbered 128-255
  // Each pixel has a lower and higher register
  unsigned char pixelLowRegister = TEMPERATURE_REGISTER_START + (2 * pixelAddr);
  int16_t temperature = getRegister(pixelLowRegister, 2);

  return temperature;

}

/********************************************************
 * Functions for retreiving the temperature of
 * the device according to the embedded thermistor. 
 ******************************************************** 
 * 
 * getDeviceTemperature() - returns float Celsius
 * 
 * getDeviceTemperatureFahrenheit() - returns float Fahrenheit
 * 
 * getDeviceTemperatureRaw() - returns int16_t contents of
 *    both thermistor temperature registers concatinated
 *    
 ********************************************************/

float GridEYE::getDeviceTemperature()
{

  int16_t temperature = getRegister(THERMISTOR_REGISTER_LSB, 2);

    // temperature is reported as 12-bit twos complement
    // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float realTemperature = temperature * 0.0625;

  return realTemperature;

}

float GridEYE::getDeviceTemperatureFahrenheit()
{

  int16_t temperature = getRegister(THERMISTOR_REGISTER_LSB, 2);

    // temperature is reported as 12-bit twos complement
    // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float realTemperatureF = (temperature * 0.0625) * 1.8 + 32;

  return realTemperatureF;

}

int16_t GridEYE::getDeviceTemperatureRaw()
{

  int16_t temperature = getRegister(THERMISTOR_REGISTER_LSB, 2);

  return temperature;

}

/********************************************************
 * Functions for manipulating Framerate
 ******************************************************** 
 * 
 * Internal framerate of the device is always 10fps
 * When operating in 1FPS mode, each frame is an average
 * of 10 readings.
 * 
 * setFramerate1FPS() - sets framerate to 1 Frame per Second
 * 
 * setFramerate10FPS() - sets framerate to 10 Frames per Second
 * 
 * isFramerate10FPS() - returns true if framerate is currently
 *    set to 10 Frames per Second (device default)
 *    
 ********************************************************/

void GridEYE::setFramerate1FPS()
{
    setRegister(FRAMERATE_REGISTER, 1);  
}

void GridEYE::setFramerate10FPS()
{
    setRegister(FRAMERATE_REGISTER, 0);  
}

bool GridEYE::isFramerate10FPS()
{

    if(getRegister(FRAMERATE_REGISTER, 1) == 0){
      return true;
    }else{
      return false;
    }
  
}

/********************************************************
 * Functions for manipulating Operating Mode
 ******************************************************** 
 * 
 * Device defaults to normal mode on reset. 
 * When the device is in standby mode, the temperature
 * register is only updated intermittently.
 * 
 * wake() - returns device to normal mode from any
 *    other state.
 *    
 * sleep() - puts device into sleep mode, temperature
 *    register is not updated
 * 
 * standby60seconds() - puts device into standby mode
 *    with 60 second update frequency
 *    
 * standby10seconds() - puts device into standby mode
 *    with 10 second update frequency
 *    
 ********************************************************/

void GridEYE::wake(){

    setRegister(POWER_CONTROL_REGISTER, 0x00);
  
}

void GridEYE::sleep(){

    setRegister(POWER_CONTROL_REGISTER, 0x10);
  
}

void GridEYE::standby60seconds(){

    setRegister(POWER_CONTROL_REGISTER, 0x20);
  
}

void GridEYE::standby10seconds(){

    setRegister(POWER_CONTROL_REGISTER, 0x21);
  
}

/********************************************************
 * Functions for manipulating Interrupt Control Register
 ******************************************************** 
 * 
 * interruptPinEnable() - Enable INT pin to pull low on 
 *    interrupt flag
 *    
 * interruptPinDisable() - Put INT pin into Hi-Z state
 * 
 * setInterruptModeAbsolute() - Set interrupt mode to
 *    "Absolute Value" mode
 *    
 * setInterruptModeDifference() - Set interrupt mode to
 *    "Difference" mode
 *    
 * interruptPinEnabled() - returns true if the INT pin
 *    is enabled. Returns false if INT pin is in Hi-Z 
 * 
 ********************************************************/

void GridEYE::interruptPinEnable(){

    int16_t ICRValue = getRegister(INT_CONTROL_REGISTER, 1);
    
    ICRValue |= (1 << 0);
    
    setRegister(INT_CONTROL_REGISTER, ICRValue & 0xFF);   
  
}

void GridEYE::interruptPinDisable(){

    int16_t ICRValue = getRegister(INT_CONTROL_REGISTER, 1);
    
    ICRValue &= ~(1 << 0);
    
    setRegister(INT_CONTROL_REGISTER, ICRValue & 0xFF);
  
}

void GridEYE::setInterruptModeAbsolute(){

    int16_t ICRValue = getRegister(INT_CONTROL_REGISTER, 1);
    
    ICRValue |= (1 << 1);
    
    setRegister(INT_CONTROL_REGISTER, ICRValue & 0xFF);
   
}

void GridEYE::setInterruptModeDifference(){

    int16_t ICRValue = getRegister(INT_CONTROL_REGISTER, 1);
    
    ICRValue &= ~(1 << 1);
    
    setRegister(INT_CONTROL_REGISTER, ICRValue & 0xFF);    
  
}

bool GridEYE::interruptPinEnabled(){

  int16_t ICRValue = getRegister(INT_CONTROL_REGISTER, 1);
  if(ICRValue & (1 << 0)){
    return true;
  }else{
    return false;
  }
  
}

/********************************************************
 * Functions for manipulating Status/Clear Registers
 ******************************************************** 
 * 
 * interruptFlagSet() - returns true if there is an 
 *    interrupt flag in the status register
 *    
 * pixelTemperatureOutputOK() - returns false if temperature
 *    output overflow flag is present in status register
 * 
 * deviceTemperatureOutputOK() - returns false if thermistor
 *    output overflow flag is present in status register
 *    
 * clearInterruptFlag() - clears interrupt flag in the 
 *    status register
 *    
 * clearPixelTemperatureOverflow() - clears temperature
 *    output overflow flag in status register
 *    
 * clearDeviceTemperatureOverflow() - clears thermistor
 *    output overflow flag in status register
 *    
 * clearAllOverflow() - clears both thermistor and 
 *    temperature overflow flags in status register but
 *    leaves interrupt flag untouched
 *    
 * clearAllStatusFlags() - clears all flags in status 
 *    register
 * 
 ********************************************************/

bool GridEYE::interruptFlagSet(){

  int16_t StatRegValue = getRegister(STATUS_REGISTER, 1);
  if(StatRegValue & (1 << 1)){
    return true;
  }else{
    return false;
  }  
  
}

bool GridEYE::pixelTemperatureOutputOK(){

  int16_t StatRegValue = getRegister(STATUS_REGISTER, 1);
  if(StatRegValue & (1 << 2)){
    return false;
  }else{
    return true;
  }  
  
}

bool GridEYE::deviceTemperatureOutputOK(){

  int16_t StatRegValue = getRegister(STATUS_REGISTER, 1);
  if(StatRegValue & (1 << 3)){
    return false;
  }else{
    return true;
  }  
  
}

void GridEYE::clearInterruptFlag(){
    
    setRegister(STATUS_CLEAR_REGISTER, 0x02); 
  
}

void GridEYE::clearPixelTemperatureOverflow(){
    
    setRegister(STATUS_CLEAR_REGISTER, 0x04); 
  
}

void GridEYE::clearDeviceTemperatureOverflow(){
    
    setRegister(STATUS_CLEAR_REGISTER, 0x08); 
  
}

void GridEYE::clearAllOverflow(){

    setRegister(STATUS_CLEAR_REGISTER, 0x0C); 
  
}

void GridEYE::clearAllStatusFlags(){

    setRegister(STATUS_CLEAR_REGISTER, 0x0E); 
  
}

/********************************************************
 * Function for reading Interrupt Table Register
 ******************************************************** 
 * 
 * pixelInterruptSet() - Returns true if interrupt flag 
 * is set for the specified pixel
 * 
 ********************************************************/

bool GridEYE::pixelInterruptSet(uint8_t pixelAddr){

  unsigned char interruptTableRegister = INT_TABLE_REGISTER_INT0 + (pixelAddr / 8);
  uint8_t pixelPosition = (pixelAddr % 8);

  int16_t interruptTableRow = getRegister(interruptTableRegister, 1);

  if(interruptTableRow & (1 << pixelPosition)){
    return true;
  }else{
    return false;
  }    

}

/********************************************************
 * Functions for manipulating Average Register
 ******************************************************** 
 * 
 * Moving Average Mode enable and disable are only 
 * referenced in some of the documentation for this 
 * device but not in all documentation. Requires writing
 * in sequence to a reserved register. I'm not sure it 
 * does anything.
 * 
 * movingAverageEnable() - enable "Twice Moving Average" 
 * 
 * movingAverageDisable() - disable "Twice Moving Average"
 * 
 * movingAverageEnabled() - returns true if enabled
 * 
 ********************************************************/

void GridEYE::movingAverageEnable(){

    setRegister(RESERVED_AVERAGE_REGISTER, 0x50); 
    setRegister(RESERVED_AVERAGE_REGISTER, 0x45); 
    setRegister(RESERVED_AVERAGE_REGISTER, 0x57); 
    setRegister(AVERAGE_REGISTER, 0x20); 
    setRegister(RESERVED_AVERAGE_REGISTER, 0x00); 
  
}

void GridEYE::movingAverageDisable(){

    setRegister(RESERVED_AVERAGE_REGISTER, 0x50); 
    setRegister(RESERVED_AVERAGE_REGISTER, 0x45); 
    setRegister(RESERVED_AVERAGE_REGISTER, 0x57); 
    setRegister(AVERAGE_REGISTER, 0x00); 
    setRegister(RESERVED_AVERAGE_REGISTER, 0x00); 
  
}

bool GridEYE::movingAverageEnabled(){

  int16_t AVGRegValue = getRegister(AVERAGE_REGISTER, 1);
  if(AVGRegValue & (1 << 5)){
    return true;
  }else{
    return false;
  }  
  
}

/********************************************************
 * Functions for manipulating Interrupt Level Register
 ******************************************************** 
 * 
 * setUpperInterruptValue() - accepts float Celsius 
 * 
 * setUpperInterruptValueRaw() - accepts int16_t register
 *    configuration
 * 
 * setUpperInterruptValueFahrenheit() - accepts float 
 *    Fahrenheit
 *  
 * setLowerInterruptValue() - accepts float Celsius 
 * 
 * setLowerInterruptValueRaw() - accepts int16_t register
 *    configuration
 * 
 * setLowerInterruptValueFahrenheit() - accepts float 
 *    Fahrenheit
 * 
 * setInterruptHysteresis() - accepts float Celsius
 * 
 * setInterruptHysteresisRaw() - accepts int16_t register
 *    configuration
 * 
 * setInterruptHysteresisFahrenheit() - accepts float 
 *    Fahrenheit
 *    
 * getUpperInterruptValue() - returns float Celsius 
 * 
 * getUpperInterruptValueRaw() - returns int16_t register
 *    contents
 * 
 * getUpperInterruptValueFahrenheit() - returns float 
 *    Fahrenheit
 *  
 * getLowerInterruptValue() - returns float Celsius 
 * 
 * getLowerInterruptValueRaw() - returns int16_t register
 *    contents
 * 
 * getLowerInterruptValueFahrenheit() - returns float 
 *    Fahrenheit
 * 
 * getInterruptHysteresis() - returns float Celsius
 * 
 * getInterruptHysteresisRaw() - returns int16_t register
 *    contents
 * 
 * getInterruptHysteresisFahrenheit() - returns float 
 *    Fahrenheit   
 * 
 ********************************************************/

void GridEYE::setUpperInterruptValue(float DegreesC){

  bool isNegative = false;

  if(DegreesC < 0){
    DegreesC = abs(DegreesC);
    isNegative = true;
  }
  
  int16_t temperature = 0;  
  temperature = round(DegreesC*4);

  if(isNegative){
    temperature = 0 - temperature;
    temperature |= (1 << 11);
  }
  
  setRegister(INT_LEVEL_REGISTER_UPPER_LSB, temperature & 0xFF);
  setRegister(INT_LEVEL_REGISTER_UPPER_MSB, temperature >> 8);
  
}

void GridEYE::setUpperInterruptValueRaw(int16_t regValue){
  
  setRegister(INT_LEVEL_REGISTER_UPPER_LSB, regValue & 0xFF);
  setRegister(INT_LEVEL_REGISTER_UPPER_MSB, regValue >> 8);
  
}

void GridEYE::setUpperInterruptValueFahrenheit(float DegreesF){

  bool isNegative = false;

  float DegreesC = (DegreesF - 32) / 1.8;

  if(DegreesC < 0){
    DegreesC = abs(DegreesC);
    isNegative = true;
  }
  
  int16_t temperature = 0;  
  temperature = round(DegreesC*4);

  if(isNegative){
    temperature = 0 - temperature;
    temperature |= (1 << 11);
  }
  
  setRegister(INT_LEVEL_REGISTER_UPPER_LSB, temperature & 0xFF);
  setRegister(INT_LEVEL_REGISTER_UPPER_MSB, temperature >> 8);
  
}

void GridEYE::setLowerInterruptValue(float DegreesC){

  bool isNegative = false;

  if(DegreesC < 0){
    DegreesC = abs(DegreesC);
    isNegative = true;
  }
  
  int16_t temperature = 0;  
  temperature = round(DegreesC*4);

  if(isNegative){
    temperature = 0 - temperature;
    temperature |= (1 << 11);
  }
  
  setRegister(INT_LEVEL_REGISTER_LOWER_LSB, temperature & 0xFF);
  setRegister(INT_LEVEL_REGISTER_LOWER_MSB, temperature >> 8);
  
}

void GridEYE::setLowerInterruptValueRaw(int16_t regValue){

  setRegister(INT_LEVEL_REGISTER_LOWER_LSB, regValue & 0xFF);
  setRegister(INT_LEVEL_REGISTER_LOWER_MSB, regValue >> 8);
  
}

void GridEYE::setLowerInterruptValueFahrenheit(float DegreesF){

  bool isNegative = false;

  float DegreesC = (DegreesF - 32) / 1.8;

  if(DegreesC < 0){
    DegreesC = abs(DegreesC);
    isNegative = true;
  }
  
  int16_t temperature = 0;  
  temperature = round(DegreesC*4);

  if(isNegative){
    temperature = 0 - temperature;
    temperature |= (1 << 11);
  }
  
  setRegister(INT_LEVEL_REGISTER_LOWER_LSB, temperature & 0xFF);
  setRegister(INT_LEVEL_REGISTER_LOWER_MSB, temperature >> 8);
  
}

void GridEYE::setInterruptHysteresis(float DegreesC){

  bool isNegative = false;

  if(DegreesC < 0){
    DegreesC = abs(DegreesC);
    isNegative = true;
  }
  
  int16_t temperature = 0;  
  temperature = round(DegreesC*4);

  if(isNegative){
    temperature = 0 - temperature;
    temperature |= (1 << 11);
  }
  
  setRegister(INT_LEVEL_REGISTER_HYST_LSB, temperature & 0xFF);
  setRegister(INT_LEVEL_REGISTER_HYST_MSB, temperature >> 8);
  
}

void GridEYE::setInterruptHysteresisRaw(int16_t regValue){
  
  setRegister(INT_LEVEL_REGISTER_HYST_LSB, regValue & 0xFF);
  setRegister(INT_LEVEL_REGISTER_HYST_MSB, regValue >> 8);
  
}

void GridEYE::setInterruptHysteresisFahrenheit(float DegreesF){

  bool isNegative = false;

  float DegreesC = (DegreesF - 32) / 1.8;

  if(DegreesC < 0){
    DegreesC = abs(DegreesC);
    isNegative = true;
  }
  
  int16_t temperature = 0;  
  temperature = round(DegreesC*4);

  if(isNegative){
    temperature = 0 - temperature;
    temperature |= (1 << 11);
  }
  
  setRegister(INT_LEVEL_REGISTER_HYST_LSB, temperature & 0xFF);
  setRegister(INT_LEVEL_REGISTER_HYST_MSB, temperature >> 8);
  
}

float GridEYE::getUpperInterruptValue()
{

  int16_t temperature = getRegister(INT_LEVEL_REGISTER_UPPER_LSB, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesC = temperature * 0.25;

  return DegreesC;

}

int16_t GridEYE::getUpperInterruptValueRaw()
{

  return getRegister(INT_LEVEL_REGISTER_UPPER_LSB, 2);

}

float GridEYE::getUpperInterruptValueFahrenheit()
{

  int16_t temperature = getRegister(INT_LEVEL_REGISTER_UPPER_LSB, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesF = (temperature * 0.25) * 1.8 + 32;

  return DegreesF;

}

float GridEYE::getLowerInterruptValue()
{

  int16_t temperature = getRegister(INT_LEVEL_REGISTER_LOWER_LSB, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesC = temperature * 0.25;

  return DegreesC;

}

float GridEYE::getLowerInterruptValueFahrenheit()
{

  int16_t temperature = getRegister(INT_LEVEL_REGISTER_LOWER_LSB, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesF = (temperature * 0.25) * 1.8 + 32;

  return DegreesF;

}

int16_t GridEYE::getLowerInterruptValueRaw()
{

  return getRegister(INT_LEVEL_REGISTER_LOWER_LSB, 2);

}

float GridEYE::getInterruptHysteresis()
{

  int16_t temperature = getRegister(INT_LEVEL_REGISTER_HYST_LSB, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesC = temperature * 0.25;

  return DegreesC;

}

float GridEYE::getInterruptHysteresisFahrenheit()
{

  int16_t temperature = getRegister(INT_LEVEL_REGISTER_HYST_LSB, 2);

  // temperature is reported as 12-bit twos complement
  // check if temperature is negative
  if(temperature & (1 << 11))
  {
    // if temperature is negative, mask out the sign byte and 
    // make the float negative
    temperature &= ~(1 << 11);
    temperature = temperature * -1;
  }

  float DegreesF = (temperature * 0.25) * 1.8 + 32;

  return DegreesF;

}

int16_t GridEYE::getInterruptHysteresisRaw()
{

  return getRegister(INT_LEVEL_REGISTER_HYST_LSB, 2);

}

/********************************************************
 * Functions for setting and getting registers over I2C
 ******************************************************** 
 * 
 * setRegister() - set unsigned char value at unsigned char register
 * 
 * getRegister() - get up to INT16 value from unsigned char register
 * 
 ********************************************************/

void GridEYE::setRegister(unsigned char reg, unsigned char val)
{

    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->write(val);
    _i2cPort->endTransmission();
    
}

// The parameter "len" may only be 1 (to retrieve a byte) or 2 (for a signed 16-bit integer).
int16_t GridEYE::getRegister(unsigned char reg, int8_t len)
{
  int16_t result = 0;     // default 0, if the sensor was not connected

  _i2cPort->beginTransmission(_deviceAddress);
  _i2cPort->write(reg);
  _i2cPort->endTransmission(false);    // 'false' for a repeated start

  _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)len);
  if (_i2cPort->available() == len)    // got valid data ?
  {
    if (len == 1)
    {
      result = (int16_t) _i2cPort->read();
    }
    else if (len == 2)
    {
      int lsb = _i2cPort->read();
      int msb = _i2cPort->read();

      // concat bytes
      result = (int16_t) (msb << 8 | lsb);
    }
  }
  return result;
}

// Start CK's code 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
#include <limits.h>
class Utils {
  public:
    static void publish(String s);
};

class GridEyeSupport {
private:
  GridEYE grideye;

public:
  int     mostRecentValue = INT_MIN;

  void begin() {
    grideye.begin();
  }

  // This will take 15-20 seconds if the GridEye isn't connected.
  int readValue() {
      float total = 0;
      for (int i = 0; i < 64; i++) {
        int t = (int)(grideye.getPixelTemperature(i) * 9.0 / 5.0 + 32.0);
        total += t;
      }
      mostRecentValue = (int)(total / 64);
    return mostRecentValue;
  }
};
GridEyeSupport gridEyeSupport;

void Utils::publish(String s) {
  char buf[12];
  sprintf(buf, "%10u", millis());
  String s1(buf);
  s1.concat(" ");
  s1.concat(s);
  Serial.println(s1);
}

class OLEDDisplayer {

  public:
    void display() {
      Utils::publish(String(gridEyeSupport.mostRecentValue));

    }
};
OLEDDisplayer oledDisplayer;

int lastDisplay = 0;
const int DISPLAY_RATE_IN_MS = 150;
#line 1080 "c:\\Users\\chris\\Documents\\Github\\arduino-heat-sensor\\arduino-heat-sensor.ino"
void display();
#line 1091 "c:\\Users\\chris\\Documents\\Github\\arduino-heat-sensor\\arduino-heat-sensor.ino"
void setup();
#line 1110 "c:\\Users\\chris\\Documents\\Github\\arduino-heat-sensor\\arduino-heat-sensor.ino"
void loop();
#line 1080 "c:\\Users\\chris\\Documents\\Github\\arduino-heat-sensor\\arduino-heat-sensor.ino"
void display() {
    int thisMS = millis();
    if (thisMS - lastDisplay > DISPLAY_RATE_IN_MS) {
      oledDisplayer.display();
      lastDisplay = thisMS;
    }
}

const String githubHash("github hash: to be filled in after 'git push'");
const String githubRepo("https://github.com/chrisxkeith/arduino-heat-sensor");

void setup() {
  Serial.begin(57600);
  Utils::publish("Started setup...");
  Utils::publish(githubRepo);
  Utils::publish(githubHash);

  // Start your preferred I2C object
  Wire.begin();
  // Library assumes "Wire" for I2C but you can pass something else with begin() if you like

    gridEyeSupport.begin();
    int now = millis();
    gridEyeSupport.readValue();
    delay(5000);
    display();

  Utils::publish("Finished setup...");	
}

void loop() {
    gridEyeSupport.readValue();
    display();
    delay(2000);
}

