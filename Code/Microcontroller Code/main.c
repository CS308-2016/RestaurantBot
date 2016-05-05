#define F_CPU 14745600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <math.h> //included to support power function
#include "lcd.c"

typedef int bool;
#define true 1
#define false 0

void port_init();
void timer5_init();
void velocity(unsigned char, unsigned char);
void motors_delay();



unsigned char ADC_Conversion(unsigned char);
unsigned char ADC_Value;
unsigned char flag = 0;
unsigned char Left_white_line = 0;
unsigned char Center_white_line = 0;
unsigned char Right_white_line = 0;
//unsigned char white = 0;
// data : to store received data from UDR0
unsigned char data; 
//unsigned char sharp;
// wait_for_signal : 1 means wait for signal from xbee
unsigned char wait_for_signal=0;
volatile unsigned long int ShaftCountLeft = 0; //to keep track of left position encoder
volatile unsigned long int ShaftCountRight = 0; //to keep track of right position encoder
//volatile unsigned int Degrees; //to accept angle in degrees for turning


// botId : different botId to different bots to calibrate values differently
unsigned int botId = 1;
unsigned char commands[2][8] = {{'A', 'B', 'C', 'D', 'E', 'F', 'G','H'}, {'I', 'J', 'K', 'L', 'M', 'N','O','P'}};
unsigned char mode;

//flags
unsigned int command_received = 0;
unsigned int forward_flag = 0;
unsigned int right_flag = 0;
unsigned int left_flag = 0;
unsigned int start_flag = 0;
unsigned int back_flag = 0;

unsigned char i=0;
// state variables
// 0-stopped, 1-moving, 2-serving
unsigned int state = 0;
unsigned int prev_state = 0;

// Default Motor values are angle positions of motor 1,2 and 3 of robotic arm in default position
// CUrrent Motor values are angle positions of motor 1,2 and 3 of robotic arm currently

unsigned int DefaultMotor1 =  105;
unsigned int DefaultMotor2 = 0;
unsigned int DefaultMotor3 = 0;
unsigned int CurrentMotor1 = 105;
unsigned int CurrentMotor2 = 0;
unsigned int CurrentMotor3 = 0;

//Configure PORTB 5 pin for servo motor 1 operation
void servo1_pin_config (void)
{
 DDRB  = DDRB | 0x20;  //making PORTB 5 pin output
 PORTB = PORTB | 0x20; //setting PORTB 5 pin to logic 1
}

//Configure PORTB 6 pin for servo motor 2 operation
void servo2_pin_config (void)
{
 DDRB  = DDRB | 0x40;  //making PORTB 6 pin output
 PORTB = PORTB | 0x40; //setting PORTB 6 pin to logic 1
}

//Configure PORTB 7 pin for servo motor 3 operation
void servo3_pin_config (void)
{
 DDRB  = DDRB | 0x80;  //making PORTB 7 pin output
 PORTB = PORTB | 0x80; //setting PORTB 7 pin to logic 1
}


//Function to configure LCD port
void lcd_port_config (void)
{
 DDRC = DDRC | 0xF7; //all the LCD pin's direction set as output
 PORTC = PORTC & 0x80; // all the LCD pins are set to logic 0 except PORTC 7
}

//ADC pin configuration
void adc_pin_config (void)
{
 DDRF = 0x00;
 PORTF = 0x00;
 DDRK = 0x00;
 PORTK = 0x00;
}

void buzzer_pin_config (void)
{
 DDRC = DDRC | 0x08;		//Setting PORTC 3 as outpt
 PORTC = PORTC & 0xF7;		//Setting PORTC 3 logic low to turnoff buzzer
}

//Function to configure ports to enable robot's motion
void motion_pin_config (void)
{
 DDRA = DDRA | 0x0F;
 PORTA = PORTA & 0xF0;
 DDRL = DDRL | 0x18;   //Setting PL3 and PL4 pins as output for PWM generation
 PORTL = PORTL | 0x18; //PL3 and PL4 pins are for velocity control using PWM.
}

//Function to configure INT4 (PORTE 4) pin as input for the left position encoder
void left_encoder_pin_config (void)
{
	DDRE  = DDRE & 0xEF;  //Set the direction of the PORTE 4 pin as input
	PORTE = PORTE | 0x10; //Enable internal pull-up for PORTE 4 pin
}

