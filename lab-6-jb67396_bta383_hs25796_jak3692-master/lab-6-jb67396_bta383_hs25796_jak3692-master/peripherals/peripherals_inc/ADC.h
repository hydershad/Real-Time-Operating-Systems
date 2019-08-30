// ADC.h
// Runs on LM4F120/TM4C123

#ifndef __ADC_H__
#define __ADC_H__ 1

#include "../../inc/tm4c123gh6pm.h"

//------------ADC_SetSampFreq------------
// Input: fs - the frequency used when sampling
// Output: 1 if successful 0 if failed
void ADC_SetSampFreq(uint32_t fs); 

//------------ADC_Collect------------
// Input: channelNum - the ADC channel to open and use
//				fs - the frequency of sampling
//				buffer[] - a buffer to put the value into
//				mberOfSamples - the number of samples to store in the buffer[]
// Output: 1 if successful 0 if failed
//int ADC_Collect(uint32_t channelNum, uint32_t fs, uint16_t buffer[], uint32_t mberOfSamples);
int ADC_Seq3Collect(uint32_t channelNum, uint32_t fs, void(*task)(unsigned long));
int ADC_Seq2Collect(uint32_t channelNum, uint32_t fs, void(*task)(unsigned long));

#endif

