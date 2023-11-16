/*
    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/
        .syntax     unified
        .cpu        cortex-m4
        .text

// uint32_t Mul32X10(uint32_t multiplicand) ;

        .global     Mul32X10
        .thumb_func
        .align
Mul32X10:           // R0 = multiplicand

        LSL     R1, R0, 3      // r1 = r0 * 8 ( 2^3 = 8 )
        ADD     R0, R1, R0, LSL 1      // r0 = r1 + r0, lsl 1 
       

        BX          LR

// uint32_t Mul64X10(uint32_t multiplicand) ;

        .global     Mul64X10
        .thumb_func
        .align

Mul64X10:
        LSLS.N  R3,R1,2          // R3 = R1 * 4
        ORR     R3,R3,R0,LSR 30  // R3 = R3 * 4 + R0 lsr 30
        LSLS.N  R2, R0, 2        // R2 = R0 * 4

        ADDS.N  R2,R2, R0        // R2 = r2 + r0
        ADCS.N  R3, R3, R1        // add carry to upper half, r3 = r3+r1+carry
        LSLS.N  R0, R2, 1        // R0 = R2 * 2 
        ADC R1, R3, R3            // ADD FINAL CARRY, r1 = r3 + carry
        BX LR                  



// uint32_t Div32X10(uint32_t dividend) ;

        .global     Div32X10
        .thumb_func
        .align
Div32X10:           // R0 = dividend
        LDR         R1,=3435973837      //load 3435973837  in to R1
        UMULL       R2,R1,R1,R0         // multiply unsinged int, r2.r1 = r1.r0
        LSRS.N      R0,R1,3             // r0 =  r1 / 3
        BX          LR                  // return

        .end


