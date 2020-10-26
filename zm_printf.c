/*****************************************************************
* Copyright (C) 2020 ZM Technology Personal.                     *
******************************************************************
* zm_printf.c
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
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include <string.h>
#include "zm_printf.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                   GLOBAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
static void buffer_add(zm_printf_ctx_t * const p_ctx, char c);
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/



void zm_printf(zm_printf_ctx_t * const p_ctx,
                 char const *              p_fmt,
                                           ...)
{
    ASSERT(p_ctx != NULL);
    ASSERT(p_ctx->fwrite != NULL);
    ASSERT(p_ctx->p_io_buffer != NULL);
    ASSERT(p_ctx->io_buffer_size > 0);

    if (p_fmt == NULL)
    {
        return;
    }

    va_list args = {0};
    va_start(args, p_fmt);

    zm_printf_fmt(p_ctx, p_fmt, &args);

    va_end(args);
}

void zm_printf_buffer_flush(zm_printf_ctx_t * const p_ctx)
{
    ASSERT(p_ctx != NULL);

    if (p_ctx->io_buffer_cnt == 0)
    {
        return;
    }

    p_ctx->fwrite(p_ctx->p_user_ctx,
                  p_ctx->p_io_buffer,
                  p_ctx->io_buffer_cnt);
    p_ctx->io_buffer_cnt = 0;
}

static void fill_space(zm_printf_ctx_t * const p_ctx,
                       uint8_t len,
                       bool zeros)
{
    for (; len > 0; len--)
    {
        if (zeros)
        {
            buffer_add(p_ctx, '0');
        }
        else
        {
            buffer_add(p_ctx, ' ');
        }
    }
}

static void buffer_add(zm_printf_ctx_t * const p_ctx, char c)
{
#ifdef ZM_FPRINTF_FLAG_AUTOMATIC_CR_ON_LF
    if (c == '\n')
    {
        buffer_add(p_ctx, '\r');
    }
#endif
    p_ctx->p_io_buffer[p_ctx->io_buffer_cnt++] = c;

    if (p_ctx->io_buffer_cnt >= p_ctx->io_buffer_size)
    {
        zm_printf_buffer_flush(p_ctx);
    }
}

static void unsigned_print(zm_printf_ctx_t * const p_ctx,
                           uint32_t                  v,
                           uint32_t                  Base,
                           uint32_t                  NumDigits,
                           uint32_t                  FieldWidth,
                           uint32_t                  FormatFlags)
{
    static const char _aV2C[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                   'A', 'B', 'C', 'D', 'E', 'F' };
    uint32_t Div;
    uint32_t Value;
    uint32_t Width;
    char c;

    Value = v;
    //
    // Get actual field width
    //
    Width = 1u;
    while (Value >= Base)
    {
        Value = (Value / Base);
        Width++;
    }
    if (NumDigits > Width)
    {
        Width = NumDigits;
    }
    //
    // Print leading chars if necessary
    //
    if ((FormatFlags & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY) == 0u)
    {
        if (FieldWidth != 0u)
        {
            if (((FormatFlags & ZM_CLI_FORMAT_FLAG_PAD_ZERO) == ZM_CLI_FORMAT_FLAG_PAD_ZERO) &&
                (NumDigits == 0u))
            {
                c = '0';
            }
            else
            {
                c = ' ';
            }
            while ((FieldWidth != 0u) && (Width < FieldWidth))
            {
                FieldWidth--;
                buffer_add(p_ctx, c);
            }
        }
    }

    Value = 1;
    /*
     * Compute Digit.
     * Loop until Digit has the value of the highest digit required.
     * Example: If the output is 345 (Base 10), loop 2 times until Digit is 100.
     */
    while (1)
    {
        /* User specified a min number of digits to print? => Make sure we loop at least that
         * often, before checking anything else (> 1 check avoids problems with NumDigits
         * being signed / unsigned)
         */
        if (NumDigits > 1u)
        {
            NumDigits--;
        }
        else
        {
            Div = v / Value;
            // Is our divider big enough to extract the highest digit from value? => Done
            if (Div < Base)
            {
                break;
            }
        }
        Value *= Base;
    }
    //
    // Output digits
    //
    do
    {
        Div = v / Value;
        v -= Div * Value;
        buffer_add(p_ctx, _aV2C[Div]);
        Value /= Base;
    } while (Value);
    //
    // Print trailing spaces if necessary
    //
    if ((FormatFlags & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY) == ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY)
    {
        if (FieldWidth != 0u)
        {
            while ((FieldWidth != 0u) && (Width < FieldWidth))
            {
                FieldWidth--;
                buffer_add(p_ctx, ' ');
            }
        }
    }
}


