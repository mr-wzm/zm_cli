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
    
#define ZM_CLI_VT100_ASCII_ESC     (0x1b)
#define ZM_CLI_VT100_ASCII_DEL     (0x7F)
#define ZM_CLI_VT100_ASCII_BSPACE  (0x08)
#define ZM_CLI_VT100_ASCII_CTRL_A  (0x1)
#define ZM_CLI_VT100_ASCII_CTRL_C  (0x03)
#define ZM_CLI_VT100_ASCII_CTRL_E  (0x5)
#define ZM_CLI_VT100_ASCII_CTRL_L  (0x0C)
#define ZM_CLI_VT100_ASCII_CTRL_U  (0x15)
#define ZM_CLI_VT100_ASCII_CTRL_W  (0x17)

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
    
#define ZM_CLI_VT100_MODESOFF                                          \
    {                                                                   \
        ZM_CLI_VT100_ASCII_ESC, '[', 'm', '\0'                         \
    } /* Turn off character attributes */
#define ZM_CLI_VT100_COLOR(__col)                                            \
    {                                                                         \
        ZM_CLI_VT100_ASCII_ESC, '[', '1', ';', '3', '0' + (__col), 'm', '\0' \
    }
#define ZM_CLI_VT100_BGCOLOR(__col)                                          \
    {                                                                         \
        ZM_CLI_VT100_ASCII_ESC, '[', '4', '0' + (__col), 'm', '\0'           \
    }
/**
 * @brief CLI colors for @ref nrf_cli_fprintf function.
 */
#define ZM_CLI_DEFAULT  ZM_CLI_VT100_COLOR_DEFAULT    /**< Turn off character attributes. */
#define ZM_CLI_NORMAL   ZM_CLI_VT100_COLOR_WHITE      /**< Normal color printf.           */
#define ZM_CLI_INFO     ZM_CLI_VT100_COLOR_GREEN      /**< Info color printf.             */
#define ZM_CLI_OPTION   ZM_CLI_VT100_COLOR_CYAN       /**< Option color printf.           */
#define ZM_CLI_WARNING  ZM_CLI_VT100_COLOR_YELLOW     /**< Warning color printf.          */
#define ZM_CLI_ERROR    ZM_CLI_VT100_COLOR_RED        /**< Error color printf.            */
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