//Function to configure INT5 (PORTE 5) pin as input for the right position encoder
void right_encoder_pin_config (void)
{
	DDRE  = DDRE & 0xDF;  //Set the direction of the PORTE 4 pin as input
	PORTE = PORTE | 0x20; //Enable internal pull-up for PORTE 4 pin
}

//Function to Initialize PORTS
void port_init()
{
	lcd_port_config(); // lcd port config
	adc_pin_config(); // adc pin config
	motion_pin_config(); // configure ports to enable robot's motion
	buzzer_pin_config(); //configuration for buzzer
	left_encoder_pin_config(); //left encoder pin config
	right_encoder_pin_config(); //right encoder pin config
	servo1_pin_config(); //Configure PORTB 5 pin for servo motor 1 operation
    servo2_pin_config(); //Configure PORTB 6 pin for servo motor 2 operation
    servo3_pin_config(); //Configure PORTB 7 pin for servo motor 3 operation
}

//TIMER1 initialization in 10 bit fast PWM mode
//prescale:256
// WGM: 7) PWM 10bit fast, TOP=0x03FF
// actual value: 52.25Hz
void timer1_init(void)
{
 TCCR1B = 0x00; //stop
 TCNT1H = 0xFC; //Counter high value to which OCR1xH value is to be compared with
 TCNT1L = 0x01;	//Counter low value to which OCR1xH value is to be compared with
 OCR1AH = 0x03;	//Output compare Register high value for servo 1
 OCR1AL = 0xFF;	//Output Compare Register low Value For servo 1
 OCR1BH = 0x03;	//Output compare Register high value for servo 2
 OCR1BL = 0xFF;	//Output Compare Register low Value For servo 2
 OCR1CH = 0x03;	//Output compare Register high value for servo 3
 OCR1CL = 0xFF;	//Output Compare Register low Value For servo 3
 ICR1H  = 0x03;
 ICR1L  = 0xFF;
 TCCR1A = 0xAB; /*{COM1A1=1, COM1A0=0; COM1B1=1, COM1B0=0; COM1C1=1 COM1C0=0}
 					For Overriding normal port functionality to OCRnA outputs.
				  {WGM11=1, WGM10=1} Along With WGM12 in TCCR1B for Selecting FAST PWM Mode*/
 TCCR1C = 0x00;
 TCCR1B = 0x0C; //WGM12=1; CS12=1, CS11=0, CS10=0 (Prescaler=256)
}

// On the buzzer
void buzzer_on (void)
{
 unsigned char port_restore = 0;
 port_restore = PINC;
 port_restore = port_restore | 0x08;
 PORTC = port_restore;
}

// Off the buzzer
void buzzer_off (void)
{
 unsigned char port_restore = 0;
 port_restore = PINC;
 port_restore = port_restore & 0xF7;
 PORTC = port_restore;
}

// Timer 5 initialized in PWM mode for velocity control
// Prescale:256
// PWM 8bit fast, TOP=0x00FF
// Timer Frequency:225.000Hz
void timer5_init()
{
	TCCR5B = 0x00;	//Stop
	TCNT5H = 0xFF;	//Counter higher 8-bit value to which OCR5xH value is compared with
	TCNT5L = 0x01;	//Counter lower 8-bit value to which OCR5xH value is compared with
	OCR5AH = 0x00;	//Output compare register high value for Left Motor
	OCR5AL = 0xFF;	//Output compare register low value for Left Motor
	OCR5BH = 0x00;	//Output compare register high value for Right Motor
	OCR5BL = 0xFF;	//Output compare register low value for Right Motor
	OCR5CH = 0x00;	//Output compare register high value for Motor C1
	OCR5CL = 0xFF;	//Output compare register low value for Motor C1
	TCCR5A = 0xA9;	/*{COM5A1=1, COM5A0=0; COM5B1=1, COM5B0=0; COM5C1=1 COM5C0=0}
 					  For Overriding normal port functionality to OCRnA outputs.
				  	  {WGM51=0, WGM50=1} Along With WGM52 in TCCR5B for Selecting FAST PWM 8-bit Mode*/

	TCCR5B = 0x0B;	//WGM12=1; CS12=0, CS11=1, CS10=1 (Prescaler=64)
}

