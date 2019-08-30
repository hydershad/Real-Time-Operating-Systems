
	AREA |.text|, CODE, READONLY, ALIGN=2
	PRESERVE8
	THUMB
	;EXPORT	OS
	EXTERN RunPt
	EXTERN priorityPts
	EXTERN NextRunPt        ; point to next thread to run
	EXPORT StartOS
	EXPORT SysTick_Handler
	EXPORT SVC_Handler
	
	EXTERN OS_Id
	EXTERN OS_Kill
	EXTERN OS_Sleep
	EXTERN OS_Time
	EXTERN OS_AddThread1

	;EXPORT OS_Wait
	;EXPORT OS_Signal

StartOS LDR R0, =RunPt
	LDR R2, [R0]
	LDR SP, [R2]
	POP {R4-R11}
	POP {R0-R3}
	POP {R12}
	POP {LR}
	POP {LR}
	POP {R1}
	CPSIE I
	BX LR
	
SysTick_Handler 
	CPSID I
	PUSH {R4-R11}
	LDR R0, =RunPt
	LDR R1, [R0]
	STR SP, [R1]
	LDR R2, [R1, #20]	;loads priority into R2
	MOV R3, #-1
	LDR R0, =priorityPts
	ADD R0, R0, #-4;	need to subtract 4 to loop correctly
	
checkAgain 
	ADD R3, R3, #1;	assumes there will always be a thread running
	ADD R0, R0, #4
	LDR R1, [R0];	offset of 4 times the index because of pointer size
	CMP R1, #0
	BEQ checkAgain	;if pointer is null we check next priority list
	LDR R0, =RunPt
	;LDR R1, [R1]
	;LDR R3, [R1, #20] ;loads priority of whatever priority thread we are looking at
	CMP R3, R2
	BNE skip	;if they are equal priority we treat it like normal
	LDR R1, [R0]	;same priority normal context switch
	LDR R1, [R1, #4]
skip
	STR R1, [R0]	;higher priority ready to go
	LDR SP, [R1]
	POP {R4-R11}
	CPSIE I
	BX LR


SVC_Handler 
	LDR R12, [SP, #24]; RETURN ADDRESS
	LDRH R12, [R12, #-2]; SVC INSTRUCTION 2 BYTES
	BIC R12, #0xFF00; EXTRACT TRAP NUMBER IN R12
	LDM SP, {R0-R3}; GET PARAMETERS
	
	PUSH {LR}

	LDR LR, = Return			;store link to come back to end of svc handler
	CMP R12, #0				
	BEQ OS_Id					;case statement based on svc call argument
	CMP R12, #1
	BEQ OS_Kill
	CMP R12, #2
	BEQ OS_Sleep
	CMP R12, #3
	BEQ OS_Time
	CMP R12, #4
	BEQ OS_AddThread1
	
Return 	POP {LR}				;RETURN TO PROCESS LINK
	STR R0,[SP] 				; Store return value in R0 for process to view
	BX LR 						; Return from exception
	;ADD STUFF?
	

	;RUNS THE OS FUNCTION
	STR R0, [SP]	;RETURN STORE RETURN VALUE FROM OS FUNCTION
	BX LR 			;RETURN TO THE USERPROCESS/OS_SUSPEND
	

	
	
;OS_Wait ;R0 points to counter
	;LDREX R1, [R0] ; counter 
	;SUBS R1, #1 ; counter -1, 
	;ITT PL ; ok if >= 0 
	;STREXPL R2,R1,[R0] ; try update 
	;CMPPL R2, #0 ; succeed? 
	;BNE OS_Wait ; no, try again 
	;BX LR 
	
			
		
;OS_Signal ; R0 points to counter 
	;LDREX R1, [R0] ; counter 
	;ADD R1, #1 ; counter + 1 
	;STREX R2,R1,[R0] ; try update 
	;CMP R2, #0 ; succeed? 
	;BNE OS_Signal ;no, try again 
	;BX LR


	ALIGN
	END