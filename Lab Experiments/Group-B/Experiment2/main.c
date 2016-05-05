/*
 * main.c
 *
 *  Created on: 02-Feb-2016
 *      Author: ARYA
 */


#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

//global variables
// 0 - idle, 1 - pressed, 2 - released
int s1_state = 0;
int s2_state = 0;
uint8_t led = 2;
int sw2Status = 0;


void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}

void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);  // Pin-1 of PORT F set as output. Modifiy this to use other 2 LEDs.
}

void switchPinConfig(void)
{
	// Following two line removes the lock from SW2 interfaced on PORTF Pin0 -- leave this unchanged
	LOCK_F=0x4C4F434BU;
	CR_F=GPIO_PIN_0|GPIO_PIN_4;

	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);

	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_0,GPIO_DIR_MODE_IN); // Set Pin-0 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_0,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

unsigned char detectKey1Press()
{
	unsigned char flag = '0';
	int32_t not_pressed = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
	if(s1_state == 0)
	{
		if(!not_pressed)
			s1_state = 1;
	}
	else if(s1_state == 1)
	{
		if(!not_pressed)
		{
			s1_state = 2;
			flag = '1';
		}
		else
		{
			s1_state = 0;
		}
	}
	else if(s1_state == 2)
	{
		if(not_pressed)
		{
			s1_state = 0;
		}
	}
	return flag;
}

unsigned char detectKey2Press()
{
	unsigned char flag = '0';
	int32_t not_pressed = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);
	if(s2_state == 0)
	{
		if(!not_pressed)
			s2_state = 1;
	}
	else if(s2_state == 1)
	{
		if(!not_pressed)
		{
			s2_state = 2;
			flag = '1';
		}
		else
		{
			s2_state = 0;
		}
	}
	else if(s2_state == 2)
	{
		if(not_pressed)
		{
			s2_state = 0;
		}
	}
	return flag;
}


void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	//detect key1 press
	if(detectKey1Press()=='1')
	{
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, led);
		led = led*2;
		if(led == 16) led = 2;
	}
	//detect key2 press
	if(detectKey2Press()=='1')
	{
		sw2Status = sw2Status + 1;
	}
}


int main(void)
{
	uint32_t ui32Period;
	setup();
	ledPinConfig();
	switchPinConfig();

	//timer enabling
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	ui32Period = SysCtlClockGet() / 100;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();
	TimerEnable(TIMER0_BASE, TIMER_A);
	while(1)
	{
	}
}


