;SVC Handler

	AREA |.text|, CODE, READONLY, ALIGN=2
	PRESERVE8
	THUMB
	EXPORT SVC_Handler
	EXPORT triggerSVC
	EXTERN  RunPt            ; currently running thread
	EXTERN  NextRunPt        ; point to next thread to run
	;OS kernel functions

	EXTERN OS_Id
	EXTERN OS_Kill
	EXTERN OS_Sleep
	EXTERN OS_Time
	EXTERN OS_AddThread


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
	BEQ OS_AddThread
	
Return 	POP {LR}				;RETURN TO PROCESS LINK
	STR R0,[SP] 				; Store return value
	BX LR 						; Return from exception
	;ADD STUFF?
	

	;RUNS THE OS FUNCTION
	STR R0, [SP]	;RETURN STORE RETURN VALUE FROM OS FUNCTION
	BX LR 			;RETURN TO THE USERPROCESS/OS_SUSPEND
	

;TRIGGER SVC CALL FROM OS_ASM SUSPEND USING "SVC #XXX", WHERE XXX IS THE OS ROUTINE TRAP NUMBER

triggerSVC
	SVC #2
	BX LR
	
	ALIGN
	END

;OS_SLEEP
;SVC #2
;BX LR
