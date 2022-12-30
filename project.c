#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "lcd.h"
#include "project.h"
#include "serial.h"
#include "encoder.h"
#define FOSC 16000000 // Clock frequency 
#define BAUD 9600 // Baud rate used
#define MYUBRR (FOSC/16/BAUD-1) // Value for UBRR0

/* Aline Garcia-Sanchez
EE109 Speed Trap Project:
Build a device that measures how fast an object is moving as it passes two sensors 
and display the speed on a dial-type indicator. It also relays that speed to a remote device.

*/

enum state {START, STOP};
volatile unsigned int time, speed, cnt, buzzCNT, r, count, dataCNT, new_speed;
volatile unsigned char timer, sensor, buzzer, dataValid, startData;  // Flags
volatile int state = STOP;
volatile unsigned char charBuff[5];  //5 byte buffer
volatile unsigned char changed;
volatile unsigned char a, b;
volatile unsigned char new_state, old_state;

int main(void) {
	//Enable Local Interupts
	PCICR |= (1 << PCIE1);
	PCMSK1 |= (1 << PCINT9)|(1 << PCINT10)|(1 << PCINT13);//for C
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PCINT19)|(1 << PCINT18);//for D
	UCSR0B |= (1 << RXCIE0); //reciever interrupt
	UBRR0 = MYUBRR;

	//Enable Global Interupts
	sei();

	UCSR0B |= (1 << TXEN0 | 1 << RXEN0);//enable reciever and transimitter
	UCSR0C = (3 << UCSZ00);

    // Initialize DDR and PORT registers and LCD
	DDRD |= (1 << 3)|(1 << 2);
	DDRC |= (1 << 5)|(1 << 4)|(1 << 3);
	DDRB |= (1 << 3)|(1 << 4)|(1 << 5); //turn on servo motor
	PORTD |= (1 << 3)|(1 << 2);//pull up resistors for Rotary Encoder
	PINC &= ~(1 << 4); 

	lcd_init();
	timer2_init();
	timer1_init();
	timer0_init();
	rotaryEncode_init();
	screenSetup();

	state = STOP;

	PORTB &= ~(1 << 5);

    while (1) {                 // Loop forever
		if(timer){
			timer = 0;
			lcd_moveto(0,0);
			lcd_stringout("time expired    ");
			PORTC &= ~(1 << 3);
			state = STOP;
		}
        if(sensor) { // Did state change?
	    	sensor = 0;        // Reset changed flag

			cnt = (TCNT1 / (62500/4000));

			//distance is 5cm
			speed = (5000)/(cnt);
			r = (5000)%(cnt);
			//r /= 100;
			//Output count to LCD
			char buf[13];
			snprintf(buf, 13, "%3dms = %u.%u    ", cnt, speed, r%10);
			//snprintf(buf, 13, "%3dms = %lu.%lu    ", cnt, speed/10, speed%10);
			lcd_moveto(0,0);
			lcd_stringout(buf);

			if(speed > count){
				buzzCNT = 0; //reset count
				TCCR0B |= ((1 << CS02)); //turn buzzer interrupt on
			}
			/*determining */
			int x = 35 - ((speed*23)/100);
			if(x < 0){
				x = 0;
			}
			OCR2A = x;
			OCR1B = 0;
			if(speed > new_speed/10){
				PORTB |= (1 << 5);
				PORTB &= ~(1 << 4);
				// _delay_ms(200);
				// PORTB &= ~(1 << 5);
			}
			else if(speed < new_speed/10){
				PORTB |= (1 << 4);
				PORTB &= ~(1 << 5);
				// _delay_ms(200);
				// PORTB &= ~(1 << 4);
			}
			else{
				//then check for the decimal points
				if(r == (new_speed%10)){
					PORTB &= ~(1 << 5);
					PORTB &= ~(1 << 4);
				}
				else if(r > (new_speed%10)){
					PORTB |= (1 << 5);
					PORTB &= ~(1 << 4);
				}
				else if(r < (new_speed%10)){
					PORTB |= (1 << 4);
					PORTB &= ~(1 << 5);
				}
			}

			//send the speed data
			char temp[6];
			snprintf(temp, 6, "%u%u", speed, r%10);
			int i = 0;
			tx_send('[');
			while(temp[i] != '\0'){
				tx_send(temp[i]);
				i++;
			}
			tx_send(']');
        }
		if(changed){
			changed = 0;

			char buf5[10];
			eeprom_update_byte((void *) 100, count);
			lcd_moveto(1,7);
			lcd_stringout(" ");
			snprintf(buf5, 10, "MAX= %2d", count);
			lcd_moveto(1,0);
			lcd_stringout(buf5);
		}
		if(dataValid){
			dataValid = 0;
			lcd_moveto(1, 11);
			lcd_stringout("   ");
			char buff6[5];
			sscanf(charBuff, "%u", &new_speed);
			snprintf(buff6, 5, "%u.%u", (new_speed/10),(new_speed%10));
			lcd_moveto(1, 8);
			lcd_stringout(buff6);
			if(speed > new_speed/10){
				PORTB |= (1 << 5);
				PORTB &= ~(1 << 4);
				// _delay_ms(200);
				// PORTB &= ~(1 << 5);
			}
			else if(speed < new_speed/10){
				PORTB |= (1 << 4);
				PORTB &= ~(1 << 5);
				// _delay_ms(200);
				// PORTB &= ~(1 << 4);
			}
			else{
				//then check for the decimal points
				if(r == (new_speed%10)){
					PORTB &= ~(1 << 5);
					PORTB &= ~(1 << 4);
				}
				else if(r > (new_speed%10)){
					PORTB |= (1 << 5);
					PORTB &= ~(1 << 4);
				}
				else if(r < (new_speed%10)){
					PORTB |= (1 << 4);
					PORTB &= ~(1 << 5);
				}
			}
		}
    }
}
//Buzzer Interrupt trying to figure out
ISR(TIMER0_COMPA_vect){ //buzzer interrupt
	PORTC ^= (1 << 5);
	buzzCNT++;
	if(buzzCNT > 330){
		TCCR0B &= ~((1 << CS12)|(1 << CS11)|(1 << CS10));
	}
}
// interrupt service routine
ISR(TIMER1_COMPA_vect){ //counter interrupt
	TCCR1B &= ~((1 << CS12)|(1 << CS11)|(1 << CS10));
	timer = 1;
	state = STOP;
}
ISR(PCINT1_vect){ //sensor interrupts
	unsigned int read = PINC;
	unsigned char start1 = (read & (1 << 1));
	unsigned char stop1 = (read & (1 << 2));
	if(state == STOP){
		if(!start1){
			state = START;
			TCNT1 = 0;
			TCCR1B |= (1 << CS12)|(1 << CS10);
			PORTC |= (1 << 3);
			lcd_moveto(0,0);
			lcd_stringout("timing..      ");
		}
		if(!stop1){
			state = STOP;
		}
	}
	else if(state == START){
		if(!start1){
			state = START;
		}
		if(!stop1){
			TCCR1B &= ~((1 << CS12)|(1 << CS10));
			PORTC &= ~(1 << 3);
			state = STOP;
			sensor = 1;
		}

	}
}

