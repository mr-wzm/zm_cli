/*****************************************************************
* Copyright (C) 2020 ZM Technology Personal.                     *
******************************************************************
* zm_printf.h
*
* DESCRIPTION:
*     zm printf
* AUTHOR:
*     zm
* CREATED DATE:
*     2020/10/25
* REVISION:
*     v0.1
*
* MODIFICATION HISTORY
* --------------------
* $Log:$
*
*****************************************************************/
#ifndef __ZM_PRINTF_H__
#define __ZM_PRINTF_H__
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "zm_config.h"
#include "zm_common.h"
//#include "zm_cli.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#define ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY        (1u << 0)
#define ZM_CLI_FORMAT_FLAG_PAD_ZERO            (1u << 1)
#define ZM_CLI_FORMAT_FLAG_PRINT_SIGN          (1u << 2)
#define ZM_CLI_FORMAT_DOUBLE_DEF_PRECISION     6

#define ZM_CLI_FORMAT_DOUBLE_SIGN_POSITION     63U
#define ZM_CLI_FORMAT_DOUBLE_SIGN_MASK         1ULL
#define ZM_CLI_FORMAT_DOUBLE_SIGN              (ZM_CLI_FORMAT_DOUBLE_SIGN_MASK << ZM_CLI_FORMAT_DOUBLE_SIGN_POSITION)
#define ZM_CLI_FORMAT_DOUBLE_EXP_POSITION      52U
#define ZM_CLI_FORMAT_DOUBLE_EXP_MASK          0x7FFULL
#define ZM_CLI_FORMAT_DOUBLE_EXP               (ZM_CLI_FORMAT_DOUBLE_EXP_MASK << ZM_CLI_FORMAT_DOUBLE_EXP_POSITION)
#define ZM_CLI_FORMAT_DOUBLE_MANT_POSITION     0U
#define ZM_CLI_FORMAT_DOUBLE_MANT_MASK         0xFFFFFFFFFFFFF
#define ZM_CLI_FORMAT_DOUBLE_MANT              (ZM_CLI_FORMAT_DOUBLE_MANT_MASK << ZM_CLI_FORMAT_DOUBLE_MANT_POSITION)

#define ZM_CLI_FORMAT_DOUBLE_SIGN_GET(v)       (!!((v) & ZM_CLI_FORMAT_DOUBLE_SIGN))
#define ZM_CLI_FORMAT_DOUBLE_EXP_GET(v)        (((v) & ZM_CLI_FORMAT_DOUBLE_EXP) >> ZM_CLI_FORMAT_DOUBLE_EXP_POSITION)
#define ZM_CLI_FORMAT_DOUBLE_MANT_GET(v)       (((v) & ZM_CLI_FORMAT_DOUBLE_MANT) >> ZM_CLI_FORMAT_DOUBLE_MANT_POSITION)
#define ZM_CLI_FORMAT_REQ_SIGN_SPACE(s, f)     ((s) | (!!((f) & ZM_CLI_FORMAT_FLAG_PRINT_SIGN)))

#define HIGH_32(v)                              ((v) >> 32)
#define LOW_32(v)                               (((1ULL << 32) - 1) & v)
    
/**
 * @brief CLI colors for @ref zm_cli_printf function.
 */
#define ZM_CLI_DEFAULT  ZM_CLI_VT100_COLOR_DEFAULT    /**< Turn off character attributes. */
#define ZM_CLI_NORMAL   ZM_CLI_VT100_COLOR_WHITE      /**< Normal color printf.           */
#define ZM_CLI_INFO     ZM_CLI_VT100_COLOR_GREEN      /**< Info color printf.             */
#define ZM_CLI_OPTION   ZM_CLI_VT100_COLOR_CYAN       /**< Option color printf.           */
#define ZM_CLI_WARNING  ZM_CLI_VT100_COLOR_YELLOW     /**< Warning color printf.          */
#define ZM_CLI_ERROR    ZM_CLI_VT100_COLOR_RED        /**< Error color printf.            */
    
#define ZM_CLI_VT100_SETNL                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', '0', 'h', '\0'               \
    } /* Set new line mode */
#define ZM_CLI_VT100_SETAPPL                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '1', 'h', '\0'               \
    } /* Set cursor key to application */
