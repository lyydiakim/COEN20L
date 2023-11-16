        .syntax     unified
        .cpu        cortex-m4
        .text

// void PutNibble(void *nibbles, uint32_t index, uint32_t nibble) ;

        .global     PutNibble
        .thumb_func
        .align


PutNibble:
	//Implementation
	PUSH	{R4,R5}
	MOV	R4,R0			// R4 <- nibbles
	MOV	R5,R1			// R5 <- (u32_t which)
	LSR	R5,R5,1			// which>>1==which/2
	ADD	R4,R4,R5		// Add which/2 + nibbles
	LDRB	R4,[R4]		//grab the u8 
	AND	R1,R1,1			// 0=even, 1=odd
	
	CMP	R1,0		
	ITE	EQ				// checking if equal
	
	//equal
	BFIEQ	R4,R2,0,4	//takes the first half , from r2 (value) width=0 to r4 lsb=4
	
	//not equal
	BFINE	R4,R2,4,4	//takes the second half 
	
	//else
	STRB	R4,[R0,R5]	//store offset R5
	
	POP	{R4,R5}
	BX	LR
// uint32_t GetNibble(void *nibbles, uint32_t index) ;

        .global     GetNibble
        .thumb_func
        .align



GetNibble:
	//Implementation
	PUSH	{R4,R5}
	MOV	R4,R0			// R4 <- nibbles
	MOV	R5,R1			// R5 <- which
	LSR	R5,R5,1			// which/2
	ADD	R4,R4,R5		// R4 = nibbles +which/2
	LDRB	R4,[R4]		// grab u8			
	AND	R1,R1,1			// 0=even, 1=odd


	CMP	R1,0		
	ITE	EQ				//checking if equal

	//equal
	UBFXEQ	R0,R4,0,4	//extends first half

	//not equal
	UBFXNE	R0,R4,4,4	//extends second half
	
	POP	{R4,R5}
	BX	LR
        .end
