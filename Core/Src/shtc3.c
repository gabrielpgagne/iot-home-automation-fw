//==============================================================================
//    S E N S I R I O N   AG,  Laubisruetistr. 50, CH-8712 Staefa, Switzerland
//==============================================================================
// Project   :  SHTC3 Sample Code (V1.0)
// File      :  shtc3.c (V1.0)
// Author    :  RFU
// Date      :  24-Nov-2017
// Controller:  STM32F100RB
// IDE       :  �Vision V5.17.0.0
// Compiler  :  Armcc
// Brief     :  Sensor Layer: Implementation of functions for sensor access.
//==============================================================================

#include <stdint.h>
#include "stm32wbxx_hal.h"

#include "shtc3.h"

typedef enum{
  READ_ID            = 0xEFC8, // command: read ID register
  SOFT_RESET         = 0x805D, // soft reset
  SLEEP              = 0xB098, // sleep
  WAKEUP             = 0x3517, // wakeup
  MEAS_T_RH_POLLING  = 0x7866, // meas. read T first, clock stretching disabled
  MEAS_T_RH_CLOCKSTR = 0x7CA2, // meas. read T first, clock stretching enabled
  MEAS_RH_T_POLLING  = 0x58E0, // meas. read RH first, clock stretching disabled
  MEAS_RH_T_CLOCKSTR = 0x5C24  // meas. read RH first, clock stretching enabled
}etCommands;


static etError SHTC3_Read2BytesAndCrc(uint16_t *data);
static etError SHTC3_ReadSampleAndCrc(uint16_t *raw_humid, uint16_t *raw_temp);
static etError SHTC3_WriteCommand(etCommands cmd);
static etError SHTC3_CheckCrc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum);
static float SHTC3_CalcTemperature(uint16_t rawValue);
static float SHTC3_CalcHumidity(uint16_t rawValue);

static uint8_t _Address;
static I2C_HandleTypeDef *_i2c;

//------------------------------------------------------------------------------
void SHTC3_Init(uint8_t address, I2C_HandleTypeDef  *i2c){
  _Address = address;
  _i2c = i2c;
}

//------------------------------------------------------------------------------
etError SHTC3_GetTempAndHumi(float *temp, float *humi){
  etError  error;        // error code
  uint16_t rawValueTemp; // temperature raw value from sensor
  uint16_t rawValueHumi; // humidity raw value from sensor

  // measure, read temperature first, clock streching enabled

  error = SHTC3_WriteCommand(MEAS_T_RH_CLOCKSTR);

  // if no error, read temperature and humidity raw values
  if(error == NO_ERROR) {
    error |= SHTC3_Read2BytesAndCrc(&rawValueTemp);
    error |= SHTC3_Read2BytesAndCrc(&rawValueHumi);
  }

  // if no error, calculate temperature in �C and humidity in %RH
  if(error == NO_ERROR) {
    *temp = SHTC3_CalcTemperature(rawValueTemp);
    *humi = SHTC3_CalcHumidity(rawValueHumi);
  }

  return error;
}

//------------------------------------------------------------------------------
etError SHTC3_GetTempAndHumiPolling(float *temp, float *humi){
  etError  error;           // error code
  uint16_t rawValueTemp;    // temperature raw value from sensor
  uint16_t rawValueHumi;    // humidity raw value from sensor

  // measure, read temperature first, clock streching disabled (polling)
  error = SHTC3_WriteCommand(MEAS_T_RH_POLLING);

  if(error == NO_ERROR) {
	// if no error, read temperature and humidity raw values
	HAL_Delay(30);
	error |= SHTC3_ReadSampleAndCrc(&rawValueTemp, &rawValueHumi);
  }

  // if no error, calculate temperature in C and humidity in %RH
  if(error == NO_ERROR) {
    *temp = SHTC3_CalcTemperature(rawValueTemp);
    *humi = SHTC3_CalcHumidity(rawValueHumi);
  }

  return error;
}

//------------------------------------------------------------------------------
etError SHTC3_GetId(uint16_t *id){

  // write ID read command
  etError error = SHTC3_WriteCommand(READ_ID);

  // if no error, read ID
  if(error == NO_ERROR) {
    error = SHTC3_Read2BytesAndCrc(id);
  }

  return error;
}