static void int_print(zm_printf_ctx_t * const p_ctx,
                      int32_t                   v,
                      uint32_t                  Base,
                      uint32_t                  NumDigits,
                      uint32_t                  FieldWidth,
                      uint32_t                  FormatFlags)
{
    uint32_t Width;
    int32_t Number;

    Number = (v < 0) ? -v : v;

    //
    // Get actual field width
    //
    Width = 1u;
    while (Number >= (int32_t)Base)
    {
        Number = (Number / (int32_t)Base);
        Width++;
    }
    if (NumDigits > Width)
    {
        Width = NumDigits;
    }
    if ((FieldWidth > 0u) && ((v < 0) ||
        ((FormatFlags & ZM_CLI_FORMAT_FLAG_PRINT_SIGN) == ZM_CLI_FORMAT_FLAG_PRINT_SIGN)))
    {
        FieldWidth--;
    }
    //
    // Print leading spaces if necessary
    //
    if ((((FormatFlags & ZM_CLI_FORMAT_FLAG_PAD_ZERO) == 0u) || (NumDigits != 0u)) &&
        ((FormatFlags & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY) == 0u))
    {
        if (FieldWidth != 0u)
        {
            while ((FieldWidth != 0u) && (Width < FieldWidth))
            {
                FieldWidth--;
                buffer_add(p_ctx, ' ');
            }
        }
    }
    //
    // Print sign if necessary
    //
    if (v < 0)
    {
        v = -v;
        buffer_add(p_ctx, '-');
    }
    else if ((FormatFlags & ZM_CLI_FORMAT_FLAG_PRINT_SIGN) == ZM_CLI_FORMAT_FLAG_PRINT_SIGN)
    {
        buffer_add(p_ctx, '+');
    }
    else
    {
        /* do nothing */
    }
    //
    // Print leading zeros if necessary
    //
    if (((FormatFlags & ZM_CLI_FORMAT_FLAG_PAD_ZERO) == ZM_CLI_FORMAT_FLAG_PAD_ZERO) &&
        ((FormatFlags & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY) == 0u) && (NumDigits == 0u))
    {
        if (FieldWidth != 0u)
        {
            while ((FieldWidth != 0u) && (Width < FieldWidth))
            {
                FieldWidth--;
                buffer_add(p_ctx, '0');
            }
        }
    }
    //
    // Print number without sign
    //
    unsigned_print(p_ctx, (uint32_t)v, Base, NumDigits, FieldWidth, FormatFlags);
}

static void string_print(zm_printf_ctx_t * const p_ctx,
                         char const *              p_str,
                         uint32_t                  FieldWidth,
                         uint32_t                  FormatFlags)
{
    uint32_t Width = 0;
    char c;

    if ((FormatFlags & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY) == ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY)
    {
        while ((c = *p_str) != '\0')
        {
            p_str++;
            Width++;
            buffer_add(p_ctx, c);
        }

        while ((FieldWidth > Width) && (FieldWidth > 0))
        {
            FieldWidth--;
            buffer_add(p_ctx, ' ');
        }
    }
    else
    {
        if (p_str != 0)
        {
            Width = strlen(p_str);
        }

        while ((FieldWidth > Width) && (FieldWidth > 0))
        {
            FieldWidth--;
            buffer_add(p_ctx, ' ');
        }

        while ((c = *p_str) != '\0')
        {
            p_str++;
            Width++;
            buffer_add(p_ctx, c);
        }
    }
}

