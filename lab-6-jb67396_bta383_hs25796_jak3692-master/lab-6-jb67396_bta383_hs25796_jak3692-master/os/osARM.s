        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8
		
		IMPORT Scheduler
		IMPORT ThreadProfiler
		IMPORT getProcessData
		IMPORT OS_DisTime
		IMPORT OS_EnTime
		IMPORT OS_Id
        IMPORT OS_Kill
		IMPORT OS_Sleep
		IMPORT OS_Time
		IMPORT OS_AddThread
		EXTERN  runPt            ; currently running thread
        EXPORT  OS_DisableInterrupts
        EXPORT  OS_EnableInterrupts
        EXPORT  StartOS
		EXPORT  SysTick_Handler
		EXPORT  SVC_Handler
			
;GPIO_PORTF1   EQU 0x40025008

; This disable/enable interrupts calls a function
; that times disabled length for benchmarking
OS_DisableInterrupts
        CPSID   I
		PUSH {R0, LR}
		BL OS_DisTime
		POP {R0, LR}
        BX      LR

OS_EnableInterrupts
        CPSIE   I
		PUSH {R0, LR}
		BL OS_EnTime
		POP {R0, LR}
        BX      LR
		
; This will start the first thread running
StartOS
	LDR R0, =runPt		; load the runPt loc
	LDR R1, [R0]		; load the runPt val
	LDR SP, [R1]		; load the savedSP into real SP
	POP {R4-R11}		; load initialized stack values
	POP {R0-R3}			; ""
	POP {R12}			; ""
	ADD SP, SP, #4		; set stack PT to skip value
	POP {LR}			; load the LR, should point to OS_Kill
	ADD SP, SP, #4		; skip another value
	CPSIE I				; enable interrupts and go!
	BX LR
	
; This function is called when a threads time
; slice is over
SysTick_Handler
	CPSID I
	;LDR R1, =GPIO_PORTF1            ; R1 = GPIO_PORTF1 (pointer)
    ;LDR R0, [R1]                    ; R0 = [R1] (previous value)
    ;EOR R0, R0, #0x02               ; flip bit 1
    ;STR R0, [R1]                    ; affect just PN1
	PUSH {R4-R11}					; save the threads values
	LDR R0, =runPt					; load the runPt
	LDR R1, [R0]
	STR SP, [R1]					; save SP
	PUSH {R0, LR}
	LDR R0, [R1, #4]
	BL ThreadProfiler
	POP {R0, LR}
	PUSH {R0, LR}
	BL Scheduler					; call the scheduler to find the next thread
	POP {R0, LR}					; get new threads registers set
	LDR R1, [R0]
	LDR SP, [R1]					; new threads SP
	POP {R4-R11}					; new threads saved values
	;LDR R1, =GPIO_PORTF1            ; R1 = GPIO_PORTN1 (pointer)
    ;LDR R0, [R1]                    ; R0 = [R1] (previous value)
    ;EOR R0, R0, #0x02               ; flip bit 1
    ;STR R0, [R1]                    ; affect just PN1
	PUSH{R0,LR}
	BL getProcessData
	MOV R9, R0
	POP{R0,LR}
	CPSIE I
	BX LR
	
SVC_Handler
	LDR  R12,[SP,#24]   ; Return address
	LDRH R12,[R12,#-2]  ; SVC instruction is 2 bytes
	BIC  R12,#0xFF00    ; Extract ID in R12
	LDM  SP,{R0-R3}     ; Get any parameters...
	
	PUSH {R7,R8}	;Save R8 (pushing R7 to keep alignment or something)
	CMP R12, #0
	IT EQ 	;IF R12 = 0 then do OS_Id
	LDREQ R8, =OS_Id;
	
	CMP R12, #1
	IT EQ 	;IF R12 = 1 then do OS_Kill
	LDREQ R8, =OS_Kill;
	
	CMP R12, #2
	IT EQ 	;IF R12 = 2 then do OS_Sleep
	LDREQ R8, =OS_Sleep;
	
	CMP R12, #3
	IT EQ 	;IF R12 = 3 then do OS_Time
	LDREQ R8, =OS_Time;
	
	CMP R12, #4
	IT EQ 	;IF R12 = 4 then do OS_AddThread
	LDREQ R8, =OS_AddThread;

	PUSH {R5, LR}	; Pushing R5 to keep alignment or something
	BLX R8	
	POP {R5, LR}
	
	POP {R7,R8}	;Retrieve R8
	
	STR  R0,[SP]        ; Store return value
	BX   LR             ; Return from exception

	ALIGN
	END
		