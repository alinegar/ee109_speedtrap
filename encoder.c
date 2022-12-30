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

#define MASK_A (1 << PD3)
#define MASK_B (1 << PD2)

void rotaryEncode_init(void)
{
	unsigned int read = PIND;
	a = read & MASK_A;
	b = read & MASK_B;

    if (!b && !a)
	old_state = 0;
    else if (!b && a)
	old_state = 1;
    else if (b && !a)
	old_state = 2;
    else
	old_state = 3;

    new_state = old_state;
}
ISR(PCINT2_vect){ //encoder interrupt
	unsigned int read = PIND;
	a = read & MASK_A;
	b = read & MASK_B;

	if (old_state == 0) {
		// Handle A and B inputs for state 0
		if(a){
			new_state = 1;
			count++;
		}
		else if(b){
			new_state = 2;
			count--;
		}
	}
	else if (old_state == 1) {
		if(b){
			new_state = 3;
			count++;
		}
		else if(!a){
			new_state = 0;
			count--;
		}
	}
	else if (old_state == 2) {
		if(!b){
			new_state = 0;
			count++;
		}
		else if(a){
			new_state = 3;
			count--;
		}
	}
	else {   // old_state = 3
		if(!b){
			new_state = 1;
			count--;
		}
		else if(!a){
			new_state = 2;
			count++;
		}
	}
	if(count > 99){
		count = 99;
	}
	if(count < 1){
		count = 1;
	}

	// If state changed, update the value of old_state,
	// and set a flag that the state has changed.
	if (new_state != old_state) {
		changed = 1;
		old_state = new_state;
	}
}