#define ZM_CLI_VT100_SETCOL_132                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '3', 'h', '\0'               \
    } /* Set number of columns to 132 */
#define ZM_CLI_VT100_SETSMOOTH                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '4', 'h', '\0'               \
    } /* Set smooth scrolling */
#define ZM_CLI_VT100_SETREVSCRN                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '5', 'h', '\0'               \
    } /* Set reverse video on screen */
#define ZM_CLI_VT100_SETORGREL                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '6', 'h', '\0'               \
    } /* Set origin to relative */
#define ZM_CLI_VT100_SETWRAP_ON                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '7', 'h', '\0'               \
    } /* Set auto-wrap mode */
#define ZM_CLI_VT100_SETWRAP_OFF                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '7', 'l', '\0'               \
    } /* Set auto-wrap mode */

#define ZM_CLI_VT100_SETREP                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '8', 'h', '\0'               \
    } /* Set auto-repeat mode */
#define ZM_CLI_VT100_SETINTER                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '9', 'h', '\0'               \
    } /* Set interlacing mode */

#define ZM_CLI_VT100_SETLF                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', '0', 'l', '\0'               \
    } /* Set line feed mode */
#define ZM_CLI_VT100_SETCURSOR                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '1', 'l', '\0'               \
    } /* Set cursor key to cursor */
#define ZM_CLI_VT100_SETVT52                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '2', 'l', '\0'               \
    } /* Set VT52 (versus ANSI) */
#define ZM_CLI_VT100_SETCOL_80                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '3', 'l', '\0'               \
    } /* Set number of columns to 80 */
#define ZM_CLI_VT100_SETJUMP                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '4', 'l', '\0'               \
    } /* Set jump scrolling */
#define ZM_CLI_VT100_SETNORMSCRN                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '5', 'l', '\0'               \
    } /* Set normal video on screen */
#define ZM_CLI_VT100_SETORGABS                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '6', 'l', '\0'               \
    } /* Set origin to absolute */
#define ZM_CLI_VT100_RESETWRAP                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '7', 'l', '\0'               \
    } /* Reset auto-wrap mode */
#define ZM_CLI_VT100_RESETREP                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '8', 'l', '\0'               \
    } /* Reset auto-repeat mode */
#define ZM_CLI_VT100_RESETINTER                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '9', 'l', '\0'               \
    } /* Reset interlacing mode */

#define ZM_CLI_VT100_ALTKEYPAD                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '=', '\0'                              \
    } /* Set alternate keypad mode */
#define ZM_CLI_VT100_NUMKEYPAD                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '>', '\0'                              \
    } /* Set numeric keypad mode */

#define ZM_CLI_VT100_SETUKG0                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '(', 'A', '\0'                         \
    } /* Set United Kingdom G0 character set */
#define ZM_CLI_VT100_SETUKG1                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, ')', 'A', '\0'                         \
    } /* Set United Kingdom G1 character set */
#define ZM_CLI_VT100_SETUSG0                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '(', 'B', '\0'                         \
    } /* Set United States G0 character set */
#define ZM_CLI_VT100_SETUSG1                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, ')', 'B', '\0'                         \
    } /* Set United States G1 character set */
#define ZM_CLI_VT100_SETSPECG0                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '(', '0', '\0'                         \
    } /* Set G0 special chars. & line set */
#define ZM_CLI_VT100_SETSPECG1                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, ')', '0', '\0'                         \
    } /* Set G1 special chars. & line set */
#define ZM_CLI_VT100_SETALTG0                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '(', '1', '\0'                         \
    } /* Set G0 alternate character ROM */
#define ZM_CLI_VT100_SETALTG1                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, ')', '1', '\0'                         \
    } /* Set G1 alternate character ROM */
#define ZM_CLI_VT100_SETALTSPECG0                                      \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '(', '2', '\0'                         \
    } /* Set G0 alt char ROM and spec. graphics */
#define ZM_CLI_VT100_SETALTSPECG1                                      \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, ')', '2', '\0'                         \
    } /* Set G1 alt char ROM and spec. graphics */

#define ZM_CLI_VT100_SETSS2                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'N', '\0'                              \
    } /* Set single shift 2 */
#define ZM_CLI_VT100_SETSS3                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', '\0'                              \
    } /* Set single shift 3 */

