#include<stdint.h>
#include<stdbool.h>
#include"inc/hw_memmap.h"
#include"inc/hw_types.h"
#include"driverlib/debug.h"
#include"driverlib/sysctl.h"
#include"driverlib/adc.h"
#include"driverlib/gpio.h"
#include"driverlib/pin_map.h"
#include"driverlib/uart.h"
#define TARGET_IS_BLIZZARD_RB1
#include "driverlib/rom.h"
#include"inc/hw_ints.h"
#include"driverlib/interrupt.h"
#include "driverlib/timer.h"

uint32_t ui32TempAvg; // Storing Avg. Temperature
uint32_t ui32TempValueC; // Storing Temp. in Celsius
uint32_t ui32SetTempValueC; // Storing Temp. in Celsius
uint32_t ui32TempValueF; // Storing Temp. in Fahrenheits
uint32_t mode; // 0: Monitor mode 1: Set mode

/*
 * Function Name: clearscreen
 * Input: none
 * Output: none
 * Description: Clears the screen
 * Example Call: clearscreen();
 */
void clearscreen(void) {
	int i;
	for (i = 0; i < 50; i++) {
		UARTCharPut(UART0_BASE, ' ');
	}
	UARTCharPut(UART0_BASE, '\r');
}

/*
 * Function Name: setup()
 * Input: none
 * Output: none
 * Description: Set system clock frequency and configure ADC
 * Example Call: setup();
 */
void setup(void) {
	SysCtlClockSet(
	SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // CPU is running at 40MHz

	// ADC Configure
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Enables ADC0
	ADCHardwareOversampleConfigure(ADC0_BASE, 64); // Hardware averages 64 readings and sends it to Sequencer

	// UART Configure
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); // UART0 Enabled
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // GPIOA Enabled
	GPIOPinConfigure(GPIO_PA0_U0RX); // Pin 0 of GPIOA set as Rx
	GPIOPinConfigure(GPIO_PA1_U0TX); // Pin 1 of GPIOA set as Tx
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1); // Port A defined as UART Base

	// LED Configure
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // GPIOF Enabled
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,
	GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3); // Pin 1,2,3 of Port F set as Output
}

/*
 * Function Name: adcSequencerConfigure
 * Input: none
 * Output: none
 * Description: Confures the ADC Sequencer
 * Example Call: adcSequencerConfigure();
 */
void adcSequencerConfigure(void) {
	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0); // Uses ADC0, sample sequencer 1, the processor triggers the sequence and uses the highest priority

	// Configuring outputs 0-2 of Sequencer1 to sample the temperature sensor
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);

	// Configuring output 3 of Sequencer1
	// Sample the temperature sensor (ADC_CTL_TS)
	// Configure the interrupt flag (ADC_CTL_IE) to be set when the sample is done
	// Tell the ADC logic that this is the last conversion on sequencer 1 (ADC_CTL_END)
	ADCSequenceStepConfigure(ADC0_BASE, 1, 3,
	ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
}
/*
 * Function Name: UARTIntHandler
 * Input: none
 * Output: none
 * Description: Interrupt handler for UART Int
 * Example Call: UARTIntHandler();
 */
void UARTIntHandler(void) {
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
	while (UARTCharsAvail(UART0_BASE)) //loop while there are chars
	{
		unsigned char a = UARTCharGetNonBlocking(UART0_BASE);

		if (a == 'S') {
			mode = 1;
			clearscreen();
			UARTCharPut(UART0_BASE, '\r');
			UARTCharPut(UART0_BASE, 'E');
			UARTCharPut(UART0_BASE, 'n');
			UARTCharPut(UART0_BASE, 't');
			UARTCharPut(UART0_BASE, 'e');
			UARTCharPut(UART0_BASE, 'r');
			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, 't');
			UARTCharPut(UART0_BASE, 'h');
			UARTCharPut(UART0_BASE, 'e');
			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, 't');
			UARTCharPut(UART0_BASE, 'e');
			UARTCharPut(UART0_BASE, 'm');
			UARTCharPut(UART0_BASE, 'p');
			UARTCharPut(UART0_BASE, ':');
			UARTCharPut(UART0_BASE, ' ');
			unsigned char x = UARTCharGet(UART0_BASE);
			UARTCharPut(UART0_BASE, x);
			unsigned char y = UARTCharGet(UART0_BASE);
			UARTCharPut(UART0_BASE, y);
			ui32SetTempValueC = (x - '0') * 10 + (y - '0');
			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, 'T');
			UARTCharPut(UART0_BASE, 'e');
			UARTCharPut(UART0_BASE, 'm');
			UARTCharPut(UART0_BASE, 'p');
			UARTCharPut(UART0_BASE, 'e');
			UARTCharPut(UART0_BASE, 'r');
			UARTCharPut(UART0_BASE, 'a');
			UARTCharPut(UART0_BASE, 't');
			UARTCharPut(UART0_BASE, 'u');
			UARTCharPut(UART0_BASE, 'r');
			UARTCharPut(UART0_BASE, 'e');
			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, 'u');
			UARTCharPut(UART0_BASE, 'p');
			UARTCharPut(UART0_BASE, 'd');
			UARTCharPut(UART0_BASE, 'a');
			UARTCharPut(UART0_BASE, 't');
			UARTCharPut(UART0_BASE, 'e');
			UARTCharPut(UART0_BASE, 'd');
			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, 't');
			UARTCharPut(UART0_BASE, 'o');
			UARTCharPut(UART0_BASE, ':');
			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, x);
			UARTCharPut(UART0_BASE, y);
			UARTCharPut(UART0_BASE, '\r');
			SysCtlDelay(SysCtlClockGet()); //delay 1 sec
			mode = 0;
		} else {
			UARTCharPutNonBlocking(UART0_BASE, a); //echo character
		}
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //blink LED
		SysCtlDelay(SysCtlClockGet() / (1000 * 3)); //delay ~1 msec
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //turn off LED
	}
}

