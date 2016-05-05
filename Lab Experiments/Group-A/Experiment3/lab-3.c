#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

// pwm frequency
#define PWM_FREQUENCY 55
// volatile means they won't be eliminated in optimization of program
volatile uint32_t ui32Load;
volatile uint32_t ui32PWMClock;
volatile uint8_t ui8Adjust5 = 83,ui8Adjust6 = 83,ui8Adjust7 = 83; // Used to set the brightness
uint8_t currentMode = 0;
uint8_t sw1PressCount = 0;
uint8_t sw2PressCount = 0;
uint32_t sw1Count = 0;
uint32_t sw2Count = 0;
uint32_t sw1flag = 0;
uint32_t sw2flag = 0;
uint32_t changeFrequency = 50;
uint32_t currentColorCount = 0;
uint32_t currentColor = 2;

/*
 * Function Name: setup()
 * Input: none
 * Output: none
 * Description: Set crystal frequency and enable GPIO Peripherals
 * Example Call: setup();
 */
void setup(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);// CPU is running at 40MHz
	SysCtlPWMClockSet(SYSCTL_PWMDIV_64);// Setting clock frequency of PWM at 625Khz by setting divider at 64
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);// Enable PWM
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);// Enable GPIO Port F
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
	// Unlock the GPIO Commit Control register fo using s2
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

	// Setting sw1 and sw2 as input pins
	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_DIR_MODE_IN);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}
/*
 * Function Name: applyColor()
 * Input:
 * Output:
 * Description: Applies the current color to LEDs
 * Example Call: applyColor();
 */
void applyColor(){
	uint32_t color = currentColor;
	if(color>=8){
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust7 * ui32Load / 1000);
		color -= 8;
	}else{
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, 1);
	}
	if(color>=4){
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust6 * ui32Load / 1000);
		color -= 4;
	}else{
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, 1);
	}
	if(color>=2){
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust5 * ui32Load / 1000);
		color -= 2;
	}else{
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 1);
	}
}
/*
 * Function Name: setPWMOutput()
 * Input: none
 * Output: none
 * Description: Set PORTF Pin 1|2|3 as output for PWM
 * Example Call: setPWMOutput();
 */
void setPWMOutput(void){
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);
	GPIOPinConfigure(GPIO_PF2_M1PWM6);
	GPIOPinConfigure(GPIO_PF3_M1PWM7);

	// Sets the PWM Clock by dividing the System clock by 64 as defined in setup()
	ui32PWMClock = SysCtlClockGet() / 64;
	ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;

	// Configures the generators being used i.e. 2,3
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, ui32Load);
	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Load);

	// Configures the output PWMs being used i.e 5,6,7
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust5 * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust6 * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust7 * ui32Load / 1000);
	PWMOutputState(PWM1_BASE, PWM_OUT_7_BIT, true);

	// Enable PWM Generators
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);
}
/*
 * Function Name: doSW1Action()
 * Input: none
 * Output: none
 * Description: Does the action for SW1 button based on mode the system is in
 * Example Call: doSW1Action();
 */
void doSW1Action(){
	if(currentMode == 0){
		changeFrequency-=5;
		if(changeFrequency<=5){
			changeFrequency = 5;
		}
		applyColor();
	}
	else if(currentMode == 1){
		if (ui8Adjust5 <= 10)
		{
			ui8Adjust5 = 10;
		}else{
			ui8Adjust5-=10;
		}
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust5 * ui32Load / 1000);
	}
	else if(currentMode == 2){
		if (ui8Adjust6 <= 10){
			ui8Adjust6 = 10;
		}else{
			ui8Adjust6-=10;
		}
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust6 * ui32Load / 1000);
	}
	else if(currentMode == 3){
		if (ui8Adjust7 <= 10){
			ui8Adjust7 = 10;
		}else{
			ui8Adjust7-=10;
		}
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust7 * ui32Load / 1000);
	}
}
/*
 * Function Name: doSW2Action()
 * Input: none
 * Output: none
 * Description: Does the action for SW2 button based on mode the system is in
 * Example Call: doSW2Action();
 */
