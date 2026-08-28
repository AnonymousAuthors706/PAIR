#ifndef SYRINGE_H
#define SYRINGE_H

#define SYRINGE_VOLUME_ML 30.0
#define SYRINGE_BARREL_LENGTH_MM 8.0

#define THREADED_ROD_PITCH 1.25
#define STEPS_PER_REVOLUTION 4.0
#define MICROSTEPS_PER_STEP 16.0

#define SPEED_MICROSECONDS_DELAY 20 //longer delay = lower speed

#define DIVIDE_FACTOR 8
#define SHIFT_FACTOR 3  // log2 of divide factor

#define false  0
#define true   1

#define LED_OUT_PIN 0
/* -- Enums and constants -- */
//syringe movement direction
enum{PUSH,PULL};

void delayMicroseconds(unsigned int delay);
char getserialinput(uint8_t inputserialpointer, char * input);
int syringepump(char * input, int * status);

#endif // SYRINGE_H
