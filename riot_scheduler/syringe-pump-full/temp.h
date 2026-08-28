#ifndef TEMP_H
#define TEMP_H

#define TEMP_PIN			0x02
#define MAX_READINGS		80
#define MAX_READINGS_1_4th	20

void delay(unsigned int us);

void read_data();
void get_temperature();

#endif // COMMS_H
