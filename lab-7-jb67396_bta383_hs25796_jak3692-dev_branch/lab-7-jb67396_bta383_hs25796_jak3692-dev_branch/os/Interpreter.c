// Interpreter.c
// Runs on TM4C123
// Provides all functions needed for the OS to implement
// semaphore functionality.
// Beathan Andersen & John Ballock
// March 28, 2019

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "UART.h"
#include "ADC.h"
#include "OS.h"
#include "can0.h"
#include "elf.h"
#include "loader.h"
#include "ST7735.h"
#include "eFile.h"
#include "Time.h"
#include "Laser.h"

#define DIRECTORY_SIZE 7

extern Sema4Type LCDmutex;

// Struct that contains command name and the function it performs
typedef struct {
    const char *name;
    void (*function)(char*);
} command_t;

// Symbol table of OS functions
// Allows for dynamic linking
static const ELFSymbol_t symtab[] = { { "ST7735_Message", ST7735_Message } }; 

// List of functions that will be implemented
void adcGet(char* arg);
void canCommand(char* arg);
void kill(char* arg);
void LoadProgram(char* arg);
void moveCommand(char* arg);
void ping(char* arg);
void time(char* arg);
void getLaserRange(char* arg);

// Define all commands and the functions they point to
command_t commandList[DIRECTORY_SIZE] = {
	{"can", canCommand},
	{"kill", kill},
	{"ldproc", LoadProgram},	
	{"move", moveCommand},
	{"ping", ping},
	{"time", time},
	{"laserr", getLaserRange}
};

// Simple next line function
void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}

// Function to parse the inputted command
void parseCommand(char* string){
	int i = 0, argIdx = 0, cmdIdx = 0;
	char command[10] = {0,0,0,0,0,0,0,0,0,0};
	char arg[20] = {0,0,0,0,0,0,0,0,0,0};
	
	// Splitting inputted string into designate substrings
	while(string[i] != 0){
		if(string[i] == ' ' && argIdx == 0){
			argIdx++;
			cmdIdx = 0;
		}
		else {
			switch(argIdx){
				case 0:
					command[cmdIdx] = string[i];
					break;
				case 1:
					arg[cmdIdx] = string[i];
					break;
				default:
					UART_OutString("Shouldn't hit here");
					return;
			}
			cmdIdx++;
			if(cmdIdx > 9 && argIdx == 0){
				UART_OutString("Command too long");
				return;
			}
			else if(cmdIdx > 19){
				UART_OutString("Argument too long");
				return;
			}
		}
		i++;
	}
	// Search through the entire command directory
	// If we can't find the command display issue
	i = DIRECTORY_SIZE;
	while(i--) {
			if(!strcmp(command,commandList[i].name)) {
					commandList[i].function(arg);
					return;
			}
	}
	UART_OutString("No command found");
	return;
}

// Actual interpreter thread
void Interpreter(void){ 
	char string[20];
	while(1){
		OutCRLF();
    UART_OutString("interpreter> ");
    UART_InString(string,19);
    OutCRLF();
		parseCommand(string);
  }
}

void canCommand(char* arg){
	if(!strncmp(arg,"get", 3)){
		uint8_t message[5];
		message[4] = 0;
		if(CAN0_GetMailNonBlock(message)){
			UART_OutString("The latest CAN message was: ");
			UART_OutString((char *) message);
		}
		else{
			UART_OutString("Failed to acquire latest CAN message!");
		}
	}
	else if(!strncmp(arg,"send ", 5)){
		CAN0_SendData((uint8_t *) &arg[5]);
		UART_OutString("Sent message: ");
		UART_OutString(&arg[5]);
	}
	else if(!strncmp(arg,"fifo", 4)){
		UART_OutString("Messages in FIFO: ");
		UART_OutUDec(CAN0_CheckMail());
	}
	else{
		UART_OutString("Your argument was invalid: ");
		UART_OutString(arg);
	}
}

void kill(char* arg){
	UART_OutString("Killing Interpreter");
	OS_Kill();
}

void LoadProgram(char* arg) 
{ 
	ELFEnv_t env = { symtab, 1 };  			// name of symbol table and number of elements in table  
	OS_bWait(&LCDmutex);
	if (!exec_elf("Proc.axf", &env)) 
	{
		UART_OutString("Failed to load");
	}
	//else UART_OutString("Failed to load");
	else UART_OutString("Loaded");
	OS_bSignal(&LCDmutex);
} 

int motorSpeed = 30;
void moveCommand(char* arg){
	uint8_t message[4];
	if(!strncmp(arg,"speed ", 6)){
		char* speed = &arg[6];
		uint8_t intSpeed = 0, i = 0;
		while(speed[i] != 0){
			if(speed[i] > 57 || speed[i] < 48)
				return;
			intSpeed = (intSpeed * 10) + (speed[i] - '0');
			i++;
		}
		// Set motor speed to intSpeed
		message[3] = 1;
		message[2] = intSpeed;
		motorSpeed = intSpeed;
		CAN0_SendData(message);
		UART_OutString("Setting speed to: ");
		UART_OutUDec(intSpeed);
	}
	else if(!strncmp(arg,"left", 4)){
		// Set servo to left
		message[3] = 2;
		message[1] = 0;
		CAN0_SendData(message);
		UART_OutString("Turning left");
	}
	else if(!strncmp(arg,"right", 5)){
		// Set servo to right 
		message[3] = 2;
		message[1] = 180;
		CAN0_SendData(message);
		UART_OutString("Turning right");
	}
	else if(!strncmp(arg,"straight", 8)){
		// Set servo to center 
		message[3] = 2;
		message[1] = 90;
		CAN0_SendData(message);
		UART_OutString("Moving straight");
	}
	else{
		UART_OutString("Your argument was invalid: ");
		UART_OutString(arg);
	}
}

void ping(char* arg){
	UART_OutString("pong");
}

void time(char* arg){
	UART_OutString("The current system time is: ");
	UART_OutUDec(OS_Time());
}

void getLaserRange(char* arg)
{
	UART_OutString("Laser distance is: ");
	UART_OutUDec(getMeasure(0));
}

