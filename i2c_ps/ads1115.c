#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

#include "xiicps.h"
#include "xil_exception.h"

#include "sleep.h"
#include "xstatus.h"

#include <xparameters.h>
#include <xparameters_ps.h>

#include "ads1115.h"

// Mapping of gain values to config register values
// Full-scale inputs accepted: 6.144V, 4.096V, 2.048V, 1.024V, 0.512V and 0.256V
u16 ADS1115_ConfigGain[6] = {0x0, 0x200, 0x400, 0x600, 0x800, 0xA00};

// Mapping of data/sample rate (in SPS) to config register values for ADS1115
// 8, 16, 32, 64, 128, 250, 475, 860
u8 ADS1115_ConfigDr[8] = {0x0, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0};


/*****************************************************************************/
/**
* This function sends a single byte after the device adress.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	InstancePtr is the slave device address.
* @param	data is the byte to send.
*
* @return	None.
*
*
 ****************************************************************************/
void I2CByteWrite(XIicPs *InstancePtr, u8 device_addr, u8 data){
    u8 tx_buff[2] = {data, 0};
    int Status = XIicPs_MasterSendPolled(InstancePtr, tx_buff, 1, device_addr);
	if (Status != XST_SUCCESS) {
		xil_printf("I2C send failed\n\r");
	}
    while(XIicPs_BusIsBusy(InstancePtr)){
		/* NOP */
	}
 }

/*****************************************************************************/
/**
* This function sends three bytes: a register byte, and two data bytes
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	InstancePtr is the slave device address.
* @param	reg_addr is the register to write.
* @param	data_msb is the most significant byte.
* @param	data_lsb is the least significant byte.
*
* @return	None.
*
 ****************************************************************************/
void I2CByteWrites(XIicPs *InstancePtr, u8 device_addr, u8 reg_addr, u8 data_msb, u8 data_lsb){
    u8 tx_buff[3] = {reg_addr, data_lsb, data_msb};
    int Status = XIicPs_MasterSendPolled(InstancePtr, tx_buff, 3, device_addr);
	if (Status != XST_SUCCESS) {
		xil_printf("I2C send failed\n\r");
	}
    while(XIicPs_BusIsBusy(InstancePtr)){
		/* NOP */
	}
 }

/*****************************************************************************/
/**
* This function receives two bytes.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	InstancePtr is the slave device address.
* @param	rx_buff_ptr is a pointer to the data array.
*
* @return	None.
*
 ****************************************************************************/
void I2CReadBytes(XIicPs *InstancePtr, u8 device_addr, u8 *rx_buff_ptr){
    u8 rx_buff[2] = {0x00, 0x00};
    int Status = XIicPs_MasterRecvPolled(InstancePtr, rx_buff, 2, device_addr);
	if (Status != XST_SUCCESS) {
		xil_printf("I2C recv failed\n\r");
	}
    while (XIicPs_BusIsBusy(InstancePtr)) {
		/* NOP */
	}
    rx_buff_ptr[0] = rx_buff[0];
    rx_buff_ptr[1] = rx_buff[1];  
}

/*****************************************************************************/
/**
* This function configures the ADS1115 by using four inputs, having one of them
* as their reference.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
* @param	channel is the result of the input multiplexer configuration.
* @param	gain is the programmable gain setting.
* @param	data_rate is the data conversion rate.
*
* @return	None.
*
 ****************************************************************************/
void ADS1115_Config(XIicPs *XIicInstPtr, u8 channel, u8 gain, u8 data_rate){

    u16 config = ADS1115_CONFIG_OS_SINGLE;
    config |= (channel & 0x07) << ADS1115_CONFIG_MUX_OFFSET;
    config |= ADS1115_ConfigGain[gain];
    config |= ADS1115_CONFIG_MODE_CONTINUOUS;
    config |= ADS1115_ConfigDr[data_rate];
    config |= ADS1115_CONFIG_COMP_QUE_DISABLE;
    u8 data[2] = {(config >> 8) & 0xFF, config & 0xFF}; 
    I2CByteWrites(XIicInstPtr,  ADS1115_ADDRESS, ADS1115_POINTER_CONFIG, data[1], data[0]);
    // Suggestion from the quickstart guide
    //I2CByteWrites(XIicInstPtr, ADS1115_ADDRESS, ADS1115_POINTER_CONFIG, 0x84, 0x83);
    printf("Ch %u: - Config: %x - ", channel, config);
    usleep((int)((1.0/ADS1115_ConfigDr[data_rate]) + 100));
}

/*****************************************************************************/
/**
* This function reads and processes the result from a given configuration,
* and prints the raw result as well as the voltage conversion.
*
* @param	InstancePtr is a pointer to the XIicPs instance.
*
* @return	None.
*
 ****************************************************************************/
void ADS1115_Read(XIicPs *XIicInstPtr){
    u8 data[2] = {0x0, 0x0};
    // First point to the conversion register
    I2CByteWrite(XIicInstPtr, ADS1115_ADDRESS, ADS1115_POINTER_CONVERSION);
    // Then, read the conversion result
    I2CReadBytes(XIicInstPtr, ADS1115_ADDRESS, data);
    // Process the result
    u16 raw_result = (data[0]*256 + data[1]);
    float voltage = raw_result * 4.096 / 32768;
    printf("Read: %u - %.4f V\n", raw_result, voltage);
}

