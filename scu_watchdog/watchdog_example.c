/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*

Github - @mardelmariam

General description: 

This code sets up a basic example for managing a watchdog timer 
along with GPIO and interrupt handling. It initializes the GPIO to provide 
visual feedback by blinking LEDs, configures and starts a watchdog timer, 
and sets up the interrupt system to catch a watchdog timeout event.

The main loop simulates work by running delays, periodically "petting" the watchdog 
to prevent a timeout, and toggling the GPIO state until the watchdog expires 
and triggers its interrupt handler. 

Upon the watchdog event, the interrupt handler stops the main loop and 
outputs a termination message.

*/
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

#include "xscugic.h"
#include "xil_exception.h"
#include "xscuwdt.h"
#include "xgpio.h"

#include "sleep.h"
#include "xstatus.h"

#include <xparameters.h>
#include <xparameters_ps.h>

// Macro definitions for base addresses, device IDs, and configuration value
#define WATCHDOG_BASEADDR XPAR_SCUWDT_BASEADDR
#define WATCHDOG_DEVICE_ID XPAR_SCUWDT_DEVICE_ID
#define GPIO_BASEADDR XPAR_XGPIO_0_BASEADDR
#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#define INTR_ID XPAR_SCUWDT_INTR

// Load value determines the watchdog countdown interval (~1 second here)
#define WATCHDOG_LOAD_VALUE  0x2FAF080

// Global driver instances
XScuGic Intc;
XScuWdt Watchdog;
XGpio Gpio;

int handler_called; // Flag to indicate the watchdog interrupt was handled

// Function prototypes for initialization and interrupt handling
int Watchdog_Initialization(XScuWdt *WatchdogInstPtr, u32 BaseAddr);
void WatchdogIntrHandler(void *CallBackRef);
int SetupInterruptSystem(XScuGic *GicInstPtr, XScuWdt *TimerInstPtr, u16 IntrId);
int Gpio_Initialization(XGpio *GpioInstPtr, u32 BaseAddr);


int main()
{
    init_platform();

    int Status;

    print("Timers example\n\r");

    Status = Gpio_Initialization(&Gpio, XPAR_XGPIO_0_BASEADDR);
    if (Status != XST_SUCCESS){
        xil_printf("Gpio fail");
        return XST_FAILURE;
    }

    Status = Watchdog_Initialization(&Watchdog, WATCHDOG_BASEADDR);
    if (Status != XST_SUCCESS){
        xil_printf("Watchdog initialization fail\n");  
        return XST_FAILURE;  
    }

    Status = SetupInterruptSystem(&Intc, &Watchdog, INTR_ID);
    if (Status != XST_SUCCESS){
        xil_printf("Interrupts fail");
        return XST_FAILURE;
    }

    XScuWdt_Start(&Watchdog);

    u32 time_counts = 0;
    u32 time_limit = 10000;
    int run_cycles = 0;
    u8 state = 0b000;
    handler_called = 0;

    // Main loop: continue until the watchdog interrupt handler sets the flag.
    while (handler_called == 0){
        // Busy-wait loop to simulate processing delay.
        for (u32 i=0; i<time_limit; i++){
            time_counts++;          
        }
        time_counts = 0;
        time_limit += 10000; // Gradually increase the delay period
        XScuWdt_RestartWdt(&Watchdog); // "Pet the dog" (reset  Watchdog timer)

        // Print out the current cycle and delay count.
        printf("Run: %d - Count: %u\n", run_cycles, time_limit);
        run_cycles++;

        // Toggle GPIO state to provide visual feedback (e.g., blinking an LED).
        state ^= 0b1111;
        XGpio_WriteReg(XPAR_XGPIO_0_BASEADDR, XGPIO_DATA_OFFSET, state);

        // If the dog isn't petted within a certain time interval... (for polling)
        //if(XScuWdt_IsTimerExpired(&Watchdog)){
        //}
    }

    // Indicate program termination.
    xil_printf("Program stopped due to Watchdog!\n");

    // Pulling a "warm reset", in which only the processor is reset is a difficult trick,
    // since the ARM processor in the Zynq SoC wasn't designed for that:
    // https://docs.amd.com/r/en-US/ug585-zynq-7000-SoC-TRM/Reset-Functionality
    // It can be done, but you will need grit and elbow grease:
    // https://adaptivesupport.amd.com/s/question/0D52E00006iHos4SAC/zynq7000-psonly-reset?language=en_US

    return 0;
}

