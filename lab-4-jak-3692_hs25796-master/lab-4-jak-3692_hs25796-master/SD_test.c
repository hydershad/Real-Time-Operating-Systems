//sd_test
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "edisk.h"
#include "string.h"
#include "SD_test.h"
#include "OS.h"
#include "interpreter.h"
//linked list for new
//delete file needs to go through file blocks and add them to free blocks
//files need beginning and an end pointer, same for the free space organizer



int main(void){
Interpret();
	return 0;
}

int mil_secs = 0;
int block_read_speed(int num_blocks){
	int period = 80;  //1ms
	mil_secs = 0;
	  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 51, interrupt number 35
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER3A
	
	BYTE buff[512]={0};
	int i = 0;
	for(i=0;i<num_blocks;i++){
		eDisk_ReadBlock(buff, num_blocks); 
	}
	i = (num_blocks*512)/(mil_secs);
	return i;
	
}


int block_write_speed(int num_blocks){
	int period = 80;
	mil_secs = 0;
	SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
  TIMER3_CTL_R = 0x00000000;    // 1) disable TIMER3A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear TIMER3A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 51, interrupt number 35
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable TIMER3A
	
	BYTE buff[512]={0};

	eDisk_Write(0, buff, 1, num_blocks); 
	int i = 0;
	i = (num_blocks*512)/(mil_secs);
	return i;
	
}



void Timer3A_Handler(void){
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER3A timeout
 mil_secs = mil_secs +1;
}