void doSW2Action(){
	if(currentMode == 0){
		changeFrequency+=5;
		if(changeFrequency>100){
			changeFrequency = 100;
		}
		applyColor();
	}
	else if(currentMode == 1){
		if (ui8Adjust5 > 240)
		{
			ui8Adjust5 = 254;
		}else{
			ui8Adjust5+=10;
		}
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust5 * ui32Load / 1000);
	}
	else if(currentMode == 2){
		if (ui8Adjust6 > 240){
			ui8Adjust6 = 254;
		}else{
			ui8Adjust6+=10;
		}
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust6 * ui32Load / 1000);
	}
	else if(currentMode == 3){
		if (ui8Adjust7 > 240){
			ui8Adjust7 = 254;
		}else{
			ui8Adjust7+=10;
		}
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust7 * ui32Load / 1000);
	}
}
/*
 * Function Name: changeColor()
 * Input: none
 * Output: none
 * Description: Changes the color of LEDs as per frequency
 * Example Call: changeColor();
 */
void changeColor(){
	currentColorCount = currentColorCount%changeFrequency;
	if(currentColorCount==0&&currentMode==0){
		if(currentColor==2){
			currentColor = 10;
		}else if(currentColor==10){
			currentColor = 8;
		}else if(currentColor==8){
			currentColor = 12;
		}else if(currentColor==12){
			currentColor = 4;
		}else if(currentColor==4){
			currentColor = 6;
		}else if(currentColor==6){
			currentColor = 2;
		}
		applyColor();
	}else if(currentMode!=0){
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui8Adjust5 * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui8Adjust6 * ui32Load / 1000);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui8Adjust7 * ui32Load / 1000);
	}
}


/*
 * Function Name: detectSW1Press()
 * Input: none
 * Output:
 * Description: sets sw1flag 0 if SW1 not pressed, returns 1 if short press and returns 2 if long press. and 3 if in press state
 * Example Call: detectSW1Press();
 */
void detectSW1Press(){
	uint8_t ui8IP4 = GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4);
	if (ui8IP4==0x00){
		sw1Count++;
		sw1flag = 3;
	}else{
		if(sw1Count>=100) sw1flag = 2;
		else if(sw1Count>=2) sw1flag = 1;
		else sw1flag = 0;
		sw1Count=0;
	}
}

/*
 * Function Name: detectSW2Press()
 * Input: none
 * Output:
 * Description: sets sw2flag 0 if SW2 not pressed, returns 1 if short press and returns 2 if long press and 3 if in press state
 * Example Call: detectSW2Press();
 */
void detectSW2Press(){
	uint8_t ui8IP0 = GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0);
	if (ui8IP0==0x00){
		sw2Count++;
		sw2flag = 3;
	}else{
		if(sw2Count>=100) sw2flag = 2;
		else if(sw2Count>=2) sw2flag = 1;
		else sw2flag = 0;
		sw2Count=0;
	}
}
/*
 * Function Name: Timer0IntHandler()
 * Input: none
 * Output: none
 * Description: Interrupt handler for timerA
 * Example Call: Timer0IntHandler();
 */
void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	detectSW1Press();
	detectSW2Press();
	if(sw2flag==3&&sw1flag==1){
		sw1PressCount++;
		if(sw1PressCount==3){
			sw1PressCount = 2;
		}
	}else if(sw2flag==3&&sw1flag==2){
		sw1PressCount=3;
	}else if(sw2flag==2&&sw1PressCount==1){
		currentMode = 1;
	}else if(sw2flag==2&&sw1PressCount==2){
		currentMode = 2;
	}else if((sw2flag==2&&sw1PressCount==3)||(sw1flag==2&&sw2PressCount==3)){
		currentMode = 3;
	}else if(sw1flag==3&&sw2flag==2){
		sw2PressCount = 3;
	}else if(sw1flag==0&&sw2flag==1){
		doSW2Action();
	}else if(sw2flag==0&&sw1flag==1){
		doSW1Action();
	}
	if(sw1flag==0){
		sw2PressCount = 0;
	}
	if(sw2flag==0){
		sw1PressCount = 0;
	}
	currentColorCount++;
	changeColor();
}
/*
 * Function Name: setTimer()
 * Input: none
 * Output: none
 * Description: Enable the timer
 * Example Call: setTimer();
 */
void setTimer(void){
	uint32_t ui32Period;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	// Enables the timer that operates for 10ms
	ui32Period = (SysCtlClockGet() / 100);
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);

	// Enable the interrupt
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	// Enable the timer
	TimerEnable(TIMER0_BASE, TIMER_A);
}

int main(void){

	setup();
	switchPinConfig();
	setPWMOutput();
	setTimer();
	while(1)
	{
	}
}