void adc_init()
{
	ADCSRA = 0x00;
	ADCSRB = 0x00;		//MUX5 = 0
	ADMUX = 0x20;		//Vref=5V external --- ADLAR=1 --- MUX4:0 = 0000
	ACSR = 0x80;
	ADCSRA = 0x86;		//ADEN=1 --- ADIE=1 --- ADPS2:0 = 1 1 0
}

void uart0_init(void)
{
 UCSR0B = 0x00; //disable while setting baud rate
 UCSR0A = 0x00;
 UCSR0C = 0x06;
 UBRR0L = 0x5F; //set baud rate lo
 UBRR0H = 0x00; //set baud rate hi
 UCSR0B = 0x98;
}

//Function For ADC Conversion. It takes a character and return an integer according to that.
unsigned char ADC_Conversion(unsigned char Ch)
{
	unsigned char a;
	if(Ch>7)
	{
		ADCSRB = 0x08;
	}
	Ch = Ch & 0x07;
	ADMUX= 0x20| Ch;
	ADCSRA = ADCSRA | 0x40;		//Set start conversion bit
	while((ADCSRA&0x10)==0);	//Wait for conversion to complete
	a=ADCH;
	ADCSRA = ADCSRA|0x10; //clear ADIF (ADC Interrupt Flag) by writing 1 to it
	ADCSRB = 0x00;
	return a;
}

//Function To Print Sesor Values At Desired Row And Coloumn Location on LCD
void print_sensor(char row, char coloumn,unsigned char channel)
{

	ADC_Value = ADC_Conversion(channel);
	lcd_print(row, coloumn, ADC_Value, 3);
}

//Function for velocity control
void velocity (unsigned char left_motor, unsigned char right_motor)
{
	OCR5AL = (unsigned char)left_motor;
	OCR5BL = (unsigned char)right_motor;
}

void left_position_encoder_interrupt_init (void) //Interrupt 4 enable
{
	cli(); //Clears the global interrupt
	EICRB = EICRB | 0x02; // INT4 is set to trigger with falling edge
	EIMSK = EIMSK | 0x10; // Enable Interrupt INT4 for left position encoder
	sei();   // Enables the global interrupt
}

void right_position_encoder_interrupt_init (void) //Interrupt 5 enable
{
	cli(); //Clears the global interrupt
	EICRB = EICRB | 0x08; // INT5 is set to trigger with falling edge
	EIMSK = EIMSK | 0x20; // Enable Interrupt INT5 for right position encoder
	sei();   // Enables the global interrupt
}

//ISR for right position encoder
ISR(INT5_vect)
{
	ShaftCountRight++;  //increment right shaft position count
}


//ISR for left position encoder
ISR(INT4_vect)
{
	ShaftCountLeft++;  //increment left shaft position count
}

//Function used for setting motor's direction
void motion_set (unsigned char Direction)
{
 unsigned char PortARestore = 0;

 Direction &= 0x0F; 		// removing upper nibbel for the protection
 PortARestore = PORTA; 		// reading the PORTA original status
 PortARestore &= 0xF0; 		// making lower direction nibbel to 0
 PortARestore |= Direction; // adding lower nibbel for forward command and restoring the PORTA status
 PORTA = PortARestore; 		// executing the command
}

// Setting different direction as forward, stop, backward, left, right, soft_left, soft_right, sot_left2, soft_right2
// You can read about these on internet
void forward (void)
{
  motion_set (0x06);
}

void stop (void)
{
  motion_set (0x00);
}

void back (void) //both wheels backward
{
	motion_set(0x09);
}

void left (void) //Left wheel backward, Right wheel forward
{
	motion_set(0x05);
}

void right (void) //Left wheel forward, Right wheel backward
{
	motion_set(0x0A);
}

void soft_left (void) //Left wheel stationary, Right wheel forward
{
	motion_set(0x04);
}

void soft_right (void) //Left wheel forward, Right wheel is stationary
{
	motion_set(0x02);
}

void soft_left_2 (void) //Left wheel backward, right wheel stationary
{
	motion_set(0x01);
}

