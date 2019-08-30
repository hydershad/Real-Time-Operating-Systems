//HYDER SHAD
//UT EID: hs25796
//Jacob Knight
//UT EID: Jak3692
//EE 445M Lab 1
//MW 6:30pm - 8pm
//2/11/2019

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "ADC.h"
#include "UART.h"
#include "interpreter.h"
#include "SDCFile.h"
#include "ff.h"
#include "loader.h"
#include "elf.h"

void menu(void);
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
void printFile(TCHAR *path);

char instring [64] = {0};				//UART input buffer
int delay = 0;
void lights(int i);
int samples = 0;
char* heap_variable [5]=  {0};
void * heap_variable_address [5] = {0};

static const ELFSymbol_t symtab[] = {
	{ "ST7735_Message", ST7735_Message }
};

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
		ST7735_Message(device, line, message[3]);		//prints message to LCD
		NLO();
		UART_OutString("Message written to screen ");
		UART_OutUDec(device);
		UART_OutString(" on line ");
		UART_OutUDec(line);
		UART_OutChar('\n');
		UART_OutChar(CR);
}


void TwoWord_Interpret(char * command){

	char* message [3];
	message[0] = command;
	int index = 1;
	if(UART_compare(message[0], "free")==1){
			for(int i = 0; command[i] != 0; i++){		//assigns every string separated by a space in the message array
					if(command[i] == ' '){
						message[index] = &command[i+1];
						index++;
						if(index > 2){
								UART_invalid_command();
							return;
						}
					}
				}
			int i = 0;
			for(i = 0; i<5; i++){
				if(UART_compare(message[1], heap_variable[i])==1){
				//	Heap_Free(heap_variable_address[i]);		//free variable
				//	heap_variable_address[i] = 0;		//clear out variable info
				//	heap_variable[i] = 0;				//clear out variable
					NLO(); 
					UART_OutString("Success!"); 
					return;
					
			}
			if(i==5){NLO(); UART_OutString("Error: Could not locate variable"); return;}
	}
	
	}
	else {UART_invalid_command(); return;}

}
void ThreeWord_Interpret(char* command){		//Changed to seq2 PE3
	char* message [3];
	message[0] = command;
	int index = 1;
	int srate = 0;
	
	if(UART_compare(message[0], "malloc")==1){
				for(int i = 0; command[i] != 0; i++){		//assigns every string separated by a space in the message array
					if(command[i] == ' '){
						message[index] = substring(&command[i+1]);
						index++;
						if(index > 3){
								UART_invalid_command();
							  return;
						}
					}
				}	
			int i = 0;
			for(i = 0; i<5; i++){
				if(heap_variable[i]== 0){
				//	heap_variable[i] = message[1][0];		//store the variable name in the variable array
					if(0 < command[2] < 513){
					//	heap_variable_address[i] = Heap_Malloc(message[2]);		//store allocated block starting address for the variable
						if(heap_variable_address[i] == 0){NLO(); UART_OutString("No room to allocate variable in heap");return;}	//if null, mallo failed
					}
					else{
						NLO();
						UART_OutString("Error: Invalid heap variable size");//too big or small to allocate
						return;
					}	
				}
			if(i == 5){
				NLO();
				UART_OutString("Error: All 5 heap variables are already in use");//no free variable space left
				return;
			}		
		}
	}	
	
	else if(UART_compare(message[0], "adcs") == 1){
		
	
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
			if((srate<10) || (srate>20)){
			UART_invalid_command();
			return;
		}
		samples = Array_to_number(message[2]);
		if((samples<1) || (samples>100)){
			UART_invalid_command();
			return;
		}
		srate = (80000000/srate);	
		DisableInterrupts();
		ADC0_InitTimer0ATriggerSeq2(0, srate);		//timer handler outputs, using ss2
		EnableInterrupts();
	}
	
	else{
			UART_invalid_command();
	}
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
			
			ThreeWord_Interpret(message);
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
				  ADC0_InitTimer0ATriggerSeq2(0, 0xFFF);
					EnableInterrupts();
				return;
			}
			
//				if(UART_compare(message, "format")){
//					simpleformat();					
//					return;
//			}
/*			if(UART_compare(message, "blue")){
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
	*/		
			//**************lab5 commands ELF loader
			
			if(UART_compare(message, "elfloader")){
						ELFEnv_t env = { symtab, 1 };
						if (!exec_elf("Proc.axf", &env)) {
						UART_OutString("Load Successful");
						}
				return;
			}
			
	if(UART_compare(message, "readfile")){
		for(int i = 0; i<64; i++){
			instring[i] = 0;
		}
		NLO();
		UART_OutString("Enter filename\n");
		UART_OutChar(CR);
		UART_InString(instring, 64);
		printFile(instring);
}		
			UART_invalid_command();
			return;
}
	if(wordCount == 2){
		TwoWord_Interpret(message);
		
	}	
}

void Interpret(void){							
	
  
	DisableInterrupts();
	UART_Init(3);
	ST7735_SetRotation(2);
	Output_Clear();						//clear LCD
	ST7735_DrawFastHLine(0, 80, 128, ST7735_RED);
	
	
	//Heap_Init();	//Heap initialization
//leds
//	SYSCTL_RCGC2_R |= 0x20;
//  delay = SYSCTL_RCGC2_R;// ready?
//  GPIO_PORTF_DIR_R |= 0x0E;    // 
//  GPIO_PORTF_AFSEL_R &= ~0xE; // disable alt func on PB2-0
//  GPIO_PORTF_DEN_R |= 0x0E;
//	//
	
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
	
//	UART_OutString("- 'blue/red/green/ledon/ledoff' :: turn on/off corresponding LED\n");
//	UART_OutChar(CR);
	
	UART_OutString("- 'format' :: Format SD card\n");
	UART_OutChar(CR);
	
	UART_OutString("- 'clear' :: reset LCD display\n");
	UART_OutChar(CR);
	
  UART_OutString("- 'malloc' :: allocate variable on heap\n");
	UART_OutChar(CR);
	
  UART_OutString("- 'free' :: free variable on heap\n");
	UART_OutChar(CR);
}
/*
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

*/
char* substring(char* message){
char* substring;	
	int i = 0;
	
	if(('0'<=(message[i])<='9') || ('A'<=(message[i])<='Z') || ('a'<=(message[i])<='z')){
		substring[i] = message[i];
		i++;
	}
	
	return substring;
	
}

void printFile(TCHAR *path){

	FIL File_Handle;
	FRESULT status;
	char data;
	UINT bytesRead;
	
	NLO();

	f_open(&File_Handle, path, FA_READ);
	do{
		status = f_read(&File_Handle, &data, 1, &bytesRead);
		if(bytesRead){
			UART_OutChar(data);
		}

	}while(status == FR_OK && bytesRead);

	f_close(&File_Handle);

}

