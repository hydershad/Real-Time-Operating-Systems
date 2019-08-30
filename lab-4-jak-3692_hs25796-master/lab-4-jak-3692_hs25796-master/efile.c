// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17

#include <string.h>
#include "edisk.h"
#include "UART2.h"
#include <stdio.h>

#define SUCCESS 0
#define FAIL 1
#define DIRECTORY_BYTE_SIZE 16
#define FILENAMEINDEX 3
#define TOTAL_FILE_BYTES_INDEX 2
#define NEXT_POINTER_INDEX 1
#define START_POINTER_INDEX 9
#define LAST_POINTER_INDEX 10
#define BUFFERSIZE 512
#define FILE_CURRENT_SIZE_INDEX 11
#define CURRENT_BLOCK_INDEX 0
unsigned char buffer[BUFFERSIZE];
unsigned char tempbuffer[BUFFERSIZE];
#define MAXBLOCKS 100                             
int directory_size_index = 0;

/*
DIRECTORY ENTRY FORMAT

0 First letter
1 Second Letter
...
7 Eighth letter
8 Null terminator
9 Start block
10 End block
11 File byte size pt 1
12 File byte size pt 2

*/

/*
BLOCK FORMAT
0 Current Block
1	Next Block
2	size
3
*/

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

void diskError(char* errtype, unsigned long n){
  //PF2 = 0x00;      // turn LED off to indicate error
  while(1){};
}

//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system
		DSTATUS result;
	  result = eDisk_Init(0);  // initialize disk
  if(result) diskError("eDisk_Init",result);
  return SUCCESS;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)



//DONE
int eFile_Format(void){ // erase disk, add format
	if(eDisk_ReadBlock(buffer,0))diskError("eDisk_ReadBlock",0);	//read from disk directory on block 0
	for(int i = DIRECTORY_BYTE_SIZE; i <BUFFERSIZE; i++){	//starts at second entry since first is free space entry
		buffer[i] = 0;
	}
	
	buffer[START_POINTER_INDEX] = 1;		//free space points to first block
	buffer[LAST_POINTER_INDEX] = MAXBLOCKS - 1;		//free space ends at last block
	if(eDisk_WriteBlock(buffer,0))diskError("eDisk_WriteBlock",0);	//write over old directory
	for(int i = 0; i <DIRECTORY_BYTE_SIZE; i++){	//clears buffer
		buffer[i] = 0;
	}
	for(int i = 2; i < MAXBLOCKS - 1; i++){	//makes all blocks point to next block
		buffer[NEXT_POINTER_INDEX] = i;
		buffer[CURRENT_BLOCK_INDEX] = i - 1;
		if(eDisk_WriteBlock(buffer,i-1))diskError("eDisk_WriteBlock",i-1);
	}
	buffer[NEXT_POINTER_INDEX] = 0;
	buffer[CURRENT_BLOCK_INDEX] = MAXBLOCKS - 1;
	if(eDisk_WriteBlock(buffer,MAXBLOCKS - 1))diskError("eDisk_WriteBlock",MAXBLOCKS - 1);	//last block points to null
  return SUCCESS;   // OK
}



//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)


//DONE
int eFile_Create( char name[]){  // create new file, make it empty 
int length = 0;
int index = DIRECTORY_BYTE_SIZE;	//pointing at the first character of the first file name after free
	for(length = 0; length < 8; length++){
		if(name[length] == 0){
			break;
		}
	}
	if((length == 0) || ((length == 8) && (name[length] != 0))){	//if there is no name or length is greater than 7 characters
		return FAIL;
	}
	
	if(eDisk_ReadBlock(buffer,0))diskError("eDisk_ReadBlock",0);	//reads directory from SD card
	for(index = index; index <BUFFERSIZE; index += DIRECTORY_BYTE_SIZE){

		if(UART_compare(&buffer[index], name)){		//if name already exists
			return FAIL;
		}
	}
	if(index > BUFFERSIZE){	//too many files in directory
		return FAIL;
	}
		for(index = DIRECTORY_BYTE_SIZE; index <BUFFERSIZE; index += DIRECTORY_BYTE_SIZE){		//checking for first available slot
		if(buffer[index] == 0){
			break;
		}
	}
	for(int i = 0; i < length; i++){	//add file name to directory
		buffer[index + i] = name[i];
	}
	if(length != 8){
		buffer[index + length] = 0;		//need to add null termination
	}
	
	
	//now we need to set block pointers
	
	
	index = index + 9; //looking at start of block
	buffer[index] = buffer[START_POINTER_INDEX];	//sets start block on new file
	buffer[index + 1] = buffer[index];	//sets end block on new file (same as start block)
	buffer[index + 2] = 0; 							//current size of the file
	
	if(eDisk_ReadBlock(tempbuffer,buffer[START_POINTER_INDEX]))diskError("eDisk_ReadBlock",0);	//reads in the first free block
	//TODO: Come Back
	buffer[START_POINTER_INDEX] = tempbuffer[NEXT_POINTER_INDEX];	//Set start of free to next of first available free
	tempbuffer[NEXT_POINTER_INDEX] = 0;		//set next of block to 0 since it is the only block used for this file
	tempbuffer[TOTAL_FILE_BYTES_INDEX] = 0;
	
	if(eDisk_WriteBlock(tempbuffer,buffer[index]))diskError("eDisk_WriteBlock",buffer[index]);		//writes block back
  if(eDisk_WriteBlock(buffer,0))diskError("eDisk_WriteBlock",0);	//writes directory back
	return SUCCESS;     
}

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)


