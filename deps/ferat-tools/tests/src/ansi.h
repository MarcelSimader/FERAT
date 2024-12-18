// Author: Marcel Simader (marcel.simader@jku.at)
// Date: 26.11.2021
// (c) Marcel Simader 2021, Johannes Kepler Universität Linz

#ifndef LIBSE_ANSI
#define LIBSE_ANSI

#include <stdbool.h>

#ifdef LIBSE_ANSI_DISABLE_COLOR
#define ANSI_ESC        ((void)0)
#define ANSI_CSI        ((void)0)
#define ANSI_GRAPHIC(c) ((void)0)
#else
#define ANSI_ESC        "\033"
#define ANSI_CSI        ANSI_ESC "["
#define ANSI_GRAPHIC(c) ANSI_CSI #c "m"
#endif

#define ANSI_NORMAL            ANSI_GRAPHIC(0)
//
#define ANSI_BLACK             ANSI_GRAPHIC(30)
#define ANSI_RED               ANSI_GRAPHIC(31)
#define ANSI_GREEN             ANSI_GRAPHIC(32)
#define ANSI_YELLOW            ANSI_GRAPHIC(33)
#define ANSI_BLUE              ANSI_GRAPHIC(34)
#define ANSI_MAGENTA           ANSI_GRAPHIC(35)
#define ANSI_CYAN              ANSI_GRAPHIC(36)
#define ANSI_WHITE             ANSI_GRAPHIC(37)
#define ANSI_BRIGHT_BLACK      ANSI_GRAPHIC(90)
#define ANSI_BRIGHT_RED        ANSI_GRAPHIC(91)
#define ANSI_BRIGHT_GREEN      ANSI_GRAPHIC(92)
#define ANSI_BRIGHT_YELLOW     ANSI_GRAPHIC(93)
#define ANSI_BRIGHT_BLUE       ANSI_GRAPHIC(94)
#define ANSI_BRIGHT_MAGENTA    ANSI_GRAPHIC(95)
#define ANSI_BRIGHT_CYAN       ANSI_GRAPHIC(96)
#define ANSI_BRIGHT_WHITE      ANSI_GRAPHIC(97)
//
#define ANSI_BG_BLACK          ANSI_GRAPHIC(40)
#define ANSI_BG_RED            ANSI_GRAPHIC(41)
#define ANSI_BG_GREEN          ANSI_GRAPHIC(42)
#define ANSI_BG_YELLOW         ANSI_GRAPHIC(43)
#define ANSI_BG_BLUE           ANSI_GRAPHIC(44)
#define ANSI_BG_MAGENTA        ANSI_GRAPHIC(45)
#define ANSI_BG_CYAN           ANSI_GRAPHIC(46)
#define ANSI_BG_WHITE          ANSI_GRAPHIC(47)
#define ANSI_BG_BRIGHT_BLACK   ANSI_GRAPHIC(100)
#define ANSI_BG_BRIGHT_RED     ANSI_GRAPHIC(101)
#define ANSI_BG_BRIGHT_GREEN   ANSI_GRAPHIC(102)
#define ANSI_BG_BRIGHT_YELLOW  ANSI_GRAPHIC(103)
#define ANSI_BG_BRIGHT_BLUE    ANSI_GRAPHIC(104)
#define ANSI_BG_BRIGHT_MAGENTA ANSI_GRAPHIC(105)
#define ANSI_BG_BRIGHT_CYAN    ANSI_GRAPHIC(106)
#define ANSI_BG_BRIGHT_WHITE   ANSI_GRAPHIC(107)

#endif

