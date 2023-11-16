        .syntax     unified
        .cpu        cortex-m4
        .text

// int32_t MxPlusB(int32_t x, int32_t mtop, int32_t mbtm, int32_t b) ;

// #define ASR31(x) ((x < 0) ? -1 : 0)                          // Same as arithmetic shift right by 31 bits    
// rounding = ((((ASR31(dvnd*dvsr)) * dvsr) << 1) + dvsr) / 2 ; // rounding (where dvnd = x*mtop, dvsr = mbtm)
// quotient = (dvnd + rounding) / dvsr ;                        // quotient = (x*mtop + rounding) / mbtm

/* ------------------------------------------------------------------------------------------------------------------- */
// Note: The purpose of ASR31(dvnd*dvsr) is to produce a result that will be -1 if dvnd/dvsr is negative, else 0. 
// Multiplying by dvsr and shifting left once produces -2*dvsr if dvnd < 0, else 0. To this we add dvsr, which yields
// -dvsr if dvnd < 0, and +dvsr if dvnd >= 0. Finally, we divide by 2 to produce a rounding that has the sign of
// dvnd and magnitude |dvsr/2|. 
//
// The 2nd product (dvnd*dvsr) below may overflow if MUL is used, generating a positive result when it should be negative, 
// or vice-versa. The solution below avoids this problem by using the most-significant half of a 64-bit product that  
// cannot overflow. There are two alternatives: (1) Use SMMUL R0,R1,R2 (covered in Chapter 8) that computes only the 
// most-signficant half of a 64-bit product, or (2) use XOR R0,R1,R2 (covered in Chapter 7).

//logical shift right not arithmetic
/* ------------------------------------------------------------------------------------------------------------------- */

        .global     MxPlusB
        .thumb_func

        .align
MxPlusB:
        MULS.N R1,R0,R1                //R1 = DVND 
        SMULL R12, R0, R1, R2
        ASRS.N R0, R0, 31                 // R0 = (dvnd*dvsr) >> 31s
        MULS.N R0,R0,R2            // R0 = (dvnd*dvsr) >> 31) * dvsr

        ADD R0, R2, R0, LSL 1           // R0 = ((dvnd*dvsr) >> 31) * dvsr) << 1) + dvsr
        ADD R0, R0, R0, LSL 31          // Add 1 if R0 is negative so ASR divides by 2 correctly
        

        ASR R0, R0, 1                   // R0 = rounding = (((dvnd*dvsr) >> 31) * dvsr) << 1) + dvsr) / 2
        ADDS.N R0,R0,R1                      // R0 = dvnd + rounding
        SDIV R0, R0, R2     // R0 = (dvnd + rounding) / dvsr
        ADDS.N R0, R0, R3      // R0 = (dvnd + rounding) / dvsr + b

        BX   LR

        .end