//------------------------------------------------------------------------------
etError SHTC3_Sleep(void) {
  return SHTC3_WriteCommand(SLEEP);
}

//------------------------------------------------------------------------------
etError SHTC3_Wakeup(void) {
  etError error = SHTC3_WriteCommand(WAKEUP);

  HAL_Delay(1);

  return error;
}

//------------------------------------------------------------------------------
etError SHTC3_SoftReset(void){
  // write reset command
  HAL_StatusTypeDef err = SHTC3_WriteCommand(SOFT_RESET);

  if (err != HAL_OK)
	  return ACK_ERROR;
  else
	  return NO_ERROR;
}

//------------------------------------------------------------------------------
static etError SHTC3_WriteCommand(etCommands cmd){
  // write MSB first
  uint8_t buf[2] = {cmd>>8, cmd&0xFF};
  HAL_StatusTypeDef err = HAL_I2C_Master_Transmit(_i2c, _Address, buf, 2, 100);

  if (err != HAL_OK)
	  return ACK_ERROR;
  else
	  return NO_ERROR;
}

//------------------------------------------------------------------------------
static etError SHTC3_Read2BytesAndCrc(uint16_t *data){
  etError error;    // error code
  uint8_t bytes[3]; // read data array

  // read two data bytes and one checksum byte
  HAL_StatusTypeDef err = HAL_I2C_Master_Receive(_i2c, _Address, bytes, sizeof(bytes), 100);

  if (err != HAL_OK)
	  return ACK_ERROR;
  else
	  return NO_ERROR;

  // verify checksum
  error |= SHTC3_CheckCrc(bytes, 2, bytes[sizeof(bytes)-1]);

  if (error != NO_ERROR) {
	  return error;
  }

  // combine the two bytes to a 16-bit value
  *data = (bytes[0] << 8) | bytes[1];
}

//------------------------------------------------------------------------------
static etError SHTC3_ReadSampleAndCrc(uint16_t *raw_1, uint16_t *raw_2){
  etError error;    // error code
  uint8_t bytes[6]; // read data array

  // read two data bytes and one checksum byte
  HAL_StatusTypeDef err = HAL_I2C_Master_Receive(_i2c, _Address, bytes, sizeof(bytes), 200);

  if (err != HAL_OK)
	  return ACK_ERROR;

  // verify checksum
  error = SHTC3_CheckCrc(bytes, 2, bytes[2]);
  error |= SHTC3_CheckCrc(&bytes[3], 2, bytes[5]);

  if (error != NO_ERROR) {
	  return error;
  }

  // combine the two bytes to a 16-bit value
  *raw_1 = (bytes[0] << 8) | bytes[1];
  *raw_2 = (bytes[3] << 8) | bytes[4];

  return error;
}
//------------------------------------------------------------------------------
static etError SHTC3_CheckCrc(uint8_t data[], uint8_t nbrOfBytes,
                              uint8_t checksum){
  uint8_t bit;        // bit mask
  uint8_t crc = 0xFF; // calculated checksum
  uint8_t byteCtr;    // byte counter

  // calculates 8-Bit checksum with given polynomial
  for(byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
    crc ^= (data[byteCtr]);
    for(bit = 8; bit > 0; --bit) {
      if(crc & 0x80) {
        crc = (crc << 1) ^ CRC_POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }

  // verify checksum
  if(crc != checksum) {
    return CHECKSUM_ERROR;
  } else {
    return NO_ERROR;
  }
}

//------------------------------------------------------------------------------
static float SHTC3_CalcTemperature(uint16_t rawValue){
  // calculate temperature [�C]
  // T = -45 + 175 * rawValue / 2^16
  return 175 * (float)rawValue / 65536.0f - 45.0f;
}

//------------------------------------------------------------------------------
static float SHTC3_CalcHumidity(uint16_t rawValue){
  // calculate relative humidity [%RH]
  // RH = rawValue / 2^16 * 100
  return 100 * (float)rawValue / 65536.0f;
}
