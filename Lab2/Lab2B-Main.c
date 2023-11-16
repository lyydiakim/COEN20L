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
#include "touch.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

// Functions to be rewritten by student without using sprintf

void __attribute__((weak)) Bits2HexString(uint8_t bits, char string[])
    {
    sprintf(string, "%X", (unsigned) bits) ;
    }

void __attribute__((weak)) Bits2OctalString(uint8_t bits, char string[])
    {
    sprintf(string, "%o", (unsigned) bits) ;
    }

void __attribute__((weak)) Bits2SignMagString(uint8_t bits, char string[])
    {
    char sign = (bits & (1 << 7)) != 0 ? '-' : '+' ;
    unsigned mag = bits & ~(1 << 7) ;
    sprintf(string, "%c%d", sign, mag) ;
    }

void __attribute__((weak)) Bits2TwosCompString(uint8_t bits, char string[])
    {
    sprintf(string, "%+d", (int8_t) bits) ;
    }	

void __attribute__((weak)) Bits2UnsignedString(uint8_t bits, char string[])
    {
    sprintf(string, "%u", (unsigned) bits) ;
    }

#pragma GCC pop_options

typedef enum {FALSE = 0, TRUE = 1} BOOL ;

typedef struct
    {
    char *                  label ;
    void                    (*convert)(uint8_t, char []) ;
    BOOL                    (*verify)(uint8_t, char []) ;
    } VALUE ;

typedef struct
    {
    char *                  label ;
    int                     ytop ;
    int                     count ;
    VALUE *                 values ;
    uint32_t                background ;
    BOOL                    init ;
    } DISPLAY ;

typedef struct
    {
    uint8_t                 bits ;        // value to be adjusted
    const uint8_t           vmin ;        // lower limit of value
    const uint8_t           vmax ;        // upper limit of value
    const uint32_t          foreground ;  // foreground color of slider
    const uint32_t          background ;  // background color of slider
    const int               xmin ;        // left edge of middle slider
    const int               ymin ;        // top edge of adjustment tool
    const int               width ;       // width of middle slider
    const int               height ;      // height of adjustment tool
    const int               xdec ;        // left edge of decrement button
    const int               xinc ;        // left edge of increment button
    uint32_t                xpos ;        // current x position of slider
    } ADJUST ;

typedef struct
    {
    const uint8_t *         table ;
    const uint16_t          Width ;
    const uint16_t          Height ;
    } sFONT ;

#define UNSIGNED_YTOP       55
#define SIGNED_YTOP         145
#define INPUT_YTOP          235
#define ENTRIES(a)          (sizeof(a)/sizeof(a[0]))
#define SCREEN_ADRS         0xD0000000

#define DISPLAY_XMARGIN     4
#define DISPLAY_XPADDING    4
#define DISPLAY_YPADDING    2
#define DISPLAY_YSPACING    3

#define ADJUST_MARGIN       4
#define ADJUST_SPACING      4
#define ADJUST_SIZE         18

#define SIGNS               "+-"

// Public fonts defined in run-time library
extern sFONT                Font8, Font12, Font16, Font20, Font24 ;

// Global variables private to this file
static sFONT *              font_label = &Font12 ;
static sFONT *              font_value = &Font16 ;
static ADJUST               adjust =
    {
    0b10000000,                                                      // bits
    0b00000000,                                                      // vmin
    0b11111111,                                                      // vmax
    COLOR_BLACK,                                                     // foreground
    0xFFE3E3E3,                                                      // background
    ADJUST_MARGIN + ADJUST_SIZE + ADJUST_SPACING,                    // xlft
    280,                                                             // ytop
    XPIXELS - 2*(ADJUST_MARGIN + ADJUST_SIZE + ADJUST_SPACING),      // width
    ADJUST_SIZE,                                                     // height
    ADJUST_MARGIN,                                                   // xdec
    XPIXELS - ADJUST_MARGIN - ADJUST_SIZE,                           // xinc
    0                                                                // xpos
    } ;

