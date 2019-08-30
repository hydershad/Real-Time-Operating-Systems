// GP2Y0A21YK.c
// Runs on TM4C123
// GP2Y0A21YK driver

#define FREQUENCY 10000
#define NUMVALUES 4

#include "../os_inc/ADC.h"
#include "GP2Y0A21YK.h"

unsigned long IR_value3, IR_value2;

int IR_getDistance1(void)
{
	// TODO calibrate here
	return 0;
}

int IR_getDistance2(void)
{
	// TODO calibrate here
	return 0;
}

void IR_Sensor1(unsigned long value)
{
	IR_value3 = value;
}

void IR_Sensor2(unsigned long value)
{
	IR_value2 = value;
}

int GP2Y0A21YK_Init (void) {
	ADC_Seq3Collect(0, FREQUENCY, IR_Sensor1);
	ADC_Seq2Collect(1, FREQUENCY, IR_Sensor2);
	
	return 1;
}