#define ZM_CLI_VT100_MODESOFF                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'm', '\0'                         \
    } /* Turn off character attributes */
#define ZM_CLI_VT100_MODESOFF_                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '0', 'm', '\0'                    \
    } /* Turn off character attributes */
#define ZM_CLI_VT100_BOLD                                              \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '1', 'm', '\0'                    \
    } /* Turn bold mode on */
#define ZM_CLI_VT100_LOWINT                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', 'm', '\0'                    \
    } /* Turn low intensity mode on */
#define ZM_CLI_VT100_UNDERLINE                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '4', 'm', '\0'                    \
    } /* Turn underline mode on */
#define ZM_CLI_VT100_BLINK                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '5', 'm', '\0'                    \
    } /* Turn blinking mode on */
#define ZM_CLI_VT100_REVERSE                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '7', 'm', '\0'                    \
    } /* Turn reverse video on */
#define ZM_CLI_VT100_INVISIBLE                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '8', 'm', '\0'                    \
    } /* Turn invisible text mode on */

#define ZM_CLI_VT100_SETWIN(t, b)                                      \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', (t), ';', (b), 'r', '\0'          \
    } /* Set top and bottom line#s of a window */

#define ZM_CLI_VT100_CURSORUP(n)                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', (n), 'A', '\0'                    \
    } /* Move cursor up n lines */
#define ZM_CLI_VT100_CURSORDN(n)                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', (n), 'B', '\0'                    \
    } /* Move cursor down n lines */
#define ZM_CLI_VT100_CURSORRT(n)                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', (n), 'C', '\0'                    \
    } /* Move cursor right n lines */
#define ZM_CLI_VT100_CURSORLF(n)                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', (n), 'D', '\0'                    \
    } /* Move cursor left n lines */
#define ZM_CLI_VT100_CURSORHOME                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'H', '\0'                         \
    } /* Move cursor to upper left corner */
#define ZM_CLI_VT100_CURSORHOME_                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', ';', 'H', '\0'                    \
    } /* Move cursor to upper left corner */
#define ZM_CLI_VT100_CURSORPOS(v, h)                                   \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', (v), ';', (h), 'H', '\0'          \
    } /* Move cursor to screen location v,h */

#define ZM_CLI_VT100_HVHOME                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'f', '\0'                         \
    } /* Move cursor to upper left corner */
#define ZM_CLI_VT100_HVHOME_                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', ';', 'f', '\0'                    \
    } /* Move cursor to upper left corner */
#define ZM_CLI_VT100_HVPOS(v, h)                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', (v), ';', (h), 'f', '\0'          \
    } /* Move cursor to screen location v,h */
#define ZM_CLI_VT100_INDEX                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'D', '\0'                              \
    } /* Move/scroll window up one line */
#define ZM_CLI_VT100_REVINDEX                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'M', '\0'                              \
    } /* Move/scroll window down one line */
#define ZM_CLI_VT100_NEXTLINE                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'E', '\0'                              \
    } /* Move to next line */
#define ZM_CLI_VT100_SAVECURSOR                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '7', '\0'                              \
    } /* Save cursor position and attributes */
#define ZM_CLI_VT100_RESTORECURSOR                                     \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '8', '\0'                              \
    } /* Restore cursor position and attribute */

#define ZM_CLI_VT100_TABSET                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'H', '\0'                              \
    } /* Set a tab at the current column */
#define ZM_CLI_VT100_TABCLR                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'g', '\0'                         \
    } /* Clear a tab at the current column */
#define ZM_CLI_VT100_TABCLR_                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '0', 'g', '\0'                    \
    } /* Clear a tab at the current column */
#define ZM_CLI_VT100_TABCLRALL                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '3', 'g', '\0'                    \
    } /* Clear all tabs */

#define ZM_CLI_VT100_DHTOP                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '#', '3', '\0'                         \
    } /* Double-height letters, top half */
#define ZM_CLI_VT100_DHBOT                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '#', '4', '\0'                         \
    } /* Double-height letters, bottom hal */
#define ZM_CLI_VT100_SWSH                                              \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '#', '5', '\0'                         \
    } /* Single width, single height letters */
#define ZM_CLI_VT100_DWSH                                              \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '#', '6', '\0'                         \
    } /* Double width, single height letters */