// Functions provate to this source code file
static int32_t              Between(uint32_t min, uint32_t val, uint32_t max) ;
static void                 Bits2BinaryString(uint8_t bits, char string[]) ;
static void                 Bits2OnesCompString(uint8_t bits, char string[]) ;
static void                 CheckAdjust(void) ;
static void                 Delay(uint32_t msec) ;
static BOOL                 DigitsOK(char *string, char *digits) ;
static void                 DisplayValues(uint8_t bits, DISPLAY *display) ;
static void                 DoAdjust(ADJUST *adjust) ;
static BOOL                 Expired(uint32_t timeout) ;
static uint32_t             GetTimeout(uint32_t msec) ;
static void                 InitializeTouchScreen(void) ;
static void                 InitSlider(ADJUST *adjust) ;
static void                 LEDs(int grn_on, int red_on) ;
static void                 SetFontSize(sFONT *pFont) ;
static void                 UpdateSlider(ADJUST *adjust, uint32_t x) ;
static void                 UpdateValue(ADJUST *adjust, uint32_t x) ;
static BOOL                 ValueOK(char *valid, char *string, unsigned radix) ;
static BOOL                 VerifyBinaryString(uint8_t bits, char string[]) ;
static BOOL                 VerifyHexString(uint8_t bits, char string[]) ;
static BOOL                 VerifyOctalString(uint8_t bits, char string[]) ;
static BOOL                 VerifyOneCompString(uint8_t bits, char string[]) ;
static BOOL                 VerifySignMagString(uint8_t bits, char string[]) ;
static BOOL                 VerifyTwosCompString(uint8_t bits, char string[]) ;
static BOOL                 VerifyUnsignedString(uint8_t bits, char string[]) ;

int main()
    {
    static VALUE _unsigned[] =
        {
        {"Octal",            Bits2OctalString,      VerifyOctalString},
        {"Decimal",          Bits2UnsignedString,   VerifyUnsignedString},
        {"Hexadecimal",      Bits2HexString,        VerifyHexString},
        } ;
    static VALUE _signed[] =
        {
        {"One's Complement", Bits2OnesCompString,   VerifyOneCompString},
        {"Two's Complement", Bits2TwosCompString,   VerifyTwosCompString},
        {"Signed Magnitude", Bits2SignMagString,    VerifySignMagString}
        } ;
    static VALUE representation[] =
        {
        {"Binary",           Bits2BinaryString,     VerifyBinaryString},
        } ;
    static DISPLAY _unsigneds =
        {
        "Unsigned Interpretations:",
        UNSIGNED_YTOP,
        ENTRIES(_unsigned),
        _unsigned,
        COLOR_YELLOW,
        TRUE
        } ;
    static DISPLAY _signeds =
        {
        "Signed Interpretations:",
        SIGNED_YTOP,
        ENTRIES(_signed),
        _signed,
        COLOR_YELLOW,
        TRUE
        } ;
    static DISPLAY representations =
        {
        "8-bit Representation:",
        INPUT_YTOP,
        ENTRIES(representation),
        representation,
        COLOR_LIGHTCYAN,
        TRUE
        } ;
    static unsigned prev = (unsigned) -1 ;
	
    InitializeHardware(HEADER, "Lab 2B: Interpreting Binary") ;
    InitializeTouchScreen() ;
    LEDs(1, 0) ;

    InitSlider(&adjust) ;

    for (;;)
        {
        CheckAdjust() ;

        if (adjust.bits == prev) continue ;
        prev = adjust.bits ;

        DisplayValues(adjust.bits, &_unsigneds) ;
        DisplayValues(adjust.bits, &_signeds) ;
        DisplayValues(adjust.bits, &representations) ;
        }

	return 0 ;
	}