static void float_print(zm_printf_ctx_t * const p_ctx,
                        double                    v,
                        uint32_t                  digits,
                        uint32_t                  width,
                        uint32_t                  format,
                        bool                      uppercase)
{
    bool sign, transform = false;
    uint64_t num, mant, lead, low, base, res, carry, x, s0, s1, s2, s3, fr;
    int32_t exp;
    uint8_t highest, offset, lead_len = 0, skipped = 0;
    uint8_t precision = digits ? digits + 1 : ZM_CLI_FORMAT_DOUBLE_DEF_PRECISION + 1;
    /* Default digits should be -1, because 0 could be a requirement, not the default.
     * This should be changed for the whole library.
     */

    if ((v > 0.0) && (v < 1.0))
    {
        v += 1.0;
        transform = true;
    }
    else if ((v > -1.0) && (v < 0.0))
    {
        v -= 1.0;
        transform = true;
    }

    memcpy(&num, &v, sizeof(num));
    sign = ZM_CLI_FORMAT_DOUBLE_SIGN_GET(num);
    exp = ZM_CLI_FORMAT_DOUBLE_EXP_GET(num);
    mant = ZM_CLI_FORMAT_DOUBLE_MANT_GET(num);

    /* Special cases */
    if (exp == ZM_CLI_FORMAT_DOUBLE_EXP_MASK)
    {
        if (width && (!(format & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY)))
        {
            fill_space(p_ctx, width - 3 - ZM_CLI_FORMAT_REQ_SIGN_SPACE(sign, format), false);
        }

        if (sign)
        {
            buffer_add(p_ctx, '-');
        }
        else if (format & ZM_CLI_FORMAT_FLAG_PRINT_SIGN)
        {
            buffer_add(p_ctx, '+');
        }

        if (mant != 0)
        {
            if(uppercase)
            {
                buffer_add(p_ctx, 'N');
                buffer_add(p_ctx, 'A');
                buffer_add(p_ctx, 'N');
            }
            else
            {
                buffer_add(p_ctx, 'n');
                buffer_add(p_ctx, 'a');
                buffer_add(p_ctx, 'n');
            }
        }
        else
        {
            if(uppercase)
            {
                buffer_add(p_ctx, 'I');
                buffer_add(p_ctx, 'N');
                buffer_add(p_ctx, 'F');
            }
            else
            {
                buffer_add(p_ctx, 'i');
                buffer_add(p_ctx, 'n');
                buffer_add(p_ctx, 'f');
            }
        }
        return;
    }

    /* Add leading 1 to mantissa (except 0.0) */
    if ((mant != 0) || (exp != 0))
    {
        mant |= (1ULL << 52);
    }

    /* Convert the exponent */
    exp = exp - 1023;

    /* Whole numbers */
    offset  = 52 - exp;

    if (offset > 64)
    {
        /* Float fraction offset overflow */
        return;
    }

    lead = (mant >> (offset));

    /* Fraction */
    low = mant & (~(lead << offset));

    while (((low & 0x1) == 0) && low > 0)
    {
        low = low >> 1U;
        skipped++;
    }

    highest = (offset - skipped);
    base = 1;

    for(uint8_t i = 0; i < precision; i++)
    {
        base *= 10;
    }

    /* Handle multiplication with possible overflow */
    x = LOW_32(low) * LOW_32(base);
    s0 = LOW_32(x);

    x = HIGH_32(low) * LOW_32(base) + HIGH_32(x);
    s1 = LOW_32(x);
    s2 = HIGH_32(x);

    x = s1 + LOW_32(low) * HIGH_32(base);
    s1 = LOW_32(x);

    x = s2 + HIGH_32(low) * HIGH_32(base) + HIGH_32(x);
    s2 = LOW_32(x);
    s3 = HIGH_32(x);

    res = s1 << 32 | s0;
    carry = s3 << 32 | s2;

    /* Divide and combine */
    carry = carry << (64 - highest);
    res = res >> highest;
    fr = res | carry;

    /* Roundup */
    if (fr%10 >= 5)
    {
        fr /= 10;
        fr++;
    }
    else
    {
        fr /= 10;
    }
    precision--;

    if (transform && (lead == 1))
    {
        lead = 0;
    }

    /* Maximum precision handled by int_print() is 10 */
    if (precision > 10)
    {
        for (uint8_t delta = precision - 10; delta > 0; delta--)
        {
            fr /= 10;
        }
        precision = 10;
    }

    res = lead;
    while (res > 0)
    {
        res /= 10;
        lead_len++;
    }

    if ((lead == 0) && (fr == 0))
    {
        lead_len = 1;
    }

    if(lead_len == 0)
    {
        lead_len = 1;
    }

    if (width && (!(format & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY)))
    {
        int32_t space = width - lead_len - precision - ZM_CLI_FORMAT_REQ_SIGN_SPACE(sign, format) - 1;
        if (space > 0)
        {
            fill_space(p_ctx, space, format & ZM_CLI_FORMAT_FLAG_PAD_ZERO);
        }
    }

    if (sign)
    {
        buffer_add(p_ctx, '-');
    }
    else if (format & ZM_CLI_FORMAT_FLAG_PRINT_SIGN)
    {
        buffer_add(p_ctx, '+');
    }

    int_print(p_ctx,
              lead,
              10u,
              0,
              0,
              0);
    buffer_add(p_ctx, '.');
    int_print(p_ctx,
              fr,
              10u,
              precision,
              0,
              0);

    if (width && (format & ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY))
    {
        int32_t space = width - lead_len - precision - ZM_CLI_FORMAT_REQ_SIGN_SPACE(sign, format) - 1;
        if (space > 0)
        {
            fill_space(p_ctx, space, false);
        }
    }
}

