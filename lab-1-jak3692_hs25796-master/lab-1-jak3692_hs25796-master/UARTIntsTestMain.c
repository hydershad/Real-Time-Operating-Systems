//HYDER SHAD
//UT EID: hs25796
//Jacob Knight
//UT EID: Jak3692
//EE 445M Lab 1
//MW 6:30pm - 8pm
//2/11/2019

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 5.11 Section 5.6, Program 3.10

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */



#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "PLL.h"
#include "tm4c123gh6pm.h"

#include "ST7735.h"
#include "ADC.h"
#include "UART.h"

void menu(void);
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
char instring [64] = {0};				//UART input buffer
int delay = 0;
void lights(int i);
int samples = 0;
int tron_active = 0;

void UART_invalid_command(void){
		NLO();
		UART_OutString("Invalid Command!\n");
	  UART_OutChar(CR);
}


int Array_to_number(char* text){		//converts the ascii number to an int
	int final = 0;
	int digit = 1;
	int num_Entries = 0;
	for( num_Entries	= 0; ((text[num_Entries]!= ' ') && (text[num_Entries] != 0)); num_Entries++){
		
	}
	for(int i = (num_Entries - 1); i >= 0; i--){
			final = final + (digit*(text[i]-48));
			digit*=10;
	}
	return final;
	
}

int UART_compare(char* first, char* second){		//checks if arrays have the same characters
	int size_First = 0;
	int size_Second = 0;
		for(int i = 0; ((first[i] != 0) && (first[i] != ' ')); i++){		//determines number of characters in first char array
				size_First++;
		}
		for(int i = 0; ((second[i] != 0) && (second[i] != ' ')); i++){	//determines number of characters in second array
				size_Second++;
		}
		if(size_First != size_Second){		//if sizes don't match then the words are not the same
				return 0;
		}
		for(int i = 0; i < size_First; i++){		//compares the individual characters in the char arrays
				if(first[i] != second[i]){
						return 0;
				}
		}
		return 1;		//if arrays match return true
}


void LCD_Interpret(char* command){		//interprets the call to print on LCD. Makes sure syntax is correct before displaying the message
	char* message [4];
	message[0] = command;
	int index = 1;
	if(UART_compare(message[0], "lcd") != 1){		//if first word is not 'lcd' then invalid command
			UART_invalid_command();
			return;
	}
		for(int i = 0; command[i] != 0; i++){		//assigns every string separated by a space in the message array
				if(command[i] == ' '){
						message[index] = &command[i+1];
						index++;
						if(index > 4){
								UART_invalid_command();
							return;
						}
				}
		}
		if(((message[1][0] != '1') && (message[1][0] != '0')) || message[1][1] != ' '){		//if the screen is'nt top or bottom them invalid
				UART_invalid_command();
			return;
		}
		int line = message[2][0] - 48;
		if(((line) > 3) || ((line) < 0) || message[2][1] != ' '){		//if line isnt 0-3 then invalid command
				UART_invalid_command();
		return;
	}
		int device = 0;
	  device = message[1][0] - 48;
		if(tron_active){
			DisableInterrupts();
			Output_Clear();
			ST7735_DrawFastHLine(0, 80, 128, ST7735_RED);
			tron_active = 0;
			EnableInterrupts();
		}
		ST7735_Message(device, line, message[3], 2);		//prints message to LCD
		NLO();
		UART_OutString("Message written to screen ");
		UART_OutUDec(device);
		UART_OutString(" on line ");
		UART_OutUDec(line);
		UART_OutChar('\n');
		UART_OutChar(CR);
}



void ADCS_Interpret(char* command){		//COME BACK NOT FINISHED
	char* message [3];
	message[0] = command;
	int index = 1;
	int srate = 0;
	if(UART_compare(message[0], "adcs") != 1){
			UART_invalid_command();
		return;
	}
	for(int i = 0; command[i] != 0; i++){		//assigns every string separated by a space in the message array
			if(command[i] == ' '){
					message[index] = &command[i+1];
					index++;
					if(index > 3){
							UART_invalid_command();
						return;
					}
			}
		}
	srate = Array_to_number(message[1]);
		if((srate<10) || (srate>100)){
		UART_invalid_command();
		return;
	}
	samples = Array_to_number(message[2]);
	if((samples<1) || (samples>20)){
		UART_invalid_command();
		return;
	}
	srate = (80000000/srate);	
	DisableInterrupts();
	ADC0_InitTimer0ATriggerSeq3(0, srate);
	EnableInterrupts();
}