static void DisplayValues(uint8_t bits, DISPLAY *display)
    {
    const int xright = XPIXELS - DISPLAY_XMARGIN - DISPLAY_XPADDING ;
    VALUE *value ;
    int y ;

    SetFontSize(font_value) ;

    if (display->init)
        {
        int xlft, ytop, width, height ;

        xlft = DISPLAY_XMARGIN ;
        SetFontSize(font_label) ;
        SetForeground(COLOR_BLACK) ;
        SetBackground(COLOR_WHITE) ;
        DisplayStringAt(xlft + DISPLAY_XPADDING, display->ytop, display->label) ;

        width = XPIXELS - 2*DISPLAY_XMARGIN ;
        ytop = display->ytop + font_label->Height + DISPLAY_YPADDING ;
        height = 2*DISPLAY_YPADDING + display->count*font_value->Height + (display->count-1)*DISPLAY_YSPACING + 4 ;

        SetForeground(display->background) ;
        FillRect(xlft, ytop, width, height) ;
        SetForeground(COLOR_BLACK) ;
        DrawRect(xlft, ytop, width, height) ;

        SetFontSize(font_value) ;
        SetBackground(display->background) ;
        y = display->ytop + font_value->Height + 2*DISPLAY_YPADDING + 1 ;
        value = display->values ;
        for (int which = 0; which < display->count; which++, value++)
            {
            DisplayStringAt(DISPLAY_XMARGIN + DISPLAY_XPADDING, y, value->label) ;
            y += font_value->Height + DISPLAY_YSPACING ;
            }

        display->init = FALSE ;
        }

    SetFontSize(font_value) ;
    SetBackground(display->background) ;
    y = display->ytop + font_value->Height + 2*DISPLAY_YPADDING + 1 ;
    value = display->values ;
    for (int which = 0; which < display->count; which++, value++)
        {
        static char blanks[] = "    " ;
        char string[10] ;
        BOOL valid ;

        DisplayStringAt(xright - font_value->Width * strlen(blanks), y, blanks) ;
        (*value->convert)(bits, string) ;
        valid = (*value->verify)(bits, string) ;
        if (!valid) LEDs(0, 1) ;
        SetForeground(valid ? COLOR_BLACK : COLOR_RED) ;
        DisplayStringAt(xright - font_value->Width * strlen(string), y, string) ;
        y += font_value->Height + DISPLAY_YSPACING ;
        }
    }

static void Bits2BinaryString(uint8_t bits, char string[])
    {
    for (int pos = 0; pos < 8; pos++)
        {
        int bit = ((bits >> (7 - pos)) & 1) ;
        string[pos] = (bit == 1) ? '1' : '0' ;
        }
    string[8] = '\0' ;
    }

static void Bits2OnesCompString(uint8_t bits, char string[])
    {
    if ((bits & (1 << 7)) != 0) bits ^= 0b01111111 ;
    Bits2SignMagString(bits, string) ;
    }

static BOOL VerifyBinaryString(uint8_t bits, char string[])
    {
    return TRUE ;
    }

static BOOL VerifyHexString(uint8_t bits, char string[])
    {
    char valid[10] ;

    sprintf(valid, "%X", (unsigned) bits) ;
    return DigitsOK(string, "0123456789ABCDEFabcdef") && ValueOK(string, valid, 16) ;
    }

static BOOL VerifyOctalString(uint8_t bits, char string[])
    {
    char valid[10] ;

    sprintf(valid, "%o", (unsigned) bits) ;
    return DigitsOK(string, "01234567") && ValueOK(string, valid, 8) ;
    }

static BOOL VerifyOneCompString(uint8_t bits, char string[])
    {
    return TRUE ;
    }