void soft_right_2 (void) //Left wheel stationary, Right wheel backward
{
	motion_set(0x08);
}

/*
Function Name : angle_rotate
Input :  Degrees (an integer)
Output : Rotates the bot with specified degrees
Logic : First it calculates the number of shaft count to rotate and then checks till the bot is rotated that much or not. It can be used for both left and right rotation.
Example Call : angle_rotate(30)
    */
void angle_rotate(unsigned int Degrees)
{
	float ReqdShaftCount = 0;
	unsigned long int ReqdShaftCountInt = 0;

	ReqdShaftCount = (float) Degrees/ 4.090; // division by resolution to get shaft count
	ReqdShaftCountInt = (unsigned int) ReqdShaftCount;
	ShaftCountRight = 0;
	ShaftCountLeft = 0;

	while (1)
	{
		if((ShaftCountRight >= ReqdShaftCountInt) | (ShaftCountLeft >= ReqdShaftCountInt))
		break;
	}
	stop(); //Stop robot
}

/*
Function Name : linear_distance_mm
Input :  Distance in milimeters
Output : Move the bot in forward direction by a soecific distance
Logic : First it calculates the number of required shaft count to move forward and then goes in while loop where it checks that current shaft count is more than required shaft count or not
Example Call : linear_distance_mm(30)
    */

void linear_distance_mm(unsigned int DistanceInMM)
{
	float ReqdShaftCount = 0;
	unsigned long int ReqdShaftCountInt = 0;

	ReqdShaftCount = DistanceInMM / 5.338; // division by resolution to get shaft count
	ReqdShaftCountInt = (unsigned long int) ReqdShaftCount;

	ShaftCountRight = 0;
	while(1)
	{
		if(ShaftCountRight > ReqdShaftCountInt)
		{
			break;
		}
	}
	stop(); //Stop robot
}

void forward_mm(unsigned int DistanceInMM)
{
	forward();
	linear_distance_mm(DistanceInMM);
}

void back_mm(unsigned int DistanceInMM)
{
	back();
	linear_distance_mm(DistanceInMM);
}

void left_degrees(unsigned int Degrees)
{
	// 88 pulses for 360 degrees rotation 4.090 degrees per count
	left(); //Turn left
	angle_rotate(Degrees);
}



void right_degrees(unsigned int Degrees)
{
	// 88 pulses for 360 degrees rotation 4.090 degrees per count
	right(); //Turn right
	angle_rotate(Degrees);
}

//For soft lefts and soft rights we need to rotate twice as given as only one wheel is rotating and other is fixed while in normal left and right both wheels will rotate
void soft_left_degrees(unsigned int Degrees)
{
	// 176 pulses for 360 degrees rotation 2.045 degrees per count
	soft_left(); //Turn soft left
	Degrees=Degrees*2;
	angle_rotate(Degrees);
}

void soft_right_degrees(unsigned int Degrees)
{
	// 176 pulses for 360 degrees rotation 2.045 degrees per count
	soft_right();  //Turn soft right
	Degrees=Degrees*2;
	angle_rotate(Degrees);
}

void soft_left_2_degrees(unsigned int Degrees)
{
	// 176 pulses for 360 degrees rotation 2.045 degrees per count
	soft_left_2(); //Turn reverse soft left
	Degrees=Degrees*2;
	angle_rotate(Degrees);
}

void soft_right_2_degrees(unsigned int Degrees)
{
	// 176 pulses for 360 degrees rotation 2.045 degrees per count
	soft_right_2();  //Turn reverse soft right
	Degrees=Degrees*2;
	angle_rotate(Degrees);
}

// This Function calculates the actual distance in millimeters(mm) from the input
// analog value of Sharp Sensor.
unsigned int Sharp_GP2D12_estimation(unsigned char adc_reading)
{
	float distance;
	unsigned int distanceInt;
	distance = (int)(10.00*(2799.6*(1.00/(pow(adc_reading,1.1546)))));
	distanceInt = (int)distance;
	if(distanceInt>800)
	{
		distanceInt=800;
	}
	return distanceInt;
}