/*
 * Function Name: Timer0IntHandler()
 * Input: none
 * Output: none
 * Description: Interrupt handler for timerA
 * Example Call: Timer0IntHandler();
 */
void Timer0IntHandler(void) {
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	unsigned char deg = 176;
	if (mode == 0) {
		clearscreen();
		UARTCharPut(UART0_BASE, 'C');
		UARTCharPut(UART0_BASE, 'u');
		UARTCharPut(UART0_BASE, 'r');
		UARTCharPut(UART0_BASE, 'r');
		UARTCharPut(UART0_BASE, 'e');
		UARTCharPut(UART0_BASE, 'n');
		UARTCharPut(UART0_BASE, 't');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, 'T');
		UARTCharPut(UART0_BASE, 'e');
		UARTCharPut(UART0_BASE, 'm');
		UARTCharPut(UART0_BASE, 'p');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, (ui32TempValueC) / 10 + '0');
		UARTCharPut(UART0_BASE, (ui32TempValueC) % 10 + '0');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, deg);
		UARTCharPut(UART0_BASE, 'C');
		UARTCharPut(UART0_BASE, ',');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, 'S');
		UARTCharPut(UART0_BASE, 'e');
		UARTCharPut(UART0_BASE, 't');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, 'T');
		UARTCharPut(UART0_BASE, 'e');
		UARTCharPut(UART0_BASE, 'm');
		UARTCharPut(UART0_BASE, 'p');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, (ui32SetTempValueC) / 10 + '0');
		UARTCharPut(UART0_BASE, (ui32SetTempValueC) % 10 + '0');
		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, deg);
		UARTCharPut(UART0_BASE, 'C');
		UARTCharPut(UART0_BASE, '\r');
	}
}

/*
 * Function Name: setTimer()
 * Input: none
 * Output: none
 * Description: Enable the timer
 * Example Call: setTimer();
 */
void setTimer(void) {
	uint32_t ui32Period;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

	// Enables the timer that operates for 1000ms
	ui32Period = (SysCtlClockGet());
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);

	// Enable the interrupt
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();

	// Enable the timer
	TimerEnable(TIMER0_BASE, TIMER_A);
}

/*
 * Function Name: setUART()
 * Input: none
 * Output: none
 * Description: Enable the UART Interrupts
 * Example Call: setUART();
 */
void setUART(void) {
	// UART Enabled
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	IntMasterEnable(); // Enables the processor interrupt
	IntEnable(INT_UART0); // Enables UART Interrupts
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); // Enables interrupt on UART for Rx and Rt
}

int main(void) {
	mode = 0;
	ui32SetTempValueC = 25;
	uint32_t ui32ADC0Value[4]; // Storing the data read from FIFO of Sequencer 1

	setup();
	adcSequencerConfigure();
	setTimer();
	setUART();

	while (1) {
		// Enable Sequencer 1
		ADCSequenceEnable(ADC0_BASE, 1);

		ADCIntClear(ADC0_BASE, 1); // Clear ADC Interrupt Flag
		ADCProcessorTrigger(ADC0_BASE, 1); // Trigger ADC Processor

		// Waiting until AD conversion is complete
		// TODO: Use interrupts instead of while loop
		while (!ADCIntStatus(ADC0_BASE, 1, false)) {
		}

		ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);// Reads the data from Sequencer 1 to memory

		ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2]
				+ ui32ADC0Value[3] + 2) / 4; // Averaging temperature data and using 2 for rounding
		ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096) / 10; // Calculate the Celsius value of the temperature

		ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5; // Celsius to fahrenheit conversion
		if (ui32SetTempValueC >= ui32TempValueC) {
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
					8); //blink LED
		} else {
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
					2); //blink LED
		}
	}
}
