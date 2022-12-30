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


void tx_send(char c)
{
	while ((UCSR0A & (1 << UDRE0)) == 0) { }
    UDR0 = c;
}

ISR(USART_RX_vect){
	char received = UDR0;
	if(received == '['){
		startData = 1;
		dataCNT = 0;
		dataValid = 0;
		for(int i = 0; i < 5; ++i)
		{
			charBuff[i] = '\0';
		}
	}
	else if(received == ']' && dataCNT > 0 && startData){
		dataValid = 1; 
	}
	else if((received <= '9' && received >= '0') && startData && dataCNT <= 4){
		charBuff[dataCNT] = received;
		dataCNT++;
	}
	else {
		startData = 0;
	}
}