// ISR for interrupt service routine
ISR(USART0_RX_vect) 		// ISR for receive complete interrupt
{
	data = UDR0; 				//making copy of data from UDR0 in 'data' variable

    if(data == commands[botId][7]) 
    {
        UDR0 = mode;
    }
    //default
    else
    {
        for(i=0;i<7;i++){
            if(data==commands[botId][i]){
                command_received=1;
            }
        }
    }
}

//Function to rotate Servo 1 by a specified angle in the multiples of 1.86 degrees
void servo_1(unsigned char degrees)
{
 float PositionPanServo = 0;
  PositionPanServo = ((float)degrees / 1.86) + 35.0;
 OCR1AH = 0x00;
 OCR1AL = (unsigned char) PositionPanServo;
}


//Function to rotate Servo 2 by a specified angle in the multiples of 1.86 degrees
void servo_2(unsigned char degrees)
{
 float PositionTiltServo = 0;
 PositionTiltServo = ((float)degrees / 1.86) + 35.0;
 OCR1BH = 0x00;
 OCR1BL = (unsigned char) PositionTiltServo;
}

//Function to rotate Servo 3 by a specified angle in the multiples of 1.86 degrees
void servo_3(unsigned char degrees)
{
 float PositionServo = 0;
 PositionServo = ((float)degrees / 1.86) + 35.0;
 OCR1CH = 0x00;
 OCR1CL = (unsigned char) PositionServo;
}

//servo_free functions unlocks the servo motors from the any angle
//and make them free by giving 100% duty cycle at the PWM. This function can be used to
//reduce the power consumption of the motor if it is holding load against the gravity.

void servo_1_free (void) //makes servo 1 free rotating
{
 OCR1AH = 0x03;
 OCR1AL = 0xFF; //Servo 1 off
}

void servo_2_free (void) //makes servo 2 free rotating
{
 OCR1BH = 0x03;
 OCR1BL = 0xFF; //Servo 2 off
}

void servo_3_free (void) //makes servo 3 free rotating
{
 OCR1CH = 0x03;
 OCR1CL = 0xFF; //Servo 3 off
}

/*
Function Name : set_default_position
Input :  Nothing
Output : Move the robotic arm to it's default position
Logic : It goes by one by one. First it puts motor3 in default position then motor2 and then motor1. For every motor it divides the difference between current position and default position in small steps so that motor moves softly.
Example Call : set_default_position()
    */

void set_default_position(){
    int value = -1;
    if(CurrentMotor1>DefaultMotor1) value = 1;

    int difference = CurrentMotor3- DefaultMotor3;
    int change = difference/10;
    for (i = 0; i <=9; i++){
        CurrentMotor3 = CurrentMotor3 - change;
        if(CurrentMotor3>180) CurrentMotor3 = 0;
        servo_3(CurrentMotor3);
        _delay_ms(10);
    }
    servo_3(DefaultMotor3);
    CurrentMotor3 = DefaultMotor3;

    difference = CurrentMotor2- DefaultMotor2;
    change = difference/10;
    for (i = 0; i <=9; i++){
        CurrentMotor2 = CurrentMotor2 - change;
        if(CurrentMotor2>180) CurrentMotor2 = 0;
        servo_2(CurrentMotor2);
        _delay_ms(10);
    }
    servo_2(DefaultMotor2);
    CurrentMotor2 = DefaultMotor2;

    if(CurrentMotor1>DefaultMotor1){
        difference = CurrentMotor1 - DefaultMotor1;
    }else{
        difference = DefaultMotor1 - CurrentMotor1;
    }
    change = difference/10;
    for (i = 0; i <=9; i++){
        CurrentMotor1 = CurrentMotor1 - change*value;
        if(CurrentMotor1>200) CurrentMotor1 = 0;
        servo_1(CurrentMotor1);
        _delay_ms(10);
    }
    servo_1(DefaultMotor1);
    CurrentMotor1 = DefaultMotor1;
}

/*
Function Name : set_position
Input :  an integer either 1 or 2
Output : if input is 1 it sets the robotic arm to serve the food positioned left and 2 is for right.
Logic : First it rotates the second motor softly. The motor 1 decides that robotic arm will go in left or right so it rotates the first motor according to the input. Then it just rotates the third motor to put in just behind of the food.
Example Call : set__position(1)
    */

