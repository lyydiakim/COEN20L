/*
    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "library.h"
#include "graphics.h"
#include "touch.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

uint32_t __attribute__((weak)) Mul32X10(uint32_t multiplicand)
    {
    return 10 * multiplicand ;
    }

uint64_t __attribute__((weak)) Mul64X10(uint64_t multiplicand)
    {
    return 10 * multiplicand ;
    }

uint32_t __attribute__((weak)) Div32X10(uint32_t dividend)
    {
    return dividend / 10 ;
    }

#pragma GCC pop_options

typedef enum {FALSE = 0, TRUE = 1} BOOL ;

typedef int             COLOR ;

typedef struct
    {
    COLOR               color ;
    uint32_t            value ;
    } CODE ;

typedef struct
    {
    COLOR               *color ;
    CODE                *code ;
    int                 codes ;
    int                 xpos ;
    int                 ypos ;
    int                 ndex ;
    } BAND ;

// Public fonts defined in run-time library
typedef struct
    {
    const uint8_t *     table ;
    const uint16_t      Width ;
    const uint16_t      Height ;
    } sFONT ;

extern sFONT Font8, Font12, Font16, Font20, Font24 ;

#define FONT_BAND       Font16
#define FONT_EXP        Font12
#define FONT_VAL        Font16
#define FONT_ERR        Font16

#define ENTRIES(a)      (sizeof(a)/sizeof(a[0]))

#define COLOR_VIOLET    COLOR_MAGENTA
#define COLOR_GOLD      0xFFFFD700
#define COLOR_SILVER    0xFFE6E6E6
#define COLOR_NONE      COLOR_BODY

#define COLOR_BODY      0xFFFAF8C4
#define COLOR_WIRE      COLOR_DARKGRAY
#define COLOR_TEXT      COLOR_BLACK
#define COLOR_BKGD      COLOR_YELLOW
#define COLOR_BRDR      COLOR_RED

#define BAND_WIDTH      12
#define BAND_GAP        6
#define BODY_LENGTH     (6*(BAND_WIDTH + BAND_GAP))
#define BODY_DIAMETER   30
#define BODY_XLFT       ((XPIXELS - BODY_LENGTH)/2)
#define BODY_YTOP       70

#define WIRE_LENGTH     (BODY_LENGTH + 80)
#define WIRE_DIAMETER   4   
#define WIRE_XLFT       ((XPIXELS - WIRE_LENGTH)/2)
#define WIRE_YTOP       (BODY_YTOP + BODY_DIAMETER/2 - WIRE_DIAMETER/2)

#define XPOS_VAL        20
#define YPOS_VAL        (BODY_YTOP + BODY_DIAMETER + 50)
#define YPOS_MIN        (YPOS_VAL + 3*FONT_VAL.Height)
#define YPOS_MAX        (YPOS_MIN + 3*FONT_VAL.Height)

#define TS_XFUDGE           -4
#define TS_YFUDGE           -4

#define CPU_CLOCK_SPEED_MHZ 168

static BOOL             Adjusted(unsigned x, unsigned y) ;
static BOOL             Between(unsigned min, unsigned val, unsigned max) ;
static void             DelayMS(uint32_t msec) ;
static void             DisplayBands(void) ;
static void             DisplayResults(void) ;
static void             DisplayValues(void) ;
static void             Error(void) ;
static unsigned         GetTimeout(uint32_t msec) ;
static void             InitializeTouchScreen(void) ;
static uint32_t         LookUp(COLOR color, CODE *code, unsigned length) ;
static void             PaintResistor(void) ;
static uint64_t         Pow10(uint32_t exp) ;
static void             SetColorIndices(void) ;
static void             SetFontSize(sFONT *Font) ;
static char *           Value(uint64_t ohms) ;

static CODE digits[] = 
    {
    {COLOR_BLACK,       0},
    {COLOR_BROWN,       1},
    {COLOR_RED,         2},
    {COLOR_ORANGE,      3},
    {COLOR_YELLOW,      4},
    {COLOR_DARKGREEN,   5},
    {COLOR_BLUE,        6},
    {COLOR_VIOLET,      7},
    {COLOR_GRAY,        8},
    {COLOR_WHITE,       9}
    } ;

#define DIGIT(color)    LookUp(color, digits, ENTRIES(digits))

static CODE zeroes[] = 
    {
    {COLOR_BLACK,       0},     //  x1
    {COLOR_BROWN,       1},     //  x10
    {COLOR_RED,         2},     //  x100
    {COLOR_ORANGE,      3},     //  x1000
    {COLOR_YELLOW,      4},     //  x10000
    {COLOR_DARKGREEN,   5},     //  x100000
    {COLOR_BLUE,        6},     //  x1000000
    {COLOR_VIOLET,      7},     //  x10000000
    {COLOR_GRAY,        8},     //  x100000000
    {COLOR_WHITE,       9},     //  x1000000000
    } ;

#define ZEROES(color)   LookUp(color, zeroes, ENTRIES(zeroes))

static CODE percnt[] =
    {
    {COLOR_BROWN,        1},        //  1%
    {COLOR_RED,          2},        //  2%
    {COLOR_GOLD,         5},        //  5%
    {COLOR_SILVER,      10},        //  10%
    {COLOR_NONE,        20}         //  20%
    } ;

#define PERCNT(color)   LookUp(color, percnt, ENTRIES(percnt))

static COLOR colors[4] = {COLOR_RED, COLOR_BLUE, COLOR_DARKGREEN, COLOR_GOLD} ;

static BAND bands[] =
    {
    {&colors[0], digits, ENTRIES(digits), BODY_XLFT + 1*BAND_GAP + 0*BAND_WIDTH, BODY_YTOP + 1},
    {&colors[1], digits, ENTRIES(digits), BODY_XLFT + 2*BAND_GAP + 1*BAND_WIDTH, BODY_YTOP + 1},
    {&colors[2], zeroes, ENTRIES(zeroes), BODY_XLFT + 3*BAND_GAP + 2*BAND_WIDTH, BODY_YTOP + 1},
    {&colors[3], percnt, ENTRIES(percnt), BODY_XLFT + 6*BAND_GAP + 3*BAND_WIDTH, BODY_YTOP + 1}
    } ;

int main()
    {
    unsigned delay1, delay2, x, y ;

    InitializeHardware(HEADER, "Lab 8C: Resistor Color Codes") ;
    InitializeTouchScreen() ;
    SetColorIndices() ;

    delay1 = delay2 = 0 ;
    while (1)
        {
        PaintResistor() ;
        DisplayResults() ;
        DelayMS(delay1) ;
        delay1 = delay2 ;
        while (1)
            {
            if (TS_Touched())
                {
                x = TS_GetX() + TS_XFUDGE ;
                y = TS_GetY() + TS_YFUDGE ;

                if (Adjusted(x, y))
                    {
                    delay2 = 30 ;
                    break ;
                    }
                }
            else delay1 = 500 ;
            }
        }

    return 0 ;
    }

static void DisplayResults(void)
    {
    SetForeground(COLOR_BKGD) ;
    FillRect(10, YPOS_VAL - FONT_VAL.Height/2, XPIXELS - 20, 9*FONT_VAL.Height + 6) ;
    SetForeground(COLOR_BRDR) ;
    DrawRect(10, YPOS_VAL - FONT_VAL.Height/2, XPIXELS - 20, 9*FONT_VAL.Height + 6) ;

    DisplayBands() ;
    DisplayValues() ;
    }

static void DisplayBands(void)
    {
    char digit[5] ;
    int xpos, ypos ;

    SetFontSize(&FONT_BAND) ;
    SetForeground(COLOR_BLACK) ;
    SetBackground(COLOR_WHITE) ;

    ypos = bands[0].ypos + BODY_DIAMETER + 8 ;
    sprintf(digit, "%u", (unsigned) DIGIT(colors[0])) ;
    DisplayStringAt(bands[0].xpos + 1, ypos, digit) ;
    sprintf(digit, "%u", (unsigned) DIGIT(colors[1])) ;
    DisplayStringAt(bands[1].xpos + 1, ypos, digit) ;
    sprintf(digit, "%u%%", (unsigned) PERCNT(colors[3])) ;
    DisplayStringAt(bands[3].xpos + 1, ypos, digit) ;

    xpos = bands[2].xpos - 4 ;
    DisplayStringAt(xpos, ypos, "10") ;
    sprintf(digit, "%u", (unsigned) ZEROES(colors[2])) ;
    SetFontSize(&FONT_EXP) ;
    xpos += 2*FONT_BAND.Width ;
    DisplayStringAt(xpos, ypos - FONT_EXP.Height/2, digit) ;
    }

static void DisplayValues()
    {
    uint64_t ohms = (Mul32X10(DIGIT(colors[0])) + DIGIT(colors[1])) * Pow10(ZEROES(colors[2])) ;
    uint64_t delR = Div32X10(PERCNT(colors[3]) * (uint32_t) (ohms/10)) ;
    uint64_t minR = ohms - delR ;
    uint64_t maxR = ohms + delR ;
    uint64_t rand ;
    BOOL ok = TRUE ;

    ((uint32_t *) &rand)[0] = GetRandomNumber() ;
    ((uint32_t *) &rand)[1] = GetRandomNumber() ;
    if (Mul64X10(rand) != 10ULL*rand) ok = FALSE ;
    if (Mul32X10(((uint32_t *) &rand)[0]) != 10*((uint32_t *) &rand)[0]) ok = FALSE ;
    if (Div32X10(((uint32_t *) &rand)[0]) != ((uint32_t *) &rand)[0]/10) ok = FALSE ;

    SetFontSize(&FONT_VAL) ;
    SetForeground(ok ? COLOR_TEXT : COLOR_WHITE) ;
    SetBackground(ok ? COLOR_BKGD : COLOR_RED) ;

    DisplayStringAt(XPOS_VAL, YPOS_VAL, "Resistance:") ;
    DisplayStringAt(XPOS_VAL + 20, YPOS_VAL + FONT_VAL.Height + 2, Value(ohms)) ;

    DisplayStringAt(XPOS_VAL, YPOS_MIN, "Minimum:") ;
    DisplayStringAt(XPOS_VAL + 20, YPOS_MIN + FONT_VAL.Height + 2, Value(minR)) ;

    DisplayStringAt(XPOS_VAL, YPOS_MAX, "Maximum:") ;
    DisplayStringAt(XPOS_VAL + 20, YPOS_MAX + FONT_VAL.Height + 2, Value(maxR)) ;
    }

static char *Value(uint64_t ohms)
    {
    static char text[20] ;

    if (ohms >= 1000000000)
        {
        sprintf(text, "%f", ohms/1e9) ;
        text[5] = '\0' ;
        if (text[4] == '.') text[4] = '\0' ;
        strcat(text, " GigaOhms") ;
        }
    else if (ohms >= 1000000)
        {
        sprintf(text, "%f", ohms/1e6) ;
        text[5] = '\0' ;
        if (text[4] == '.') text[4] = '\0' ;
        strcat(text, " MegaOhms") ;
        }
    else if (ohms >= 1000)
        {
        sprintf(text, "%f", ohms/1e3) ;
        text[5] = '\0' ;
        if (text[4] == '.') text[4] = '\0' ;
        strcat(text, " KiloOhms") ;
        }
    else
        {
        sprintf(text, "%"PRIu64" Ohms", ohms) ;
        }

    return text ;
    }

static void SetColorIndices(void)
    {
    BAND *band ;
    int b ;

    band = &bands[0] ;
    for (b = 0; b < ENTRIES(bands); b++, band++)
        {
        CODE *code ;
        int c ;

        code = &band->code[0] ;
        for (c = 0; c < band->codes; c++, code++)
            {
            if (code->color == colors[b])
                {
                band->ndex = b ;
                break ;
                }
            }
        }
    }

static void PaintResistor(void)
    {
    BAND *band ;
    int b ;

    SetForeground(COLOR_WIRE) ;
    FillRect(WIRE_XLFT, WIRE_YTOP, WIRE_LENGTH, WIRE_DIAMETER) ;

    SetForeground(COLOR_BODY) ;
    FillRect(BODY_XLFT, BODY_YTOP, BODY_LENGTH, BODY_DIAMETER) ;
    SetForeground(COLOR_BLACK) ;
    DrawRect(BODY_XLFT, BODY_YTOP, BODY_LENGTH, BODY_DIAMETER) ;

    band = &bands[0] ;
    for (b = 0; b < ENTRIES(bands); b++, band++)
        {
        SetForeground(colors[b]) ;
        FillRect(band->xpos, band->ypos, BAND_WIDTH, BODY_DIAMETER - 1) ;
        }
    }

static BOOL Adjusted(unsigned x, unsigned y)
    {
    BAND *band ;
    int b ;

    band = &bands[0] ;
    for (b = 0; b < ENTRIES(bands); b++, band++)
        {
        if (Between(band->ypos, y, band->ypos + BODY_DIAMETER - 1))
            {
            if (Between(band->xpos, x, band->xpos + BAND_WIDTH))
                {
                band->ndex = (band->ndex + 1) % band->codes ;
                *band->color = band->code[band->ndex].color ;
                return TRUE ;
                }
            }
        }

    return FALSE ;
    }

static uint64_t Pow10(uint32_t exp)
    {
    uint64_t result = 1000000ULL ;
    while (exp-- > 0) result = Mul64X10(result) ;
    return result / 1000000ULL ;
    }

static void Error(void)
    {
    SetFontSize(&FONT_ERR) ;
    SetForeground(COLOR_BLACK) ;
    SetBackground(COLOR_WHITE) ;
    printf("ERROR!\n") ;
    exit(0) ;
    }

static uint32_t LookUp(COLOR color, CODE *code, unsigned length)
    {
    while (length-- > 0)
        {
        if (code->color == color) return code->value ;
        code++ ;
        }
    Error() ;
    return 0 ;
    }

static void InitializeTouchScreen(void)
    {
    static char *message[] =
        {
        "If this message remains on",
        "the screen, there is an",
        "initialization problem with",
        "the touch screen. This does",
        "NOT mean that there is a",
        "problem with your code.",
        " ",
        "To correct the problem,",
        "disconnect and reconnect",
        "the power.",
        NULL
        } ;
    char **pp ;
    unsigned x, y ;

    x = 25 ;
    y = 100 ;
    for (pp = message; *pp != NULL; pp++)
        {
        DisplayStringAt(x, y, *pp) ;
        y += 12 ;
        }
    TS_Init() ;
    ClearDisplay() ;
    }

static void SetFontSize(sFONT *Font)
    {
    extern void BSP_LCD_SetFont(sFONT *) ;
    BSP_LCD_SetFont(Font) ;
    }

static unsigned GetTimeout(uint32_t msec)
    {
    unsigned cycles = 1000 * msec * CPU_CLOCK_SPEED_MHZ ;
    return GetClockCycleCount() + cycles ;
    }

static void DelayMS(uint32_t msec)
    {
    uint32_t timeout = GetTimeout(msec) ;
    while ((int) (timeout - GetClockCycleCount()) > 0) ;
    }

static BOOL Between(unsigned min, unsigned val, unsigned max)
    {
    return (min <= val && val <= max) ;
    }