void timer2_init(void)
{
    TCCR2A |= (0b11 << WGM20);  // Fast PWM mode, modulus = 256
    TCCR2A |= (0b10 << COM2A0); // Turn PB3 on at 0x00 and off at OCR2A
    OCR2A = 23;                // Initial pulse duty cycle of 50%
    TCCR2B |= (0b111 << CS20);  // Prescaler = 1024 for 16ms period
}
void timer1_init(void)
{
	//Set mode for "Clear Timer on Compare"
    TCCR1B |= (1 << WGM12);
    //Enable timer Interupt
    TIMSK1 |= (1 << OCIE1A);
    //load 16-bit counter modulus into OCR1A
    OCR1A = 62500;
	OCR1B = 0;
    //set prescalar 1024
    //TCCR1B |= (1 << CS12)|(1 << CS10);
}
void timer0_init(void)
{
	//Set mode for "Clear Timer on Compare"
    TCCR0A |= (1 << WGM01);
    //Enable timer Interupt
    TIMSK0 |= (1 << OCIE0A);
    //load 8-bit counter modulus into OCR1A
    OCR0A = 79;
    //set prescalar 256
    //TCCR0B |= (1 << CS02);
}
void screenSetup(void)
{
	lcd_writecommand(1);
    lcd_moveto(0,0);
    lcd_stringout("Aline");
    char *a = "EE109", *b = "Project";
    char buf[15];
    snprintf(buf, 15, "%s %s", a, b);
    lcd_moveto(1,0);
    lcd_stringout(buf);
    _delay_ms(1500);
    lcd_writecommand(1);

	char buf1[15];
	snprintf(buf1, 15, "%3dms = %u.%1u", cnt, speed, r);
	lcd_moveto(0,0);
	lcd_stringout(buf1);

	lcd_moveto(1,0);
	count = eeprom_read_byte((void *) 100);
	char buf3[10];
	snprintf(buf3, 10, "MAX= %2d", count);
	lcd_stringout(buf3);
}