void zm_printf_fmt(zm_printf_ctx_t * const p_ctx,
                   char const *               p_fmt,
                   va_list *                  p_args)
{
    ASSERT(p_ctx != NULL);

    ASSERT(p_ctx->fwrite != NULL);
    ASSERT(p_ctx->p_io_buffer != NULL);
    ASSERT(p_ctx->io_buffer_size > 0);

    if (p_fmt == NULL)
    {
        return;
    }

    char c;
    int32_t v;
    uint32_t NumDigits;
    uint32_t FormatFlags;
    uint32_t FieldWidth;

    do
    {
        c = *p_fmt;
        p_fmt++;

        if (c == 0u)
        {
            break;
        }
        if (c == '%')
        {
            //
            // Filter out flags
            //
            FormatFlags = 0u;
            v = 1;

            do
            {
                c = *p_fmt;
                switch (c)
                {
                    case '-':
                        FormatFlags |= ZM_CLI_FORMAT_FLAG_LEFT_JUSTIFY;
                        p_fmt++;
                        break;
                    case '0':
                        FormatFlags |= ZM_CLI_FORMAT_FLAG_PAD_ZERO;
                        p_fmt++;
                        break;
                    case '+':
                        FormatFlags |= ZM_CLI_FORMAT_FLAG_PRINT_SIGN;
                        p_fmt++;
                        break;
                    default:
                        v = 0;
                        break;
                }
            } while (v);

            //
            // filter out field width
            //
            FieldWidth = 0u;
            do
            {
                if (c == '*')
                {
                    /*lint -save -e64 -e56*/
                    FieldWidth += va_arg(*p_args, unsigned);
                    /*lint -restore*/
                    p_fmt++;
                    break;
                }
                c = *p_fmt;
                if ((c < '0') || (c > '9'))
                {
                    break;
                }
                p_fmt++;
                FieldWidth = (FieldWidth * 10u) + (c - '0');
            } while (1);

            //
            // Filter out precision (number of digits to display)
            //
            NumDigits = 0u;
            c = *p_fmt;
            if (c == '.')
            {
                p_fmt++;
                do
                {
                    c = *p_fmt;
                    if ((c < '0') || (c > '9'))
                    {
                        break;
                    }
                    p_fmt++;
                    NumDigits = NumDigits * 10u + (c - '0');
                } while (1);
            }
            //
            // Filter out length modifier
            //
            c = *p_fmt;
            do
            {
                if ((c == 'l') || (c == 'h'))
                {
                    p_fmt++;
                    c = *p_fmt;
                }
                else
                {
                    break;
                }
            } while (1);
            //
            // Handle specifiers
            //
            /*lint -save -e64*/
            switch (c)
            {
                case 'c':
                {
                    char c0;
                    v = va_arg(*p_args, int32_t);
                    c0 = (char)v;
                    buffer_add(p_ctx, c0);
                    break;
                }
                case 'd':
                case 'i':
                    v = va_arg(*p_args, int32_t);
                    int_print(p_ctx,
                              v,
                              10u,
                              NumDigits,
                              FieldWidth,
                              FormatFlags);
                    break;
                case 'u':
                    v = va_arg(*p_args, int32_t);
                    unsigned_print(p_ctx,
                                   (uint32_t)v,
                                   10u,
                                   NumDigits,
                                   FieldWidth,
                                   FormatFlags);
                    break;
                case 'x':
                case 'X':
                    v = va_arg(*p_args, int32_t);
                    unsigned_print(p_ctx,
                                   (uint32_t)v,
                                   16u,
                                   NumDigits,
                                   FieldWidth,
                                   FormatFlags);
                    break;
                case 's':
                {
                    char const * p_s = va_arg(*p_args, const char *);
                    string_print(p_ctx, p_s, FieldWidth, FormatFlags);
                    break;
                }
                case 'p':
                    v = va_arg(*p_args, int32_t);
                    buffer_add(p_ctx, '0');
                    buffer_add(p_ctx, 'x');
                    unsigned_print(p_ctx, (uint32_t)v, 16u, 8u, 8u, 0);
                    break;
                case '%':
                    buffer_add(p_ctx, '%');
                    break;
#if 1//ZM_MODULE_ENABLED(ZM_FPRINTF_DOUBLE)
                case 'f':
                {
                    double dbl = va_arg(*p_args, double);
                    float_print(p_ctx,
                                dbl,
                                NumDigits,
                                FieldWidth,
                                FormatFlags,
                                false);
                    break;
                }
                case 'F':
                {
                    double dbl = va_arg(*p_args, double);
                    float_print(p_ctx,
                                dbl,
                                NumDigits,
                                FieldWidth,
                                FormatFlags,
                                true);
                    break;
                }
#endif
                default:
                    break;
            }
            /*lint -restore*/
            p_fmt++;
        }
        else
        {
            buffer_add(p_ctx, c);
        }
    } while (*p_fmt != '\0');

    if (p_ctx->auto_flush)
    {
        zm_printf_buffer_flush(p_ctx);
    }
}
/****************************************************** END OF FILE ******************************************************/