void UART_Interpret(char* buff){		//takes the input char array from the user input and determines what action to take
	int wordCount = 0;
	char* message = buff;
	for(int i = 0; buff[i] != 0; i++){		//keeps track of the number of words in command
			if(buff[i] == ' '){
					wordCount++;
			}
	}
	wordCount++;
	
	if(wordCount == 4){		//possible LCD call
			LCD_Interpret(message);
		return;
	}
	if(wordCount == 3){		//possible ADCS call
			ADCS_Interpret(message);
		return;
	}

	if(wordCount == 1){		//possible help or ADC call
			if(UART_compare(message, "clear")){
				Output_Clear();
				ST7735_DrawFastHLine(0, 80, 128, ST7735_RED);
				NLO();
				UART_OutString("Display Cleared!\n");
				UART_OutChar(CR);
				return;
			}
			if(UART_compare(message, "help")){
					menu();
					return;
			}
			if(UART_compare(message, "adci")){
					samples = 1;
				DisableInterrupts();
				  ADC0_InitTimer0ATriggerSeq3(0, 0xFFF);
					EnableInterrupts();
				return;
			}
			if(UART_compare(message, "blue")){
				lights(1);
				return;
			}
			if(UART_compare(message, "red")){
				lights(2);
				return;
			}
			if(UART_compare(message, "green")){
				lights(3);
				return;
			}
			if(UART_compare(message, "ledon")){
				lights(4);
				return;
			}
			if(UART_compare(message, "ledoff")){
				lights(5);
				return;
			}
			if(UART_compare(message, "tron")){
				DisableInterrupts();
				tron_active = 1;
				Output_Clear();
				ST7735_SetRotation(3);
				ST7735_DrawBitmap(0,119, tronboot, 160, 120); //STARTUP SCREENS
				ST7735_SetRotation(2);
				EnableInterrupts();
				NLO();
				UART_OutString("Tron displayed!\n");
				UART_OutChar(CR);
				return;
			}
			UART_invalid_command();
			return;
	}
	if(wordCount == 2){
	
		UART_invalid_command();
	}	
}

int main(void){							
	
  PLL_Init(Bus80MHz);       // set system clock to 50 MHz
	DisableInterrupts();
	UART_Init(3);

	//leds
	SYSCTL_RCGC2_R |= 0x20;
  delay = SYSCTL_RCGC2_R;// ready?
  GPIO_PORTF_DIR_R |= 0x0E;    // 
  GPIO_PORTF_AFSEL_R &= ~0x0E; // disable alt func on PB2-0
  GPIO_PORTF_DEN_R |= 0x0E;
	//
	ST7735_InitR(INITR_REDTAB);
	ST7735_SetRotation(2);
	Output_Clear();						//clear LCD
	ST7735_DrawFastHLine(0, 80, 128, ST7735_RED);
	EnableInterrupts();

	menu();
	while(1){
		UART_InString(instring, 64);
		UART_Interpret(instring);
		for(int i = 0; i<64; i++){
			instring[i] = 0;
		}
	}
}

void menu (void){
	NLO();
  UART_OutString("<<< EE445M Lab 1 Help Menu >>>\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'help' :: prints command list and parameters\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'adci' :: read single value from ADC input on PE3\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'adcs [SPACE] * [SPACE] **' :: sample ADC on PE3 @ * frequency (10 - 100), repeat ** times (1 - 20)\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'lcd [SPACE] * [SPACE] ** [SPACE] ***' :: screen * (0-1), line ** (0-3), print ***\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'blue/red/green/ledon/ledoff' :: turn on/off corresponding LED\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'tron' :: draw bitmap image of Tron videogame project\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'clear' :: reset LCD display\n");
	UART_OutChar(CR);
}

void lights(int i){
	switch(i){
		
		case 1:	//blue
			GPIO_PORTF_DATA_R ^= 0x04;
			NLO();
			if((GPIO_PORTF_DATA_R & 0x04)== 0x04){
				UART_OutString("Blue LED = ON\n");
				UART_OutChar(CR);
			}
			else{
				UART_OutString("Blue LED = OFF\n");
				UART_OutChar(CR);
			}
		break;
			
		case 2:	//red
			GPIO_PORTF_DATA_R ^= 0x02; 
			NLO();
			if((GPIO_PORTF_DATA_R & 0x02)== 0x02){
				UART_OutString("Red LED = ON\n");
				UART_OutChar(CR);
			}
			else{
				UART_OutString("Red LED = OFF\n");
				UART_OutChar(CR);
			}
		break;
	
		case 3:	//green
			GPIO_PORTF_DATA_R ^= 0x08;
			NLO();
			if((GPIO_PORTF_DATA_R & 0x08)== 0x08){
				UART_OutString("Green LED = ON\n");
				UART_OutChar(CR);
			}
			else{
				UART_OutString("Green LED = OFF\n");
				UART_OutChar(CR);
			}		
		break;
		
		case 4:
			GPIO_PORTF_DATA_R = 0x0E;
			NLO();
			UART_OutString("All LEDs = ON\n");
			UART_OutChar(CR);
		break;
		
		case 5:
			GPIO_PORTF_DATA_R = 0x00;
			NLO();
			UART_OutString("All LEDs = OFF\n");
			UART_OutChar(CR);
		break;
		default:
			break;
	}
}