#define ZM_CLI_VT100_CLEAREOL                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'K', '\0'                         \
    } /* Clear line from cursor right */
#define ZM_CLI_VT100_CLEAREOL_                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '0', 'K', '\0'                    \
    } /* Clear line from cursor right */
#define ZM_CLI_VT100_CLEARBOL                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '1', 'K', '\0'                    \
    } /* Clear line from cursor left */
#define ZM_CLI_VT100_CLEARLINE                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', 'K', '\0'                    \
    } /* Clear entire line */

#define ZM_CLI_VT100_CLEAREOS                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'J', '\0'                         \
    } /* Clear screen from cursor down */
#define ZM_CLI_VT100_CLEAREOS_                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '0', 'J', '\0'                    \
    } /* Clear screen from cursor down */
#define ZM_CLI_VT100_CLEARBOS                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '1', 'J', '\0'                    \
    } /* Clear screen from cursor up */
#define ZM_CLI_VT100_CLEARSCREEN                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', 'J', '\0'                    \
    } /* Clear entire screen */

#define ZM_CLI_VT100_DEVSTAT                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '5', 'n', '\0'                         \
    } /* Device status report */
#define ZM_CLI_VT100_TERMOK                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '0', 'n', '\0'                         \
    } /* Response: terminal is OK */
#define ZM_CLI_VT100_TERMNOK                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '3', 'n', '\0'                         \
    } /* Response: terminal is not OK */

#define ZM_CLI_VT100_GETCURSOR                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '6', 'n', '\0'                    \
    } /* Get cursor position */
#define ZM_CLI_VT100_CURSORPOSAT                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, (v), ';', (h), 'R', '\0'               \
    } /* Response: cursor is at v,h */

#define ZM_CLI_VT100_IDENT                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'c', '\0'                         \
    } /* Identify what terminal type */
#define ZM_CLI_VT100_IDENT_                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '0', 'c', '\0'                    \
    } /* Identify what terminal type */
#define ZM_CLI_VT100_GETTYPE                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '?', '1', ';', (n), '0', 'c', '\0'\
    } /* Response: terminal type code n */

#define ZM_CLI_VT100_RESET                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'c', '\0'                              \
    } /*  Reset terminal to initial state */

#define ZM_CLI_VT100_ALIGN                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '#', '8', '\0'                         \
    } /* Screen alignment display */
#define ZM_CLI_VT100_TESTPU                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', ';', '1', 'y', '\0'          \
    } /* Confidence power up test */
#define ZM_CLI_VT100_TESTLB                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', ';', '2', 'y', '\0'          \
    } /* Confidence loopback test */
#define ZM_CLI_VT100_TESTPUREP                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', ';', '9', 'y', '\0'          \
    } /* Repeat power up test */
#define ZM_CLI_VT100_TESTLBREP                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', ';', '1', '0', 'y', '\0'     \
    } /* Repeat loopback test */

#define ZM_CLI_VT100_LEDSOFF                                           \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '0', 'q', '\0'                    \
    } /* Turn off all four leds */
#define ZM_CLI_VT100_LED1                                              \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '1', 'q', '\0'                    \
    } /* Turn on LED #1 */
#define ZM_CLI_VT100_LED2                                              \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '2', 'q', '\0'                    \
    } /* Turn on LED #2 */
#define ZM_CLI_VT100_LED3                                              \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '3', 'q', '\0'                    \
    } /* Turn on LED #3 */
#define ZM_CLI_VT100_LED4                                              \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', '4', 'q', '\0'                    \
    } /* Turn on LED #4 */

/* Function Keys */

#define ZM_CLI_VT100_PF1                                               \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'P', '\0'                         \
    }
#define ZM_CLI_VT100_PF2                                               \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'Q', '\0'                         \
    }
#define ZM_CLI_VT100_PF3                                               \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'R', '\0'                         \
    }
#define ZM_CLI_VT100_PF4                                               \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'S', '\0'                         \
    }

/* Arrow keys */

#define ZM_CLI_VT100_UP_RESET                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'A', '\0'                              \
    }
#define ZM_CLI_VT100_UP_SET                                            \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'A', '\0'                         \
    }
#define ZM_CLI_VT100_DOWN_RESET                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'B', '\0'                              \
    }
