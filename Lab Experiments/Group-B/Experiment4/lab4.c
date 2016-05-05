#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"

uint32_t ui32ADC0Value[4];
volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;
int mode = 0;
int set_temp =  25;
int curr_temp = 0;

void UARTIntHandler(void)
{
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
	while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
	{
		unsigned char s_detector = UARTCharGetNonBlocking(UART0_BASE);
		if(mode==0){
			if(s_detector == 'S' || s_detector == 's'){
				mode = 1;
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);
				UARTCharPut(UART0_BASE, 'E');
				UARTCharPut(UART0_BASE, 'n');
				UARTCharPut(UART0_BASE, 't');
				UARTCharPut(UART0_BASE, 'e');
				UARTCharPut(UART0_BASE, 'r');
				UARTCharPut(UART0_BASE, ':');
				UARTCharPut(UART0_BASE, ' ');
			}
		}
		else{
			//UARTCharPut(UART0_BASE, 'a');
			int ss = s_detector;
			if(ss == 13){
				//UARTCharPut(UART0_BASE, 'b');
				set_temp = curr_temp;
				curr_temp = 0;
				UARTCharPut(UART0_BASE, '\r');
				UARTCharPut(UART0_BASE, '\n');

				UARTCharPut(UART0_BASE, 'S');
				UARTCharPut(UART0_BASE, 'e');
				UARTCharPut(UART0_BASE, 't');
				UARTCharPut(UART0_BASE, ' ');
				UARTCharPut(UART0_BASE, 't');
				UARTCharPut(UART0_BASE, 'e');
				UARTCharPut(UART0_BASE, 'm');
				UARTCharPut(UART0_BASE, 'p');
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
				uint32_t yo = set_temp;
				int div = 1;
				while(yo!=0){
					yo = yo/10;
					div = div*10;
				}
				yo = set_temp;
				div = div/10;
				while(div!=0){
					uint32_t remain = yo/div;
					unsigned char xyz = remain+'0';
					UARTCharPut(UART0_BASE, xyz);
					yo = yo - remain*div;
					div = div/10;
				}

				UARTCharPut(UART0_BASE, ' ');
				unsigned char xyz = 167;
				UARTCharPut(UART0_BASE, xyz);
				UARTCharPut(UART0_BASE, 'C');


				UARTCharPut(UART0_BASE, '\r');
				UARTCharPut(UART0_BASE, '\n');
				//Set Temp updated to XX ºC

				mode = 0;
			}
			else{
				//UARTCharPut(UART0_BASE, 'c');
				UARTCharPut(UART0_BASE, s_detector);
				if(ss == 127) curr_temp = curr_temp/10;
				else curr_temp = curr_temp*10 + (s_detector - '0');
			}
		}
		//UARTCharPut(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE)); //echo character
		//UARTCharPut(UART0_BASE, 'r');
		//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //blink LED
		//SysCtlDelay(SysCtlClockGet() / (1000 * 3)); //delay ~1 msec
		//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //turn off LED
	}
}

int main(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	ADCHardwareOversampleConfigure(ADC0_BASE, 64);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //enable GPIO port for LED

	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, 1);

	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_3); //enable pin for LED PF


	//interrupt section
	IntMasterEnable(); //enable processor interrupts
	IntEnable(INT_UART0); //enable the UART interrupt
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); //only enable RX and TX interrupts

	while(1){
		// Task - 1
		/*ADCIntClear(ADC0_BASE, 1);
		ADCProcessorTrigger(ADC0_BASE, 1);
		while(!ADCIntStatus(ADC0_BASE, 1, false)){}

		ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
		ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
		ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
		ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;
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
		UARTCharPut(UART0_BASE, 'e');
		UARTCharPut(UART0_BASE, 'r');
		UARTCharPut(UART0_BASE, 'a');
		UARTCharPut(UART0_BASE, 't');
		UARTCharPut(UART0_BASE, 'u');
		UARTCharPut(UART0_BASE, 'r');
		UARTCharPut(UART0_BASE, 'e');
		UARTCharPut(UART0_BASE, ' ');
		uint32_t yo = ui32TempValueC;
		int div = 1;
		while(yo!=0){
			yo = yo/10;
			div = div*10;
		}
		yo = ui32TempValueC;
		div = div/10;
		while(div!=0){
			uint32_t remain = yo/div;
			unsigned char xyz = remain+'0';
			UARTCharPut(UART0_BASE, xyz);
			yo = yo - remain*div;
			div = div/10;
		}

		UARTCharPut(UART0_BASE, ' ');
		UARTCharPut(UART0_BASE, '\'');
		UARTCharPut(UART0_BASE, 'C');
		UARTCharPut(UART0_BASE, '\r');
		UARTCharPut(UART0_BASE, '\n');
		SysCtlDelay(SysCtlClockGet() / 3);

*/
		// Task - 2
		// monitor
		if(mode==0)
		{
			ADCIntClear(ADC0_BASE, 1);
			ADCProcessorTrigger(ADC0_BASE, 1);
			while(!ADCIntStatus(ADC0_BASE, 1, false))
			{
			}
			ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
			ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;
			ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
			ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;


			//led glow code
			if(ui32TempValueC < set_temp)
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 8);
			}
			else
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 2);
			}

			//display
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

			uint32_t yo = ui32TempValueC;
			int div = 1;
			while(yo!=0){
				yo = yo/10;
				div = div*10;
			}
			yo = ui32TempValueC;
			div = div/10;
			while(div!=0){
				uint32_t remain = yo/div;
				unsigned char xyz = remain+'0';
				UARTCharPut(UART0_BASE, xyz);
				yo = yo - remain*div;
				div = div/10;
			}

			UARTCharPut(UART0_BASE, ' ');
			unsigned char xyz = 176;
			UARTCharPut(UART0_BASE, xyz);
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

			yo = set_temp;
			div = 1;
			while(yo!=0){
				yo = yo/10;
				div = div*10;
			}
			yo = set_temp;
			div = div/10;
			while(div!=0){
				uint32_t remain = yo/div;
				unsigned char xyz = remain+'0';
				UARTCharPut(UART0_BASE, xyz);
				yo = yo - remain*div;
				div = div/10;
			}

			UARTCharPut(UART0_BASE, ' ');
			UARTCharPut(UART0_BASE, xyz);
			UARTCharPut(UART0_BASE, 'C');
			UARTCharPut(UART0_BASE, '\r');
			UARTCharPut(UART0_BASE, '\n');
			SysCtlDelay(30000000);
		}
	}
}