static BOOL VerifySignMagString(uint8_t bits, char string[])
    {
    char valid[10] ;
    char sign = (bits & (1 << 7)) != 0 ? '-' : '+' ;
    unsigned mag = bits & ~(1 << 7) ;
    sprintf(valid, "%c%d", sign, mag) ;
    return DigitsOK(string, SIGNS "0123456789") && ValueOK(string, valid, 10) ;
    }

static BOOL VerifyTwosCompString(uint8_t bits, char string[])
    {
    char valid[10] ;

    sprintf(valid, "%d", (int8_t) bits) ;
    return DigitsOK(string, SIGNS "0123456789") && ValueOK(string, valid, 10) ;
    }

static BOOL VerifyUnsignedString(uint8_t bits, char string[])
    {
    char valid[10] ;

    sprintf(valid, "%u", (unsigned) bits) ;
    return DigitsOK(string, "0123456789") && ValueOK(string, valid, 10) ;
    }

static BOOL DigitsOK(char *string, char *digits)
    {
    while (*string == ' ') string++ ;
    if (strncmp(digits, SIGNS, 2) == 0)
       {
       if (strchr(SIGNS, string[0]) != NULL)
          {
          string += 1 ;
          digits += 2 ;
          }
       }
    return strspn(string, digits) == strlen(string) ;
    }

static BOOL ValueOK(char *valid, char *string, unsigned radix)
    {
    return strtol(string, NULL, radix) == strtol(valid, NULL, radix) ;
    }