//DONE
int eFile_WOpen(char name[]){      // open a file for writing
if(name[0] == 0){	//if no name is passed
	return FAIL;
}	
	if(eDisk_ReadBlock(buffer,0))diskError("eDisk_ReadBlock",0);	//load directory

	for(int i = DIRECTORY_BYTE_SIZE; i < BUFFERSIZE; i += DIRECTORY_BYTE_SIZE){		//start looking at the names
		if(UART_compare(&buffer[i], name)){
			if(eDisk_ReadBlock(buffer, buffer[i-1]))diskError("eDisk_ReadBlock",buffer[i-1]);	//loads last block of file name into ram
			directory_size_index = i + 11;		//points to byte size for specific file in directory
			return SUCCESS;
		}
	}
  return FAIL;   
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)


//TODO: COME BACK NOT FINISHED
int eFile_Write(char data){
	
	if(eDisk_ReadBlock(tempbuffer,0))diskError("eDisk_ReadBlock",0);		//open directory
	tempbuffer[directory_size_index] += 1;
	if(eDisk_WriteBlock(tempbuffer,0))diskError("eDisk_WriteBlock",0);	//write directory back to disk
	
	
	buffer[TOTAL_FILE_BYTES_INDEX] += 1;	//add one to the total size
	buffer[buffer[TOTAL_FILE_BYTES_INDEX] + 2] = data;		//places data at the end
	if(eDisk_WriteBlock(buffer,buffer[CURRENT_BLOCK_INDEX]))diskError("eDisk_WriteBlock",buffer[CURRENT_BLOCK_INDEX]);
	
	
if(buffer[TOTAL_FILE_BYTES_INDEX] < 509){	//still space left in the block
	return SUCCESS;
}



//not enough space in this block, so we have to go to free blocks and take one there

if(eDisk_ReadBlock(tempbuffer,0))diskError("eDisk_ReadBlock",0);		//get directory
if(tempbuffer[NEXT_POINTER_INDEX] == 0){	//no free space left
	return FAIL;
}

}


//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){ 

  return SUCCESS;     
}

//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
  
  return SUCCESS;     
}


//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){      // open a file for reading 

  return SUCCESS;     
}
 
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){       // get next byte 

  return SUCCESS; 
}

    
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing

  return SUCCESS;
}




//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: none
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
//int eFile_Directory(void(*fp)(char)){



//TODO: add file size
int eFile_Directory(char List[]){
	int listIndex = 0;
	if(eDisk_ReadBlock(buffer,0))diskError("eDisk_ReadBlock",0);
	for(int i = DIRECTORY_BYTE_SIZE; i < BUFFERSIZE; i += DIRECTORY_BYTE_SIZE){
		if(buffer[i] != 0){
			for(int k = i; buffer[k] != 0; k++){	//add characters of file name to list
				List[listIndex] = buffer[k];
				listIndex++;
			}
			List[listIndex] = ' ';
			listIndex++;
			
			
			//TODO: add file sizes here
			List[listIndex] = 'S';
			listIndex++;
			List[listIndex] = 'i';
			listIndex++;
			List[listIndex] = 'z';
			listIndex++;
			List[listIndex] = 'e';
			listIndex++;
			List[listIndex] = ':';
			listIndex++;
			List[listIndex] = ' ';
			listIndex++;
			List[listIndex] = buffer[i+11];
			listIndex++;
			
		}
	}
  return SUCCESS;
}

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)



//DONE
int eFile_Delete( char name[]){  // remove this file
	if(name[0] == 0){	//if no name is passed
	return FAIL;
}	
	if(eDisk_ReadBlock(buffer,0))diskError("eDisk_ReadBlock",0);	//load directory

	for(int i = DIRECTORY_BYTE_SIZE; i < BUFFERSIZE; i += DIRECTORY_BYTE_SIZE){		//start looking at the names
		if(UART_compare(&buffer[i], name)){
			
			if(eDisk_ReadBlock(tempbuffer, buffer[LAST_POINTER_INDEX]))diskError("eDisk_ReadBlock",buffer[LAST_POINTER_INDEX]);	//loads last free block into ram
				
				tempbuffer[NEXT_POINTER_INDEX] = buffer[i + 9];		//sets next pointer of free list to the list we are deleting
				
			if(eDisk_WriteBlock(tempbuffer, buffer[LAST_POINTER_INDEX]))diskError("eDisk_WriteBlock",buffer[LAST_POINTER_INDEX]);
				
				buffer[LAST_POINTER_INDEX] = buffer[i + 10];		//sets last free to last block in file being deleted
			
			for(int k = i; k < i + DIRECTORY_BYTE_SIZE; k++){
				
				buffer[k] = 0;		//clears out slot for file
			
			}
			
			if(eDisk_WriteBlock(buffer,0))diskError("eDisk_WriteBlock",0);
			
			return SUCCESS;
		}
	}

  return FAIL;    // restore directory back to flash
}

int StreamToFile=0;                // 0=UART, 1=stream to file

int eFile_RedirectToFile(char *name){
  eFile_Create(name);              // ignore error if file already exists
  if(eFile_WOpen(name)) return 1;  // cannot open file
  StreamToFile = 1;
  return 0;
}

int eFile_EndRedirectToFile(void){
  StreamToFile = 0;
  if(eFile_WClose()) return 1;    // cannot close file
  return 0;
}

int fputc (int ch, FILE *f) { 
  if(StreamToFile){
    if(eFile_Write(ch)){          // close file on error
       eFile_EndRedirectToFile(); // cannot write to file
       return 1;                  // failure
    }
    return 0; // success writing
  }

   // regular UART output
  UART_OutChar(ch);
  return 0; 
}

int fgetc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);            // echo
  return ch;
}
