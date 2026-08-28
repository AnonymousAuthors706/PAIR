#include "thread.h"
#include "mutex.h"
#include <stdint.h>
#include "hardware.h"
#include "syringe.h"

//#include "hardware.h"
void delayMicroseconds(unsigned int delay)
{
    volatile unsigned int j = 0;
    for (; j < delay; j++);
}

char getserialinput(uint8_t inputserialpointer, char * input)
{
	uint8_t maxinputpointer = 2;
	//char input[2] = "+\n";
    if (inputserialpointer < maxinputpointer)
    {
        return input[inputserialpointer];
    }
    return 0;
}
// INSTRUMENTER: entry
uint8_t syringe_port = 0;

void syringe_control(char * input, int * status)
{
	*status = 1;
    /* -- Global variables -- */
    // Input related variables
    volatile uint8_t inputserialpointer = -1;
    uint16_t inputStrLen = 0;
    char inputStr[10]; //input string storage

    // Bolus size
    uint16_t mLBolus =  5;

    // Steps per ml
    int ustepsPerML = (MICROSTEPS_PER_STEP * STEPS_PER_REVOLUTION * SYRINGE_BARREL_LENGTH_MM) / (SYRINGE_VOLUME_ML * THREADED_ROD_PITCH );
    
    //int ustepsPerML = 10;
    int inner = 0;
    int outer = 0;
    int steps = 0;
	P3OUT = 0;
	P1OUT = 0;
	int count = 0;
    while(outer < 1)
   {
	   char c = getserialinput(++inputserialpointer, input);
	   // hex to char reader
	   while (inner < 10)
	   {
		   if(c == '\n') // Custom EOF
		   {
			   break;
		   }
		   if(c == 0)
		   {
			   outer = 10;
			   break;
		   }
		   inputStr[inputStrLen++] = c;
		   c = getserialinput(++inputserialpointer, input);
		   inner += 1;
	   }
	   inputStr[inputStrLen++] = '\0';
	   steps = (mLBolus * ustepsPerML) >> SHIFT_FACTOR; 
	   if(inputStr[0] == '+' || inputStr[0] == '-')
	   {
		   for(int j=0; j<DIVIDE_FACTOR; j++){
				for(int i=0; i < steps; i++){
					delayMicroseconds(SPEED_MICROSECONDS_DELAY);
					delayMicroseconds(SPEED_MICROSECONDS_DELAY);
					// "print" to waveform window
					count++;
					P1OUT = (0xff00 & count) >> 8;
					P3OUT = (0x00ff & count);
				}
				thread_yield(); // yield after 1/4th
			}
		}
		inputStrLen = 0;
		outer += 1;
	}
	*status = 1;
	
	thread_zombify();
}