void set_position(unsigned int num){
    int value = 1;
    if(num==1) value = -1;
    /**/

    CurrentMotor2 = DefaultMotor2;
    for (i = 0; i <=27; i++)
    {
        CurrentMotor2 = CurrentMotor2 + 5;
        if(CurrentMotor2>180) CurrentMotor2 = 180;
        servo_2(CurrentMotor2);
        _delay_ms(30);
    }
    CurrentMotor2 = CurrentMotor2 + 4;
    if(CurrentMotor2>180) CurrentMotor2 = 180;
    servo_2(CurrentMotor2);

    for (i = 0; i <=32; i++)
    {
        servo_1(CurrentMotor1+3*value);
        CurrentMotor1 = CurrentMotor1 + 3*value;
        _delay_ms(20);

    }
    servo_3(5);
    CurrentMotor3 = 5;

    return;
}

/*
Function Name : push_food
Input :  Nothing
Output : Move the third motor of the robotic arm front to push the food and come back after it.
Example Call : push_food()
    */
void push_food(){
    for (i = 0; i <=19; i++){
        CurrentMotor3 = CurrentMotor3 + 3;
        if(CurrentMotor3>180) CurrentMotor3 = 180;
        servo_3(CurrentMotor3);
        //_delay_ms(5);
    }
    _delay_ms(2000);
    for (i = 0; i <=69; i++){
        CurrentMotor3 = CurrentMotor3 - 1;
        if(CurrentMotor3>180) CurrentMotor3 = 0;
        servo_3(CurrentMotor3);
        _delay_ms(10);
    }
    return;
}

/*
Function Name : serve_food
Input :  Integer (1 for left and 2 for right)
Output : Serves the food and then goes to default position
Example Call : serve_food(1)
    */

void serve_food(unsigned int num){
    buzzer_on();
    _delay_ms(500);
    buzzer_off();
    set_position(num);
    _delay_ms(2000);
    push_food();
    _delay_ms(2000);
    set_default_position();

    return;
}

// In starting arm should be in default position
void arm_init(void)
{
    servo_1(DefaultMotor1);
    servo_2(DefaultMotor2);
    servo_3(DefaultMotor3);
}

//Initialization of all devices
void init_devices (void)
{
 	cli(); //Clears the global interrupts
	port_init();
	left_position_encoder_interrupt_init();
	right_position_encoder_interrupt_init();
	uart0_init(); //Initailize UART1 for serial communiaction
	adc_init();
	timer1_init();
	arm_init();
	//timer5_init();
	sei();   //Enables the global interrupts
}

//print led values values
void readAndPrintLed (void)
{
    Left_white_line = ADC_Conversion(3);	//Getting data of Left WL Sensor
    Center_white_line = ADC_Conversion(2);	//Getting data of Center WL Sensor
    Right_white_line = ADC_Conversion(1);	//Getting data of Right WL Sensor
    print_sensor(1,1,3);	//Prints value of White Line Sensor1
    print_sensor(1,5,2);	//Prints Value of White Line Sensor2
    print_sensor(1,9,1);	//Prints Value of White Line Sensor3

}

// moves forward 10 milimeter with velocity 100,100
void move_forward(void){
    velocity(100, 100);
    forward_mm(10);
}

/*
Function Name : handleXbee
Output : handles the xbee communication between the server (laptop) and the bot.
Logic : command_received variable denotes that bot has received a command or not. This is changed by ISR. Now here the bot take the command and do something according to the command.
Example Call : handleXbee()
    */