int SetupInterruptSystem(XScuGic *GicInstPtr, XScuWdt *WatchdogInstPtr, u16 IntrId){

    XScuGic_Config *IntcConfig;

    // Initializing the interrupt controller
    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (IntcConfig == NULL) {
        xil_printf("Fail1\n");
		return XST_FAILURE;
	}

    int Status = XScuGic_CfgInitialize(GicInstPtr, IntcConfig, IntcConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS){
        xil_printf("Fail2\n");
        return XST_FAILURE;
    };

    // Set interrupt priority and trigger type.
    XScuGic_SetPriorityTriggerType(GicInstPtr, IntrId, 0xA0, 0x3); // F8 no funciona

    // Connect the interrupt controller and interrupt handler to the hardware interrupt
    Status = XScuGic_Connect(GicInstPtr, IntrId, (Xil_InterruptHandler) WatchdogIntrHandler, (void *)WatchdogInstPtr);

    if (Status != XST_SUCCESS){
        xil_printf("Fail3\n");
        return XST_FAILURE;
    }

    // Enable interrupts for the watchdog
    XScuGic_Enable(GicInstPtr, IntrId);

    // Enable interrupts in the processor
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, GicInstPtr);
    Xil_ExceptionEnable();

    xil_printf("Interrupt: Success\n");

    return XST_SUCCESS;

}

int Watchdog_Initialization(XScuWdt *WatchdogInstPtr, u32 BaseAddr){

    /*
    * Here, the Timer mode is preferred over the Watchdog mode.
    * Using the Watchdog mode desn't give any visible chances to indicate
    * that the program was stopped.
    */

    XScuWdt_Config *WatchdogConfig;

    // Initializing the watchdog instance
    WatchdogConfig = XScuWdt_LookupConfig(BaseAddr);
	if (WatchdogConfig == NULL) {
		return XST_FAILURE;
	}

    int Status = XScuWdt_CfgInitialize(WatchdogInstPtr, WatchdogConfig, WatchdogConfig->BaseAddr);
    if (Status != XST_SUCCESS){
        return XST_FAILURE;
    };

    Status = XScuWdt_SelfTest(WatchdogInstPtr);
    if (Status != XST_SUCCESS){
        return XST_FAILURE;
    };

    // Watchdog configurations    
    XScuWdt_LoadWdt(WatchdogInstPtr, WATCHDOG_LOAD_VALUE);
    XScuWdt_SetTimerMode(WatchdogInstPtr);

    // Enabling interrupts
    u32 reg = XScuWdt_GetControlReg(WatchdogInstPtr);
    XScuWdt_SetControlReg(WatchdogInstPtr,reg | XSCUWDT_CONTROL_IT_ENABLE_MASK);

    xil_printf("Watchdog: Success\n");
    
    return XST_SUCCESS;
}

int Gpio_Initialization(XGpio *GpioInstPtr, u32 BaseAddr){

    int Status = XGpio_Initialize(GpioInstPtr, BaseAddr);
    if (Status != XST_SUCCESS){
        return XST_FAILURE;
    };

    Status = XGpio_SelfTest(GpioInstPtr);
    if (Status != XST_SUCCESS){
        return XST_FAILURE;
    };

    XGpio_SetDataDirection(GpioInstPtr, 1, 0b0000); 

    XGpio_DiscreteWrite(GpioInstPtr, 1, 0b1111);
    sleep(1);
    XGpio_DiscreteWrite(GpioInstPtr, 1, 0b0000);
    sleep(1);

    XGpio_DiscreteWrite(GpioInstPtr, 1, 0b1111);
    sleep(1);
    XGpio_DiscreteWrite(GpioInstPtr, 1, 0b0000);
    sleep(1);

    XGpio_DiscreteWrite(GpioInstPtr, 1, 0b1111);
    
    return XST_SUCCESS;

}

void WatchdogIntrHandler(void *CallBackRef){
    xil_printf("Watchdog Event!\n\r");
    XScuWdt *WatchdogInstPtr = (XScuWdt *)CallBackRef;

    // Clear Watchdog's interrupt
	XScuWdt_WriteReg(WatchdogInstPtr->Config.BaseAddr, XSCUWDT_ISR_OFFSET, XSCUWDT_ISR_EVENT_FLAG_MASK);

    handler_called = 1;
}
