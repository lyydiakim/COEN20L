// Lydia Kim, LAB 3
// Wednesday 2:15


; When you push {LR}, make sure you push {R4, LR}

; The reason that we need to push both R4 and LR is that we can't do just LR
; The ARM Procedure Call Standard requires the stack pointer be double word aligned when functions are called. Failure to observe this requirement can cause a program to behave incorrectly or crash.
; So you need to save two registers


        .syntax         unified
        .cpu            cortex-m4
        .text


// int32_t Return32Bits(void) ;
        .global         Return32Bits
        .thumb_func
        .align

Return32Bits:
	//Implementation
        //32 bit signed
        //Return +10 into R0
        LDR R0, = 10
        BX              LR

// int64_t Return64Bits(void) ;
        .global         Return64Bits
        .thumb_func
        .align
Return64Bits:
	//Implementation
        //64 bit signed, keep in mind 2's comp
        //Return -10

        //Twos comp of 10
        LDR R0, = -10// R0 = -10 ~~ R0 is 1010 which is 
        LDR R1, = -1//R1 = -1  ~~ R1 is all 1's which is -1 


        BX              LR



// uint8_t Add8Bits(uint8_t x, uint8_t y) ;
        .global         Add8Bits
        .thumb_func
        .align
Add8Bits:
	//Implementation
        //UXTB for leading zeroes 

        ADD R0,R0,R1
        UXTB R0, R0
   
        BX             LR


// uint32_t FactSum32(uint32_t x, uint32_t y) ;
        .global         FactSum32
        .thumb_func
        .align
FactSum32:
	//Implementation
        //Reference factorial from C Main
        PUSH {R4, LR}

        //add value of x and y
        ADD R0, R0, R1


        //calculate factorial value
        BL Factorial

        POP {R4, PC}


// uint32_t XPlusGCD(uint32_t x, uint32_t y, uint32_t z) ;
        .global         XPlusGCD
        .thumb_func
        .align
XPlusGCD:
	//Implementation
        //Reference GCD from C Main
        //perform stack alignment - using push and pop
        //x is reused, so preserve input parameter - copy it somewhere in case the registers value changes
        //content of r0 is added to register where input parameter is stored
       
        PUSH {R4, LR} 

        //save x into  a different register
        MOV R4,R0

        //move  y and z into R0 and R1
        MOV R0, R1 
        MOV R1, R2

        //GCD function call from main
        //return of gcd is in R0
        BL gcd 
        
        //add x to gcd (y  + z)
        ADD R0,R0,R4

        POP {R4, PC}

      

        .end


