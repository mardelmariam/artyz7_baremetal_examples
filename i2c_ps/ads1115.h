#ifndef ADS1115_H_
#define ADS1115_H_

#include "platform.h"
#include "xil_printf.h"

#include "xiicps.h"
#include "xil_exception.h"

#include "sleep.h"
#include "xstatus.h"

#include <xparameters.h>
#include <xparameters_ps.h>

#define ADS1115_ADDRESS 0x49

// Registers and other configurable values
#define ADS1115_POINTER_CONVERSION 0x0
#define ADS1115_POINTER_CONFIG 0x1
#define ADS1115_CONFIG_OS_SINGLE 0x8000
#define ADS1115_CONFIG_MUX_OFFSET 12
#define ADS1115_CONFIG_MODE_CONTINUOUS 0x0
#define ADS1115_CONFIG_MODE_SINGLE 0x100
#define ADS1115_CONFIG_COMP_QUE_DISABLE 0x3

// Functions
void I2CByteWrite(XIicPs *InstancePtr, u8 device_addr, u8 data);
void I2CByteWrites(XIicPs *InstancePtr, u8 device_addr, u8 reg_addr, u8 data_msb, u8 data_lsb);
void I2CReadBytes(XIicPs *InstancePtr, u8 device_addr, u8 *rx_buff_ptr);
u8 I2CReadByte(XIicPs *InstancePtr, u8 device_addr, u8 reg_addr);

void ADS1115_Config(XIicPs *XIicInstPtr, u8 ch, u8 gain, u8 data_rate);
void ADS1115_Read(XIicPs *XIicInstPtr);

#endif