void handleXbee(void)
{
	// Checking fo command received
    if(command_received==1)
    {
        wait_for_signal=0;
        buzzer_off();
        state = 1;
        // If command received is 'A' {check commands array for this defined as global variable}
        if(data == commands[botId][0])
        {
        	// Command 'A' for move forward
            mode = commands[botId][4];
            move_forward();
            readAndPrintLed();
        }
        else if(data == commands[botId][1])
        {
            // Command 'B' for turning right
            mode = commands[botId][4];
            right_flag = 1;
        }
        else if(data == commands[botId][2]){
        	// Command 'C' for turning back
        	mode = commands[botId][4];
        	back_flag = 1;
        }
        else if(data == commands[botId][3])
        {
        	// Command 'D' for turning left
            mode = commands[botId][4];
            left_flag = 1;
        }
        else if(data == commands[botId][4])
        {
        	// Command 'E' for serving left food
            mode = commands[botId][4];
            serve_food(1);
            state = 0;
            mode = commands[botId][5];
        }
        else if(data == commands[botId][5])
        {
        	// Command 'F'for serving right food
            mode = commands[botId][4];
            serve_food(2);
            state = 0;
            mode = commands[botId][5];
        }
        else if(data == commands[botId][6])
        {
        	// Command 'G' to stop the bot
            stop();
        }
        command_received=0;

    }
}

// check left infrared sensor on base of bot
// returns one if on white line
bool check_left_ir(void){
    if(botId==0) return Left_white_line<0x28;
    return Left_white_line<0x50;
}

// check right infrared sensor on base of bot
// returns one if on white line
bool check_right_ir(void){
    return Right_white_line<0x28;
}

// check middle infrared sensor on base of bot
// returns one if on white line
bool check_middle_ir(void){
    return Center_white_line<0x28;
}

// action when we reach the node
/*
Function Name : handle_node
Output : handles what should bot do after reaching a node
Logic : If all three sensors are on white we detect that we have reached a node. It sends signal through xbee that it reached a node and wait for a signal for next command.
Example Call : handle_node()
    */

void handle_node(void){
    stop();
    _delay_ms(500);
    wait_for_signal=1;
    while(wait_for_signal){
        handleXbee();
    }
    //state is 1 means moving
    if(state==1){
        if(right_flag==1)
        {
        	// Right flag is 1 means the bot has to turn right.
        	// Logic for moving right is first go forward 7 cm then take a turn of 40 degrees and then take turn 5-5 degrees till u find the white line 
            forward_mm(70);
            right_degrees(40);
            readAndPrintLed();
            while(!check_middle_ir()){
                readAndPrintLed();
                right_degrees(5);
            }
            right_flag=0;
        }
        else if(left_flag==1)
        {
        	// Left flag is 1 means the bot has to turn left.
        	// Logic is same as for right
            forward_mm(70);
            left_degrees(40);
            readAndPrintLed();
            while(!check_middle_ir()){
                readAndPrintLed();
                left_degrees(5);
            }
            left_flag=0;
        }
        else if (back_flag==1){
        	// Back flag is 1 means the bot has to turn back.
        	// Logic is same as for right just the starting angle is now 160 instead of 40
        	forward_mm(70);
            right_degrees(160);
            readAndPrintLed();
            while(!check_middle_ir()){
                readAndPrintLed();
                right_degrees(5);
            }
            back_flag=0;
        }
        else
        {
            if(botId == 0)
                forward_mm(20);
            move_forward();
        }
    }
}

// moves left
void move_left(void){
    stop();
    _delay_ms(200);
    soft_left_degrees(4);
}

// moves right
void move_right(void){
    stop();
    _delay_ms(200);
    soft_right_degrees(4);
}

//Main function which operates all devices and their operations
int main()
{
    mode = commands[botId][5];
    unsigned int value;
	init_devices();
	lcd_set_4bit();
	lcd_init();
    //waiting for start flag to set. done
	while(1)
	{
        readAndPrintLed();
        handleXbee();
        if(state==1){
        	// Here it will check the sensor values of all three sensors and take decision according to those values
            if(!check_left_ir() && !check_right_ir() && check_middle_ir())
            {
            	// Only middle sensor is on white so move forward
                mode=commands[botId][0];
                move_forward();
            }
            else if(check_left_ir() && check_right_ir() && check_middle_ir())
            {
            	// All three sensors are on means we reached a node and hanle node
                mode=commands[botId][3];
                handle_node();
            }
            else if(check_left_ir())
            {
            	//left sensor is on means the bot is tilted slightly so rotate it slightly
                mode=commands[botId][2];
                move_left();
            }
            else if(check_right_ir())
            {
                mode=commands[botId][1];
                move_right();
            }
            else
            {
            	// No sensor is on white so we are not on the track.
                stop();
            }
        }else{
            stop();
        }
	}
}