/*

* Author: Texas Instruments

* Editted by: Saurav Shandilya, Vishwanathan Iyer
	      ERTS Lab, CSE Department, IIT Bombay

* Description: This code will familiarize you with using GPIO on TMS4C123GXL microcontroller.

* Filename: lab-1.c

* Functions: setup(), ledPinConfig(), switchPinConfig(), main()

* Global Variables: none

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
unsigned char flagSW1 = 0;
unsigned char flagSW2 = 0;
uint8_t ui8LED = 2;
uint32_t sw2status = 0;
/*
 ------ Global Variable Declaration
*/


/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set crystal frequency and enable GPIO Peripherals

* Example Call: setup();

*/
void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}

/*

* Function Name: ledPinConfig()

* Input: none

* Output: none

* Description: Set PORTF Pin 1, Pin 2, Pin 3 as output.

* Example Call: ledPinConfig();

*/

void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);  // Pin-1 of PORT F set as output. Modifiy this to use other 2 LEDs.
}

/*

* Function Name: switchPinConfig()

* Input: none

* Output: none

* Description: Set PORTF Pin 0 and Pin 4 as input. Note that Pin 0 is locked.

* Example Call: switchPinConfig();

*/
void switchPinConfig(void)
{
	// Following two line removes the lock from SW2 interfaced on PORTF Pin0 -- leave this unchanged
	LOCK_F=0x4C4F434BU;
	CR_F=GPIO_PIN_0|GPIO_PIN_4;

	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_0|GPIO_PIN_4,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_0|GPIO_PIN_4,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	// Read the current state of the GPIO pin and
	// write back the opposite state
//	if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2))
//	{
//		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
//	}
//	else
//	{
//		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 4);
//	}
	if(detectSW1Press()==2){
		changeColor();
	}
	if(detectSW2Press()==2){
		incrementsw2status();
	}
}

void changeColor(){
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8LED);
	if (ui8LED == 2)
	{
		ui8LED = 8;
	}
	else if(ui8LED==8)
	{
		ui8LED = 4;
	}else{
		ui8LED = 2;
	}
}

unsigned char detectSW1Press(){
	uint8_t ui8IP4 = GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4);
	if (ui8IP4==0x00&&(flagSW1==0||flagSW1==1)){
		flagSW1++;
	}else if(ui8IP4==0x00&&flagSW1==2){
		return 0;
	}else{
		flagSW1 = 0;
	}
	return flagSW1;
}

unsigned char detectSW2Press(){
	uint8_t ui8IP0 = GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0);
	if (ui8IP0==0x00&&(flagSW2==0||flagSW2==1)){
		flagSW2++;
	}else if(ui8IP0==0x00&&flagSW2==2){
		return 0;
	}else{
		flagSW2 = 0;
	}
	return flagSW2;
}

void incrementsw2status(){
	sw2status++;
}

int main(void)
{
	uint32_t ui32Period;

	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

	setup();
	ledPinConfig();
	switchPinConfig();

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	ui32Period = (SysCtlClockGet() / 100);
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	TimerEnable(TIMER0_BASE, TIMER_A);

	while(1)
	{
	}
}
