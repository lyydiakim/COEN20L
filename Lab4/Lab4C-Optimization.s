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

// ----------------------------------------------------------
// unsigned HalfWordAccess(int16_t *src) ;
// ----------------------------------------------------------

        .global     HalfWordAccess
        .thumb_func
        .align
HalfWordAccess:
        .rept       100
        LDRH        R1,[R0]
        .endr
        BX          LR

// ----------------------------------------------------------
// unsigned FullWordAccess(int32_t *src) ;
// ----------------------------------------------------------

        .global     FullWordAccess
        .thumb_func
        .align
FullWordAccess:
        .rept       100
        LDR         R1,[R0]
        .endr
        BX          LR

// ----------------------------------------------------------
// unsigned NoAddressDependency(uint32_t *src) ;
// ----------------------------------------------------------

        .global     NoAddressDependency
        .thumb_func
        .align
NoAddressDependency:
        .rept       100
        LDR         R1,[R0]
        LDR         R2,[R0]
        .endr
        BX          LR

// ----------------------------------------------------------
// unsigned AddressDependency(uint32_t *src) ;
// ----------------------------------------------------------

        .global     AddressDependency
        .thumb_func
        .align
AddressDependency:
        .rept       100
        LDR         R1,[R0]
        LDR         R0,[R1]
        .endr
        BX          LR

// ----------------------------------------------------------
// unsigned NoDataDependency(float f1) ;
// ----------------------------------------------------------

        .global     NoDataDependency
        .thumb_func
        .align
NoDataDependency:
        .rept       100
        VADD.F32    S1,S0,S0
        VADD.F32    S2,S0,S0
        .endr
        VMOV        S1,S0
        BX          LR

// ----------------------------------------------------------
// unsigned DataDependency(float f1) ;
// ----------------------------------------------------------

        .global     DataDependency
        .thumb_func
        .align
DataDependency:
        .rept       100
        VADD.F32    S1,S0,S0
        VADD.F32    S0,S1,S1
        .endr
        VMOV        S1,S0
        BX          LR

// ----------------------------------------------------------
// void VDIVOverlap(float dividend, float divisor) ;
// ----------------------------------------------------------

        .global     VDIVOverlap
        .thumb_func
        .align
VDIVOverlap:
        VDIV.F32    S0,S1,S2
        .rept       1
        NOP
        .endr
        VMOV        R0,S1
        BX          LR

        .end
