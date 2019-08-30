
	AREA |.text|, CODE, READONLY, ALIGN=2

	THUMB
	;EXPORT	OS
	EXTERN RunPt
	EXTERN priorityPts
	EXPORT StartOS
	EXPORT SysTick_Handler
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