static void SetFontSize(sFONT *Font)
    {
    extern void BSP_LCD_SetFont(sFONT *) ;
    BSP_LCD_SetFont(Font) ;
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

static void InitSlider(ADJUST *adjust)
    {
    int xmin, xmax ;
    float percent ;

    SetForeground(adjust->background) ;
    FillRect(adjust->xmin + 1, adjust->ymin, adjust->width - 1, adjust->height) ;
    SetForeground(COLOR_RED) ;
    DrawRect(adjust->xmin, adjust->ymin, adjust->width, adjust->height) ;

    percent = (float) (adjust->bits - adjust->vmin) / (adjust->vmax - adjust->vmin) ;
    xmin = adjust->xmin + ADJUST_SIZE/2 ;
    xmax = adjust->xmin + adjust->width - ADJUST_SIZE/2 - 1 ;
    UpdateSlider(adjust, adjust->xpos = xmin + percent * (xmax - xmin)) ;

    SetFontSize(&Font20) ;

    SetForeground(adjust->background) ;
    FillRect(adjust->xdec + 1, adjust->ymin, ADJUST_SIZE - 1, adjust->height) ;
    SetForeground(COLOR_BLACK) ;
    SetBackground(adjust->background) ;
    DisplayChar(adjust->xdec + 3, adjust->ymin + 1, '-') ;
    SetForeground(COLOR_RED) ;
    DrawRect(adjust->xdec, adjust->ymin, ADJUST_SIZE, adjust->height) ;

    SetForeground(adjust->background) ;
    FillRect(adjust->xinc + 1, adjust->ymin, ADJUST_SIZE - 1, adjust->height) ;
    SetForeground(COLOR_BLACK) ;
    SetBackground(adjust->background) ;
    DisplayChar(adjust->xinc + 3, adjust->ymin + 1, '+') ;
    SetForeground(COLOR_RED) ;
    DrawRect(adjust->xinc, adjust->ymin, ADJUST_SIZE, adjust->height) ;
    }

static void UpdateSlider(ADJUST *adjust, uint32_t x)
    {
    static uint32_t bg[ADJUST_SIZE], fg[ADJUST_SIZE] ;
    int k, xold, xnew, ypos, yoff ;
    static BOOL init = TRUE ;

    if (init)
        {
        for (k = 0; k < ADJUST_SIZE; k++)
            {
            bg[k] = adjust->background ;
            fg[k] = adjust->foreground ;
            }
        init = FALSE ;
        }

    xold = adjust->xpos - ADJUST_SIZE/2 + 1 ;
    xnew = x - ADJUST_SIZE/2 + 1 ;
    ypos = adjust->ymin + 1 ;
    for (yoff = 0; yoff < adjust->height - 1; yoff++)
        {
        uint32_t *base = (uint32_t *) SCREEN_ADRS + XPIXELS*ypos++ ;
        memcpy(base + xold, bg, sizeof(bg)) ;
        memcpy(base + xnew, fg, sizeof(fg)) ;
        }
    adjust->xpos = x ;
    }

static void CheckAdjust(void)
    {
    if (TS_Touched()) DoAdjust(&adjust) ;
    }

static void DoAdjust(ADJUST *adjust)
    {
#   define  XFUDGE  0
#   define  YFUDGE  -10
    int x, y, xmin, xmax ;
    uint32_t timeout ;
    float percent ;

    y = TS_GetY() + YFUDGE ;
    if (!Between(adjust->ymin, y, adjust->ymin + adjust->height - 1))   return ;

    x = TS_GetX() + XFUDGE ;
    xmin = adjust->xmin + ADJUST_SIZE/2 ;
    xmax = adjust->xmin + adjust->width - ADJUST_SIZE/2 - 1 ;

    if (Between(adjust->xmin, x, adjust->xmin + adjust->width - 1))
        {
        if (x < xmin) x = xmin ;
        if (x > xmax) x = xmax ;
        UpdateValue(adjust, x) ;
        UpdateSlider(adjust, x) ;
        return ;
        }

    if (Between(adjust->xdec, x, adjust->xdec + ADJUST_SIZE -1) && adjust->bits > adjust->vmin)
        {
        percent = (float) (--adjust->bits - adjust->vmin) / (adjust->vmax - adjust->vmin) ;
        }

    else if (Between(adjust->xinc, x, adjust->xinc + ADJUST_SIZE -1) && adjust->bits < adjust->vmax)
        {
        percent = (float) (++adjust->bits - adjust->vmin) / (adjust->vmax - adjust->vmin) ;
        }
    else return ;

    UpdateSlider(adjust, xmin + percent * (xmax - xmin)) ;
    timeout = GetTimeout(300) ;
    do
        {
        if (Expired(timeout)) break ;
        Delay(100) ;
        } while (TS_Touched()) ;
    }

static int32_t Between(uint32_t min, uint32_t val, uint32_t max)
    {
    return (min <= val && val <= max) ;
    }

static void UpdateValue(ADJUST *adjust, uint32_t x)
    {
    float percent = (float) (x - adjust->xmin - ADJUST_SIZE/2) / (adjust->width - ADJUST_SIZE - 1) ;
    adjust->bits = adjust->vmin + percent * (adjust->vmax - adjust->vmin) ;
    }

static uint32_t GetTimeout(uint32_t msec)
    {
#   define  CPU_CLOCK_SPEED_MHZ         168
    uint32_t cycles = 1000 * msec * CPU_CLOCK_SPEED_MHZ ;
    return GetClockCycleCount() + cycles ;
    }

static BOOL Expired(uint32_t timeout)
    {
    return (int) (GetClockCycleCount() - timeout) >= 0 ;
    }

static void Delay(uint32_t msec)
    {
    uint32_t timeout = GetTimeout(msec) ;
    while (!Expired(timeout)) ;
    }

static void LEDs(int grn_on, int red_on)
    {
    static uint32_t * const pGPIOG_MODER    = (uint32_t *) 0x40021800 ;
    static uint32_t * const pGPIOG_ODR      = (uint32_t *) 0x40021814 ;
    
    *pGPIOG_MODER |= (1 << 28) | (1 << 26) ;    // output mode
    *pGPIOG_ODR &= ~(3 << 13) ;         // both off
    *pGPIOG_ODR |= (grn_on ? 1 : 0) << 13 ;
    *pGPIOG_ODR |= (red_on ? 1 : 0) << 14 ;
    }
