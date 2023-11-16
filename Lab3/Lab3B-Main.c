/*
    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"
#include "graphics.h"

uint32_t gcd(uint32_t u1, uint32_t u2) ;
uint32_t Factorial(uint32_t n) ;

#pragma GCC push_options
#pragma GCC optimize ("O0")

int32_t __attribute__((weak)) Return32Bits(void)
    {
    return (int32_t) +10 ;
    }

int64_t __attribute__((weak)) Return64Bits(void)
    {
    return (int64_t) -10 ;
    }

uint8_t __attribute__((weak)) Add8Bits(uint8_t x, uint8_t y)
    {
    return (uint8_t) (x + y) ;
    }

uint32_t __attribute__((weak)) FactSum32(uint32_t x, uint32_t y)
    {
    return (uint32_t) Factorial(x + y) ;
    }

uint32_t __attribute__((weak)) XPlusGCD(uint32_t x, uint32_t y, uint32_t z)
    {
    return (uint32_t) (x + gcd(y, z)) ;
    }

#pragma GCC pop_options

#define FONT_TITLE   Font12
#define FONT_RESULT  Font16

#define BOX_MARGIN   8
#define BOX_PADDING  4
#define BOX_WIDTH    (XPIXELS - 2*BOX_MARGIN)
#define BOX_HEIGHT   (2*BOX_PADDING + 11*FONT_RESULT.Height)
#define BOX_XLFT     BOX_MARGIN
#define BOX_YTOP     80
#define BOX_XTITLE   BOX_XLFT
#define BOX_YTITLE   (BOX_YTOP - FONT_TITLE.Height - BOX_PADDING)
#define BOX_XLABEL   (BOX_XLFT + BOX_PADDING)
#define BOX_XSTATUS  (BOX_XLFT + BOX_WIDTH - BOX_PADDING - 3*FONT_RESULT.Width)
#define BOX_YRESULTS (BOX_YTOP + BOX_PADDING)

#define XLFT_ERRORS  (BOX_XLFT + BOX_PADDING)
#define YTOP_ERRORS  (BOX_YTOP + BOX_HEIGHT + BOX_MARGIN)

#define COLOR_YES    COLOR_BLACK
#define COLOR_NO     COLOR_RED

#define  ARG_REGS    4   // R0-R3
#define  RTN_REGS    2   // R0,R1
#define  ALL_REGS   12   // R0-R11

#define  LSW(x)      ((uint32_t) (x & 0xFFFFFFFF))
#define  MSW(x)      ((uint32_t) (x >> 32))
#define  ENTRIES(a)  (sizeof(a)/sizeof(a[0]))

typedef enum {FALSE = 0, TRUE = 1} BOOL ;

typedef struct
    {
    char *          name ;
    void *          function ;
    int             regs_in ;
    uint32_t        input[ARG_REGS] ;
    int             regs_out ;
    uint32_t        output[RTN_REGS] ;
    } TEST ;

// Public fonts defined in run-time library
typedef struct
    {
    const uint8_t * table ;
    const uint16_t  Width ;
    const uint16_t  Height ;
    } sFONT ;
extern sFONT        Font8, Font12, Font16, Font20, Font24 ;
static sFONT        *font ;
static uint32_t     stack ;
static int          errors ;

static void         CallFunction(void *function, uint32_t before[], uint32_t after[]) ;
static int          CheckFunctionReturns(int y, TEST *test, uint32_t before[], uint32_t after[]) ;
static int          CheckPreserved(int y, uint32_t before[], uint32_t after[]) ;
static int          CheckReturnValue(int y, TEST *test, uint32_t after[]) ;
static int          CheckStackAlignment(int y, unsigned stack) ;
static void         DisplayNumberErrors(void) ;
static void         DisplayStatus(int y, char *label, BOOL ok) ;
static void         InitializeDisplay(TEST *test, int which, int total) ;
static void         InitializeRegisters(TEST *test, uint32_t before[]) ;
static void         LEDs(BOOL grn_on, BOOL red_on) ;
static void         SetFontSize(sFONT *pFont) ;

int main()
    {
    static TEST tests[] =
        {
        {"Return32Bits", (void *) Return32Bits, 0, {},               1, {10}},
        {"Return64Bits", (void *) Return64Bits, 0, {},               2, {LSW(-10LL), MSW(-10LL)}},
        {"Add8Bits",     (void *) Add8Bits,     2, {0xC5, 0x83},     1, {0x48}},
        {"FactSum32",    (void *) FactSum32,    2, {2,3},            1, {5*4*3*2*1}},
        {"XPlusGCD",     (void *) XPlusGCD,     3, {3,5*7,7*11},     1, {10}}
        } ;

    InitializeHardware(HEADER, "Lab 3B: Functions & Parameters") ;

    for (int t = 0;; t = (t + 1) % ENTRIES(tests))
        {
        static uint32_t before[ALL_REGS] __attribute__((aligned(8))) ;
        static uint32_t after[ALL_REGS]  __attribute__((aligned(8))) ;
        int y = BOX_YRESULTS ;
        TEST *test = &tests[t] ;

        InitializeDisplay(test, t, ENTRIES(tests)) ;
        InitializeRegisters(test, before) ;
        y = CheckFunctionReturns(y, test, before, after) ;
        y = CheckReturnValue(y, test, after) ;
        y = CheckPreserved(y, before, after) ;
        y = CheckStackAlignment(y, stack) ;
        DisplayNumberErrors() ;
        WaitForPushButton() ;
        }

    return 0 ;
    }

static void InitializeDisplay(TEST *test, int which, int total)
    {
    char bfr[100], call[100] ;
    uint32_t *reg ;

    SetForeground(COLOR_WHITE) ;
    ClearDisplay() ;
    SetForeground(COLOR_YELLOW) ;
    FillRect(BOX_XLFT, BOX_YTOP, BOX_WIDTH, BOX_HEIGHT) ;
    SetForeground(COLOR_BLACK) ;
    DrawRect(BOX_XLFT, BOX_YTOP, BOX_WIDTH, BOX_HEIGHT) ;

    SetBackground(COLOR_WHITE) ;
    sprintf(call, "%s(", test->name) ;
    reg = &test->input[0] ;
    for (int r = 0; r < test->regs_in; r++, reg++)
        {
        if (r != 0) strcat(call, ",") ;
        sprintf(call + strlen(call), "%d", (int) *reg) ;
        }
    strcat(call, ")") ;

    sprintf(bfr, "TEST %d of %d: %s", which + 1, total, call) ;
    SetFontSize(&FONT_TITLE) ;
    DisplayStringAt(BOX_XTITLE, BOX_YTITLE, (uint8_t *) bfr) ;
    }

static void InitializeRegisters(TEST *test, uint32_t before[])
    {
    uint32_t *reg = &before[0] ;
    uint32_t *inp = &test->input[0] ;
    for (int r = 0; r < ALL_REGS; r++, reg++)
        {
        if (r < test->regs_in) *reg = *inp++ ;
        else *reg = GetRandomNumber() ;
        }
    }

static int CheckFunctionReturns(int y, TEST *test, uint32_t before[], uint32_t after[])
    {
    SetFontSize(&FONT_RESULT) ;

    SetForeground(COLOR_BLACK) ;
    SetBackground(COLOR_YELLOW) ;
    DisplayStringAt(BOX_XLABEL, y, (uint8_t *) "Call returned:") ;

    errors = 1 ; // Assume function doesn't return
    DisplayNumberErrors() ;
    LEDs(FALSE, TRUE) ;
    SetForeground(COLOR_NO) ;
    SetBackground(COLOR_YELLOW) ;
    DisplayStringAt(BOX_XSTATUS, y, (uint8_t *) "NO") ;

    stack = 0 ;
    CallFunction(test->function, before, after) ;

    errors = 0 ; // function DID return!
    SetForeground(COLOR_YES) ;
    SetBackground(COLOR_YELLOW) ;
    DisplayStringAt(BOX_XSTATUS, y, (uint8_t *) "YES") ;

    return y + font->Height ;
    }

static int CheckReturnValue(int y, TEST *test, uint32_t after[])
    {
    char bfr[100] ;

    if (test->regs_out == 1)
        {
        BOOL ok = (after[0] == test->output[0]) ;
        sprintf(bfr, "Rtn Val %d OK:", (int) after[0]) ;
        DisplayStatus(y, bfr, ok) ;
        }
    else if (test->regs_out == 2)
        {
        int64_t result = *((int64_t *) &after[0]) ;
        BOOL ok = (result == *((int64_t *) test->output)) ;
        sprintf(bfr, "Rtn Val %lld OK:", (long long int) result) ;
        DisplayStatus(y, bfr, ok) ;
        }
    else
        {
        SetFontSize(&FONT_RESULT) ;
        SetForeground(COLOR_BLACK) ;
        DisplayStringAt(BOX_XLABEL, y, (uint8_t *) "Rtn Val OK:") ;
        SetForeground(COLOR_YES) ;
        DisplayStringAt(BOX_XSTATUS, y, (uint8_t *) "N/A") ;
        }

    return y + font->Height ;
    }

static int CheckPreserved(int y, uint32_t before[], uint32_t after[])
    {
    uint32_t *inp, *out ;
    char bfr[100] ;

    inp = &before[ARG_REGS] ; 
    out = &after[ARG_REGS] ;
    for (int r = ARG_REGS; r < ALL_REGS; r++, inp++, out++)
        {
        BOOL ok = (*out == *inp) ;
        sprintf(bfr, "R%02d preserved:", r) ;
        DisplayStatus(y, bfr, ok) ;
        y += font->Height ;
       }

    return y ;
    }

static int CheckStackAlignment(int y, unsigned stack)
    {
    BOOL ok = (stack & 0x7) == 0 ;
    DisplayStatus(y, "Stack aligned:", ok) ;
    return y + font->Height ;
    }

static void DisplayNumberErrors(void)
    {
    char bfr[100] ;

    sprintf(bfr, "Number of Errors: %d ", errors) ;
    SetFontSize(&FONT_RESULT) ;
    SetBackground(COLOR_WHITE) ;
    SetForeground(errors == 0 ? COLOR_YES : COLOR_NO) ;
    DisplayStringAt(XLFT_ERRORS, YTOP_ERRORS, (uint8_t *) bfr) ;
    }

static void DisplayStatus(int y, char *label, BOOL ok)
    {
    SetFontSize(&FONT_RESULT) ;
    SetBackground(COLOR_YELLOW) ;
    SetForeground(COLOR_BLACK) ;
    DisplayStringAt(BOX_XLABEL, y, (uint8_t *) label) ;
    SetForeground(ok ? COLOR_YES : COLOR_NO) ;
    DisplayStringAt(BOX_XSTATUS, y, (uint8_t *) (ok ? "YES" : "NO")) ;
    if (!ok) LEDs(TRUE, FALSE), errors++ ;
    }

uint32_t gcd(uint32_t u1, uint32_t u2)
    {
    uint32_t temp ;

    stack = (unsigned) &temp ;
    while (u1 != u2)
	    {
        if (u1 > u2)  u1 = u1 - u2 ;
        else          u2 = u2 - u1 ;
        }

    return u1 ;
    }

uint32_t Factorial(uint32_t n)
    {
    uint32_t result = 1 ;

    stack = (unsigned) &result ;
    while (n != 0)
        {
        result *= n ;
        n = n - 1 ;
        }

    return result ;
    }

static void SetFontSize(sFONT *Font)
    {
    extern void BSP_LCD_SetFont(sFONT *) ;
    BSP_LCD_SetFont(Font) ;
    font = Font ;
    }

static void LEDs(BOOL grn_on, BOOL red_on)
    {
    static uint32_t * const pGPIOG_MODER    = (uint32_t *) 0x40021800 ;
    static uint32_t * const pGPIOG_ODR      = (uint32_t *) 0x40021814 ;
    
    *pGPIOG_MODER |= (1 << 28) | (1 << 26) ;    // output mode
    *pGPIOG_ODR &= ~(3 << 13) ;                 // both off
    *pGPIOG_ODR |= (grn_on ? 1 : 0) << 13 ;
    *pGPIOG_ODR |= (red_on ? 1 : 0) << 14 ;
    }

#pragma GCC push_options
#pragma GCC optimize ("O1")
static void CallFunction(void *function, uint32_t before[], uint32_t after[]) 
    {
    __asm(
        "push    {r4-r12,lr}    \n\t"
        "mov     r12,r0         \n\t"             // r12 <-- function
        "push    {r2}           \n\t"             // save after
        "ldmia   r1,{r0-r11}    \n\t"             // regs <-- before
        "blx     r12            \n\t"             // call function
        "pop     {r12}          \n\t"             // r12 <-- after
        "stmia   r12,{r0-r11}   \n\t"             // regs --> after
        "pop     {r4-r12,pc}    \n\t"
        ) ;
    }
#pragma GCC pop_options

