#include <Wire.h>
#define HDC_I2C_ADDRESS 0x40

bool is_device_available(uint8_t address)
{
  Wire.beginTransmission(address);
  uint8_t error = Wire.endTransmission();

  return (error == 0);
}

void wait_for_device(uint8_t i2c_device_address) {
  while(!is_device_available(i2c_device_address));
}

void hdc_write_byte(uint8_t i2c_device_address, uint8_t address, uint8_t data) {
  Wire.beginTransmission(i2c_device_address);
  Wire.write(address >> 8); 
  Wire.write(data);
  Wire.endTransmission();
  wait_for_device(i2c_device_address);
}

byte hdc_read_byte(uint8_t i2c_device_address, uint8_t address) {
  uint8_t rdata = 0xFF;

  Wire.beginTransmission(i2c_device_address);
  Wire.write(address); 
  Wire.endTransmission();
  Wire.requestFrom(i2c_device_address,(uint8_t)1);
  if (Wire.available()) {
    rdata = Wire.read();
  }
  return rdata;
}

void setup()
{
  char buffer[20];
  Wire.begin();

  Serial.begin(9600);
  while (!Serial);

  if (!is_device_available(HDC_I2C_ADDRESS))
  {
    Serial.println("No device was found");
    while(true);
  }

  byte data = hdc_read_byte(HDC_I2C_ADDRESS, 0xfe);
  if(data == 0xd0) {
    Serial.println("Works");
  } else {
    sprintf(buffer, "Doesn't work: %x", data);
    Serial.println(buffer);
  }
}

void loop()
{

}