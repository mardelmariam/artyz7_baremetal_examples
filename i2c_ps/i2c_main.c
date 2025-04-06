/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/*
 * Github - @mardelmariam
 * An excerpt of an I2C application for demonstration purposes
 * License: MIT
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

#include "xiicps.h"
#include "xiicps_xfer.h"
#include "xil_exception.h"

#include "sleep.h"
#include "xstatus.h"

#include <xparameters.h>
#include <xparameters_ps.h>

#include "ads1115.h"

#define IIC_BASEADDR XPAR_XIICPS_0_BASEADDR
#define IIC_FREQUENCY 100000

XIicPs IicInstance; // I2C driver instance


int IIC_Initialization(XIicPs *XIicInstPtr, u32 DeviceAddr){

    int Status;
    XIicPs_Config *ConfigPtr;

    xil_printf("Initializing I2C...\n");

    ConfigPtr = XIicPs_LookupConfig(DeviceAddr);
    if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

    XIicPs_CfgInitialize(XIicInstPtr, ConfigPtr, ConfigPtr->BaseAddress);

    Status = XIicPs_SelfTest(XIicInstPtr);
    if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

    XIicPs_SetSClk(XIicInstPtr, IIC_FREQUENCY);

    xil_printf("I2C initialized\n");

    return XST_SUCCESS;

}

int main()
{
    init_platform();
    xil_printf("Starting I2C...\n");

    int Status = IIC_Initialization(&IicInstance, IIC_BASEADDR);

    if (Status == XST_SUCCESS) {
        while(1){
            ADS1115_Config(&IicInstance, 1, 1, 4);
            ADS1115_Read(&IicInstance);

            ADS1115_Config(&IicInstance, 2, 1, 4);
            ADS1115_Read(&IicInstance);
            
            ADS1115_Config(&IicInstance, 3, 1, 4);
            ADS1115_Read(&IicInstance);
            
            sleep(1);
        }
    }

    //cleanup_platform();
    
    return 0;
}