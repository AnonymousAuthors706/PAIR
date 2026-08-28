#include <stdint.h>
#include "hardware.h"
#include "thread.h"
#include "temp.h"

// Temperature sensor output data
int temp;
uint8_t data[5] = {0,0,0,0,0};
uint8_t valid_reading = 0;

void delay(unsigned int us){
	int i;
	for(i=0; i<us; i++);
}

void read_data(){
	uint8_t counter = 0;
  	uint16_t j = 0, i;

  	/// pull signal high & delay
  	P2OUT |= TEMP_PIN;
  	thread_yield();

  	/// pull signal low for 20us
  	P2OUT &= ~TEMP_PIN;
  	thread_yield();

  	/// pull signal high for 40us
  	P2OUT &= ~TEMP_PIN;
  	thread_yield();

  	//read timings
	int outer;
	for(outer = 0; outer < 4; outer++){
		for(i=0; i<MAX_READINGS_1_4th; i++){
			counter += (P2IN & TEMP_PIN);

			// ignore first 3 transitions
			if ((i >= 4) && ( (i & 0x01) == 0x00)) {
				// shove each bit into the storage bytes
				data[j >> 3] <<= 1;
				if (counter > 6)
				data[j >> 3] |= 1;
				j++;
			}
		}
		thread_yield();
	}

	// check we read 40 bits and that the checksum matches
	if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ) {
		valid_reading = 1;
	} else {
		valid_reading = 0;
	}
}

void get_temperature(){
	read_data();

	uint16_t t = data[2];
	t |= (data[3] << 8);
	thread_zombify();
}