#define ZM_CLI_VT100_DOWN_SET                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'B', '\0'                         \
    }
#define ZM_CLI_VT100_RIGHT_RESET                                       \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'C', '\0'                              \
    }
#define ZM_CLI_VT100_RIGHT_SET                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'C', '\0'                         \
    }
#define ZM_CLI_VT100_LEFT_RESET                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'D', '\0'                              \
    }
#define ZM_CLI_VT100_LEFT_SET                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'D', '\0'                         \
    }

/* Numeric Keypad Keys */

#define ZM_CLI_VT100_NUMERIC_0                                         \
    {                                                                   \
        '0', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_0                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'p', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_1                                         \
    {                                                                   \
        '1', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_1                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'q', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_2                                         \
    {                                                                   \
        '2', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_2                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'r', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_3                                         \
    {                                                                   \
        '3', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_3                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 's', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_4                                         \
    {                                                                   \
        '4', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_4                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 't', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_5                                         \
    {                                                                   \
        '5', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_5                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'u', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_6                                         \
    {                                                                   \
        '6', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_6                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'v', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_7                                         \
    {                                                                   \
        '7', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_7                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'w', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_8                                         \
    {                                                                   \
        '8', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_8                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'x', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_9                                         \
    {                                                                   \
        '9', '\0'
#define ZM_CLI_VT100_ALT_9                                             \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'y'                               \
    }
#define ZM_CLI_VT100_NUMERIC_MINUS                                     \
    {                                                                   \
        '-', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_MINUS                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'm', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_COMMA                                     \
    {                                                                   \
        ',', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_COMMA                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'l', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_PERIOD                                    \
    {                                                                   \
        '.', '\0'                                                       \
    }
#define ZM_CLI_VT100_ALT_PERIOD                                        \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'n', '\0'                         \
    }
#define ZM_CLI_VT100_NUMERIC_ENTER                                     \
    {                                                                   \
        ASCII_CR                                                        \
    }
#define ZM_CLI_VT100_ALT_ENTER                                         \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, 'O', 'M', '\0'                         \
    }

#define ZM_CLI_VT100_COLOR(__col)                                            \
    {                                                                         \
        ZM_CLI_VT100_ASCII_ESC, '[', '1', ';', '3', '0' + (__col), 'm', '\0' \
    }
#define ZM_CLI_VT100_BGCOLOR(__col)                                          \
    {                                                                         \
        ZM_CLI_VT100_ASCII_ESC, '[', '4', '0' + (__col), 'm', '\0'           \
    }
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/


typedef void (* zm_printf_fwrite)(void const * p_user_ctx, char const * p_str, size_t length);

/**
 * @brief fprintf context
 */
typedef struct zm_fprintf_ctx
{
    char * const p_io_buffer;       ///< Pointer to IO buffer.
    size_t const io_buffer_size;    ///< IO buffer size.
    size_t io_buffer_cnt;           ///< IO buffer usage.
    bool auto_flush;                ///< Auto flush configurator.

    void const * const p_user_ctx;  ///< Pointer to user data to be passed to the fwrite funciton.
    zm_printf_fwrite fwrite;      ///< Pointer to function sending data stream.
} zm_printf_ctx_t;

/*************************************************************************************************************************
 *                                                  AFTER MACROS                                                         *
 *************************************************************************************************************************/
#define ZM_PRINTF_DEF(name, _p_user_ctx, _p_io_buffer, _io_buffer_size, _auto_flush, _fwrite) \
    static zm_printf_ctx_t name =                                                               \
    {                                                                                           \
        .p_io_buffer = _p_io_buffer,                                                            \
        .io_buffer_size = _io_buffer_size,                                                      \
        .io_buffer_cnt = 0,                                                                     \
        .auto_flush = _auto_flush,                                                              \
        .p_user_ctx = _p_user_ctx,                                                              \
        .fwrite = _fwrite                                                                       \
    }
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
  
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
    
void zm_printf(zm_printf_ctx_t * const p_ctx,
                 char const *              p_fmt,
                                           ...);
void zm_printf_fmt(zm_printf_ctx_t * const p_ctx,
                   char const *               p_fmt,
                   va_list *                  p_args);

#ifdef __cplusplus
}
#endif
#endif /* zm_printf.h */
