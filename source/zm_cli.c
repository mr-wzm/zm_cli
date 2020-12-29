/*****************************************************************
* Copyright (C) 2020 WangZiming. All rights reserved.            *
******************************************************************
* zm_cli.c
*
* DESCRIPTION:
*     zm_cli.c
* AUTHOR:
*     zm
* CREATED DATE:
*     2020/10/20
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "zm_cli.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
// <q> ZM_CLI_WILDCARD_ENABLED  - Enable wildcard functionality for CLI commands.
#ifndef ZM_CLI_WILDCARD_ENABLED
#define ZM_CLI_WILDCARD_ENABLED ZM_DISABLE
#endif

#define ZM_CLI_INIT_OPTION_PRINTER                  (NULL)

#define ZM_CLI_MAX_TERMINAL_SIZE       (250u)
#define ZM_CLI_CURSOR_POSITION_BUFFER  (10u)   /* 10 == {esc, [, 2, 5, 0, ;, 2, 5, 0, '\0'} */
#define ZM_CLI_DEFAULT_TERMINAL_WIDTH  (80u)   /* Default PuTTY width. */
#define ZM_CLI_DEFAULT_TERMINAL_HEIGHT (24u)   /* Default PuTTY height. */
#define ZM_CLI_INITIAL_CURS_POS        (1u)    /* Initial cursor position is: (1, 1). */

#define ZM_CLI_CMD_BASE_LVL             (0u)

#define ZM_PRINT_VT100_ASCII_ESC     (0x1b)
#define ZM_PRINT_VT100_ASCII_DEL     (0x7F)
#define ZM_PRINT_VT100_ASCII_BSPACE  (0x08)
#define ZM_PRINT_VT100_ASCII_CTRL_A  (0x1)
#define ZM_PRINT_VT100_ASCII_CTRL_C  (0x03)
#define ZM_PRINT_VT100_ASCII_CTRL_E  (0x5)
#define ZM_PRINT_VT100_ASCII_CTRL_L  (0x0C)
#define ZM_PRINT_VT100_ASCII_CTRL_U  (0x15)
#define ZM_PRINT_VT100_ASCII_CTRL_W  (0x17)

/* Macro to send VT100 commands. */
#define ZM_PRINT_VT100_CMD(_p_cli_, _cmd_)   {       \
    ASSERT(_p_cli_);                                \
    ASSERT(_p_cli_->m_printf_ctx->printf_ctx);                 \
    static char const cmd[] = _cmd_;                \
    zm_printf(_p_cli_->m_printf_ctx->printf_ctx, "%s", cmd); \
}

#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)

#define     EOS    '\0'

#define     FNM_NOMATCH      1    /* Match failed. */
#define     FNM_NOSYS        2    /* Function not implemented. */
#define     FNM_NORES        3    /* Out of resources */

#define     FNM_NOESCAPE     0x01    /* Disable backslash escaping. */
#define     FNM_PATHNAME     0x02    /* Slash must be matched by slash. */
#define     FNM_PERIOD       0x04    /* Period must be matched by period. */
#define     FNM_CASEFOLD     0x08    /* Pattern is matched case-insensitive */
#define     FNM_LEADING_DIR  0x10    /* Ignore /<tail> after Imatch. */

#define    FOLDCASE(ch, flags)    foldcase((unsigned char)(ch), (flags))
    
#endif

#define ZM_CLI_MSG_SPECIFY_SUBCOMMAND  "Please specify a subcommand."
#define ZM_CLI_MSG_COMMAND_NOT_FOUND   ": command not found."
#define ZM_CLI_MSG_TAB_OVERFLOWED      "Tab function: commands counter overflowed."


CLI_SECTION_DEF(COMMAND_SECTION_NAME, cli_static_entry_t);
#define CLI_DATA_SECTION_ITEM_GET(i) ZM_SECTION_ITEM_GET(COMMAND_SECTION_NAME, cli_cmd_entry_t, (i))
#define CLI_DATA_SECTION_ITEM_COUNT  ZM_SECTION_ITEM_COUNT(COMMAND_SECTION_NAME, cli_cmd_entry_t)

CLI_SECTION_DEF(PARA_SECTION_NAME, const char);
#define CLI_SORTED_CMD_PTRS_ITEM_GET(i) ZM_SECTION_ITEM_GET(PARA_SECTION_NAME, const char, (i))
#define CLI_SORTED_CMD_PTRS_START_ADDR_GET ZM_SECTION_START_ADDR(PARA_SECTION_NAME)
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)
typedef enum
{
    WILDCARD_CMD_ADDED,
    WILDCARD_CMD_ADDED_MISSING_SPACE,
    WILDCARD_CMD_NO_MATCH_FOUND
} wildcard_cmd_status_t;
#endif
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
static void cli_cmd_init(void);
static void cli_cmd_collect(zm_cli_t const * p_cli);
static inline void  char_insert_echo_off(zm_cli_t const * p_cli, char data);
static void char_insert(zm_cli_t const * p_cli, char data);
static void cli_cmd_buffer_clear(zm_cli_t const * p_cli);
static void cli_state_set(zm_cli_t const * p_cli, cli_state_t state);
static void cli_write(zm_cli_t const *  p_cli,
                      void const *      p_data,
                      size_t            length,
                      size_t *          p_cnt);
static int string_cmp(void const * pp_a, void const * pp_b);
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL FUNCTIONS                                                    *
 *************************************************************************************************************************/
/*****************************************************************
* DESCRIPTION: cli_init
*     
* INPUTS:
*     p_cli : CLI instance internals.
* OUTPUTS:
*     null
* RETURNS:
*     state
* NOTE:
*     null
*****************************************************************/
int zm_cli_init(zm_cli_t const * p_cli)
{
    ASSERT(p_cli);
    
    p_cli->m_cmd_hist->m_hist_head = NULL;
    p_cli->m_cmd_hist->m_hist_tail = NULL;
    p_cli->m_cmd_hist->m_hist_current = NULL;
    p_cli->m_cmd_hist->m_hist_num = 0;
    p_cli->m_ctx->state = ZM_CLI_STATE_INITIALIZED;
    p_cli->m_printf_ctx->printf = zm_cli_printf;
#if ZM_MODULE_ENABLED(ZM_PRINT_VT100_COLORS)
    p_cli->m_ctx->internal.flag.use_colors = true;
#endif
    p_cli->m_printf_ctx->printf_ctx->auto_flush = true;
    p_cli->m_ctx->internal.flag.insert_mode = false;
    p_cli->m_ctx->internal.flag.echo = true;
    p_cli->m_ctx->vt100_ctx.cons.terminal_wid = ZM_CLI_DEFAULT_TERMINAL_WIDTH;
    p_cli->m_ctx->vt100_ctx.cons.terminal_hei = ZM_CLI_DEFAULT_TERMINAL_HEIGHT;

    cli_cmd_init();

    return 0;
}



void zm_cli_process(zm_cli_t const * p_cli)
{
    ASSERT(p_cli);
    
    switch(p_cli->m_ctx->state)
    {
        case ZM_CLI_STATE_UNINITIALIZED:
        case ZM_CLI_STATE_INITIALIZED:
            /* Console initialized but not started. */
            break;
        case ZM_CLI_STATE_ACTIVE:
        {
            cli_cmd_collect(p_cli);
            break;
        }
        
        default:
            break;
    }
}

/* Function shall be only used by the zm_fprintf module. */
void zm_cli_print_stream(void const * p_user_ctx, char const * p_data, size_t data_len)
{
    cli_write((zm_cli_t const *)p_user_ctx,
              p_data,
              data_len,
              NULL);
}

static void cli_cmd_init(void)
{
    const char ** pp_sorted_cmds = CLI_SORTED_CMD_PTRS_START_ADDR_GET;
    for(uint8_t i = 0; i < CLI_DATA_SECTION_ITEM_COUNT; i++)
    {
        const cli_cmd_entry_t * cmd;
        cmd = CLI_DATA_SECTION_ITEM_GET(i);
        /* NULL syntax commands not allowed. */
        ASSERT(cmd);
        ASSERT(cmd->u.m_static_entry->m_syntax);
        pp_sorted_cmds[i] = cmd->u.m_static_entry->m_syntax;
    }
    if (CLI_DATA_SECTION_ITEM_COUNT > 0)
    {
        qsort(pp_sorted_cmds,
              CLI_DATA_SECTION_ITEM_COUNT,
              sizeof (char *),
              string_cmp);
    }
}

/* Function forcing new line - cannot be replaced with function cursor_down_move. */
static inline void cursor_next_line_move(zm_cli_t const * p_cli)
{
    zm_printf(p_cli->m_printf_ctx->printf_ctx, ZM_NEW_LINE);
}

#define ZM_CLI_ASCII_MAX_CHAR (127u)
static inline ret_code_t ascii_filter(char const data)
{
    return (uint8_t)data > ZM_CLI_ASCII_MAX_CHAR ? ZM_ERROR_INVALID_DATA : ZM_SUCCESS;
}

static inline void receive_state_change(zm_cli_t const * p_cli, cli_receive_t state)
{
    p_cli->m_ctx->receive_state = state;
}
static inline void cli_flag_last_nl_set(zm_cli_t const * p_cli, uint8_t val)
{
    p_cli->m_ctx->internal.flag.last_nl = val;
}
static inline uint8_t cli_flag_last_nl_get(zm_cli_t const * p_cli)
{
    return p_cli->m_ctx->internal.flag.last_nl;
}
static inline size_t cli_strlen(char const * str)
{
    return str == NULL ? 0 : strlen(str);
}
/* Function moves cursor left by n positions. */
static inline void cursor_left_move(zm_cli_t const * p_cli, cli_cmd_len_t n)
{
    if (n > 0)
    {
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "\033[%dD", n);
    }
}
/* Function moves cursor right by n positions. */
static inline void cursor_right_move(zm_cli_t const * p_cli, cli_cmd_len_t n)
{
    if (n > 0)
    {
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "\033[%dC", n);
    }
}
/* Function returns true if cursor is at beginning of an empty line. */
static inline bool cursor_in_empty_line(zm_cli_t const * p_cli)
{
    return ( (p_cli->m_ctx->cmd_cur_pos + cli_strlen(p_cli->m_name)) %
              p_cli->m_ctx->vt100_ctx.cons.terminal_wid == 0);
}
/* Function returns true if command length is equal to multiplicity of terminal width. */
static inline bool full_line_cmd(zm_cli_t const * p_cli)
{
    return ((p_cli->m_ctx->cmd_len + cli_strlen(p_cli->m_name)) %
             p_cli->m_ctx->vt100_ctx.cons.terminal_wid == 0);
}
/* Function moves cursor up by n positions. */
static inline void cursor_up_move(zm_cli_t const * p_cli, cli_cmd_len_t n)
{
    if (n > 0)
    {
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "\033[%dA", n);
    }
}
/* Function moves cursor down by n positions but it will bring no effect if cursor is in the last
 * line of terminal screen. In such case, the cursor_next_line_move function shall be invoked. */
static inline void cursor_down_move(zm_cli_t const * p_cli, cli_cmd_len_t n)
{
    if (n > 0)
    {
         zm_printf(p_cli->m_printf_ctx->printf_ctx, "\033[%dB", n);
    }
}
/* Function sends 1 character to the CLI instance. */
static inline void cli_putc(zm_cli_t const * p_cli, char ch)
{
    zm_printf(p_cli->m_printf_ctx->printf_ctx, "%c", ch);
}
/* Function sends VT100 command to save cursor position. */
static inline void cli_cursor_save(zm_cli_t const * p_cli)
{
    ZM_PRINT_VT100_CMD(p_cli, ZM_PRINT_VT100_SAVECURSOR);
}
/* Function sends VT100 command to restore saved cursor position. */
static inline void cli_cursor_restore(zm_cli_t const * p_cli)
{
    ZM_PRINT_VT100_CMD(p_cli, ZM_PRINT_VT100_RESTORECURSOR);
}
/* Function sends VT100 command to clear the screen from cursor position to end of the screen. */
static inline void cli_clear_eos(zm_cli_t const * p_cli)
{
    ZM_PRINT_VT100_CMD(p_cli, ZM_PRINT_VT100_CLEAREOS);
}
static inline bool cli_flag_echo_is_set(zm_cli_t const * p_cli)
{
    return p_cli->m_ctx->internal.flag.echo == 1 ? true : false;
}
static inline void cli_flag_tab_set(zm_cli_t const * p_cli, bool val)
{
    p_cli->m_ctx->internal.flag.tab = val;
}
static inline bool cli_flag_tab_get(zm_cli_t const * p_cli)
{
    return p_cli->m_ctx->internal.flag.tab;
}
static inline void cli_flag_help_set(zm_cli_t const * p_cli)
{
    p_cli->m_ctx->internal.flag.show_help = 1;
}
static inline void cli_flag_help_clear(zm_cli_t const * p_cli)
{
    p_cli->m_ctx->internal.flag.show_help = 0;
}
/* Functions returns true if new line character shall be processed */
static bool process_nl(zm_cli_t const * p_cli, uint8_t data)
{
    if(data != '\t') cli_flag_tab_set(p_cli, false);
    if ((data != '\r') && (data != '\n'))
    {
        cli_flag_last_nl_set(p_cli, 0);
        return false;
    }

    if ((cli_flag_last_nl_get(p_cli) == 0) ||
        (data == cli_flag_last_nl_get(p_cli)))
    {
        cli_flag_last_nl_set(p_cli, data);
        return true;
    }

    return false;
}
#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY)
static inline void history_mode_exit(zm_cli_t const * p_cli)
{
    p_cli->m_cmd_hist->m_hist_current = NULL;
}
#endif

#if ZM_MODULE_ENABLED(ZM_CLI_METAKEYS)
/* Calculates relative line number of given position in buffer */
static uint32_t cli_line_num_with_buffer_offset_get(zm_cli_t const * p_cli,
                                                    cli_cmd_len_t buffer_pos)
{
    uint32_t name_len;
    zm_cli_multiline_cons_t *p_cons = &p_cli->m_ctx->vt100_ctx.cons;

    name_len = cli_strlen(p_cli->m_name);

    return ((buffer_pos + name_len) / p_cons->terminal_wid);
}
/* Calculates column number of given position in buffer */
static uint32_t cli_col_num_with_buffer_offset_get(zm_cli_t const * p_cli,
                                                   cli_cmd_len_t buffer_pos)
{
    uint32_t name_len;
    zm_cli_multiline_cons_t *p_cons = &p_cli->m_ctx->vt100_ctx.cons;

    name_len = cli_strlen(p_cli->m_name);

    /* columns are counted from 1 */
    return (1 + ((buffer_pos + name_len) % p_cons->terminal_wid));
}
/* For the given buffer, calculates row span between position2 and position1 */
static int32_t cli_row_span_with_buffer_offsets_get(zm_cli_t const * p_cli,
                                                    cli_cmd_len_t offset1,
                                                    cli_cmd_len_t offset2)
{
    return cli_line_num_with_buffer_offset_get(p_cli, offset2)
            - cli_line_num_with_buffer_offset_get(p_cli, offset1);
}

/* For the given buffer, calculates column span between position2 and position 1 */
static int32_t cli_column_span_with_buffer_offsets_get(zm_cli_t const * p_cli,
                                                       cli_cmd_len_t offset1,
                                                       cli_cmd_len_t offset2)
{
    return cli_col_num_with_buffer_offset_get(p_cli, offset2)
            - cli_col_num_with_buffer_offset_get(p_cli, offset1);
}
/* Moves cursor horizontally by a number. Positive is right */
static void cursor_horiz_move(zm_cli_t const * p_cli, int32_t delta)
{
    if (delta > 0)
    {
        cursor_right_move(p_cli, delta);
    }
    else if (delta < 0)
    {
        cursor_left_move(p_cli, -delta);
    }
    else { }
}
/* Moves cursor vertically by a number. Positive is down */
static void cursor_vert_move(zm_cli_t const * p_cli, int32_t delta)
{
    if (delta > 0)
    {
        cursor_down_move(p_cli, delta);
    }
    else if (delta < 0)
    {
        cursor_up_move(p_cli, -delta);
    }
    else { }
}
#endif


/* Function multiline_console_data_check checks the current cursor position (x, y) on terminal screen
 * based on: command length, console name length, and terminal width.
 * Example 1:
 * || - cursor
 *  ----------------------------
 * |console_name $: ||          |
 *  ----------------------------
 * => coordinates are:  cur_x = 17, cur_x_end = 17,
 *                      cur_y = 1,  cur_y_end = 1
 * Example 2:
 *  ----------------------------
 * |console_name $: test command|
 * |showing |e|xample           |
 *  ----------------------------
 * => coordinates are:  cur_x = 9, cur_x_end = 18 (cursor can be one column after 'e')
 * =>                   cur_y = 2, cur_y_end = 2
 * Example 3:
 *  ----------------------------
 * |console_name $: test command|
 * |showing e|x|ample with more |
 * |parameters                  |
 *  ----------------------------
 * => coordinates are:  cur_x = 10, cur_x_end = 11 (cursor can be one column after 's')
 * =>                   cur_y = 2, cur_y_end = 3
 */
static zm_cli_multiline_cons_t const * multiline_console_data_check(zm_cli_t const * p_cli)
{
    cli_ctx_t * m_ctx = p_cli->m_ctx;
    zm_cli_multiline_cons_t * p_cons = &p_cli->m_ctx->vt100_ctx.cons;

    p_cons->name_len = cli_strlen(p_cli->m_name);

    /* Current cursor position in command.
     * +1 -> because home position is (1, 1) */
    p_cons->cur_x = (m_ctx->cmd_cur_pos + p_cons->name_len) % p_cons->terminal_wid + 1;
    p_cons->cur_y = (m_ctx->cmd_cur_pos + p_cons->name_len) / p_cons->terminal_wid + 1;

    /* Extreme position when cursor is at the end of command. */
    p_cons->cur_y_end = (m_ctx->cmd_len + p_cons->name_len) / p_cons->terminal_wid + 1;
    p_cons->cur_x_end = (m_ctx->cmd_len + p_cons->name_len) % p_cons->terminal_wid + 1;

    return p_cons;
}

static void right_arrow_handle(zm_cli_t const * p_cli)
{
    zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);

    if ((p_cons->cur_x == p_cons->cur_x_end) &&
        (p_cons->cur_y == p_cons->cur_y_end))
    {
        return; /* nothing to handle because cursor is in start position */
    }

    if (p_cons->cur_x == p_cons->terminal_wid) /* go to next line */
    {
        cursor_down_move(p_cli, 1);
        cursor_left_move(p_cli, p_cons->terminal_wid);
        ++p_cli->m_ctx->cmd_cur_pos;
    }
    else
    {
        cursor_right_move(p_cli, 1);
        ++p_cli->m_ctx->cmd_cur_pos;
    }
}

static void left_arrow_handle(zm_cli_t const * p_cli)
{
    zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);

    if ((p_cons->cur_x == p_cons->name_len + ZM_CLI_INITIAL_CURS_POS) &&
        (p_cons->cur_y == ZM_CLI_INITIAL_CURS_POS))
    {
        return; /* nothing to handle because cursor is in start position */
    }

    if (p_cons->cur_x == ZM_CLI_INITIAL_CURS_POS)
    {   /* go to previous line */
        cursor_up_move(p_cli, 1);
        cursor_right_move(p_cli, p_cons->terminal_wid);
        --p_cli->m_ctx->cmd_cur_pos;
    }
    else
    {
        cursor_left_move(p_cli, 1);
        --p_cli->m_ctx->cmd_cur_pos;
    }
}
#if ZM_MODULE_ENABLED(ZM_CLI_METAKEYS)
/**
 *  Removes the "word" to the left of the cursor:
 *  - if there are spaces at the cursor position, remove all spaces to the left
 *  - remove the non-spaces (word) until a space is found or a beginning of buffer
 */
static void cli_cmd_word_remove(zm_cli_t const * p_cli)
{
    cli_cmd_len_t new_pos;
    cli_cmd_len_t chars_to_delete;
    int32_t row_span;
    int32_t col_span;

    /* Line must not be empty and cursor must not be at 0 to continue */
    if ((p_cli->m_ctx->cmd_len == 0) || (p_cli->m_ctx->cmd_cur_pos == 0))
    {
        return;
    }

    /* start at the current position */
    new_pos = p_cli->m_ctx->cmd_cur_pos;
    chars_to_delete = 0;

    /* look back for all spaces then for non-spaces */
    while ((new_pos >= 1) && (p_cli->m_ctx->cmd_buff[new_pos - 1] == ' '))
    {
        ++chars_to_delete;
        --new_pos;
    }

    while ((new_pos >= 1) && (p_cli->m_ctx->cmd_buff[new_pos - 1] != ' '))
    {
        --new_pos;
        ++chars_to_delete;
    }

    /* calculate the new cursor */
    row_span = cli_row_span_with_buffer_offsets_get(p_cli, p_cli->m_ctx->cmd_cur_pos, new_pos);
    col_span = cli_column_span_with_buffer_offsets_get(p_cli, p_cli->m_ctx->cmd_cur_pos, new_pos);

    /* manage the buffer */
    memmove(&p_cli->m_ctx->cmd_buff[new_pos],
            &p_cli->m_ctx->cmd_buff[new_pos + chars_to_delete],
            p_cli->m_ctx->cmd_len - chars_to_delete);
    p_cli->m_ctx->cmd_len -= chars_to_delete;
    p_cli->m_ctx->cmd_cur_pos = new_pos;
    p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_len] = '\0';

    /* update display */
    cursor_horiz_move(p_cli, col_span);
    cursor_vert_move(p_cli, row_span);
    cli_cursor_save(p_cli);
    zm_cli_printf(p_cli,
            CLI_DEFAULT_COLOR,
            "%s",
            &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
    cli_clear_eos(p_cli);
    cli_cursor_restore(p_cli);
}
#endif

#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY) || ZM_MODULE_ENABLED(ZM_CLI_METAKEYS)
/* Function moves cursor to begin of command position, just after console name. */
static void cursor_home_position_move(zm_cli_t const * p_cli)
{
    zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);

    if ((p_cons->cur_x == p_cons->name_len + ZM_CLI_INITIAL_CURS_POS) &&
        (p_cons->cur_y == ZM_CLI_INITIAL_CURS_POS))
    {
        return; /* nothing to handle because cursor is in start position */
    }

    if (p_cons->cur_y > ZM_CLI_INITIAL_CURS_POS)
    {
        cursor_up_move(p_cli, p_cons->cur_y - ZM_CLI_INITIAL_CURS_POS);
    }

    if (p_cons->cur_x > p_cons->name_len)
    {
        cursor_left_move(p_cli, p_cons->cur_x - ZM_CLI_INITIAL_CURS_POS - p_cons->name_len);
    }
    else
    {
        cursor_right_move(p_cli, p_cons->name_len + ZM_CLI_INITIAL_CURS_POS - p_cons->cur_x);
    }
    /* align data buffer pointer with cursor position */
    p_cli->m_ctx->cmd_cur_pos = 0;
}
#endif
/* Function moves cursor to end of command. */
static void cursor_end_position_move(zm_cli_t const * p_cli)
{
    zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);

    if ((p_cons->cur_x == p_cons->cur_x_end) && (p_cons->cur_y == p_cons->cur_y_end))
    {
        return; /* nothing to handle because cursor is in end position */
    }

    if (p_cons->cur_y_end > p_cons->cur_y)
    {
        cursor_down_move(p_cli, p_cons->cur_y_end - p_cons->cur_y);
    }

    if (p_cons->cur_x > p_cons->cur_x_end)
    {
        cursor_left_move(p_cli, p_cons->cur_x - p_cons->cur_x_end);
    }
    else
    {
        cursor_right_move(p_cli, p_cons->cur_x_end - p_cons->cur_x);
    }
    /* align data buffer pointer with cursor position */
    p_cli->m_ctx->cmd_cur_pos = p_cli->m_ctx->cmd_len;
}
/* Function will move cursor back to position == cmd_cur_pos. Example usage is when cursor needs
 * to be moved back after printing some text. This function cannot be used to move cursor to new
 * location by manual change of cmd_cur_pos.*/
static void cursor_position_synchronize(zm_cli_t const * p_cli)
{
    zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);
    bool last_line = p_cons->cur_y == p_cons->cur_y_end ? true : false;

    /* In case cursor reaches the bottom line of a terminal, it will be moved to the next line. */
    if (cursor_in_empty_line(p_cli) || full_line_cmd(p_cli))
    {
        cursor_next_line_move(p_cli);
    }

    if (last_line)
    {
        cursor_left_move(p_cli, p_cons->cur_x_end - p_cons->cur_x);
    }
    else
    {
        cursor_up_move(p_cli, p_cons->cur_y_end - p_cons->cur_y);
        if (p_cons->cur_x > p_cons->cur_x_end)
        {
            cursor_right_move(p_cli, p_cons->cur_x - p_cons->cur_x_end);
        }
        else
        {
            cursor_left_move(p_cli, p_cons->cur_x_end - p_cons->cur_x);
        }
    }
}
/* Function increments cursor position (if possible) and moves cursor to new line if necessary. */
static void cursor_position_increment(zm_cli_t const * p_cli)
{
    if (p_cli->m_ctx->cmd_cur_pos >= p_cli->m_ctx->cmd_len)
    {
        return; /* incrementation not possible */
    }

    zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);
    cli_cmd_len_t cur_y = p_cons->cur_y;
    ++p_cli->m_ctx->cmd_cur_pos;
    p_cons = multiline_console_data_check(p_cli);

    if (cur_y == p_cons->cur_y)
    {
        cursor_right_move(p_cli, 1);
    }
    else
    {
        cursor_next_line_move(p_cli);
    }
}
static void char_backspace(zm_cli_t const * p_cli)
{
    cli_cmd_len_t diff;

    if ((p_cli->m_ctx->cmd_len == 0) || (p_cli->m_ctx->cmd_cur_pos == 0))
    {
        return;
    }

    diff = p_cli->m_ctx->cmd_len - p_cli->m_ctx->cmd_cur_pos;

    memmove(&p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos - 1],
            &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos],
            diff + 1);

    --p_cli->m_ctx->cmd_cur_pos;
    --p_cli->m_ctx->cmd_len;

    if (diff > 0)
    {
        cli_putc(p_cli, ZM_PRINT_VT100_ASCII_BSPACE);

        zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);
        bool last_line = p_cons->cur_y == p_cons->cur_y_end ? true : false;

        if (last_line)
        {
            zm_cli_printf(p_cli,
                          CLI_DEFAULT_COLOR,
                          "%s",
                          &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
            cli_clear_eos(p_cli);
            cursor_left_move(p_cli, diff);
        }
        else
        {
            /* If cursor is not in last cmd line, its position needs to be saved by
             * VT100 command. */
            cli_cursor_save(p_cli);
            cli_clear_eos(p_cli);
            zm_cli_printf(p_cli,
                           CLI_DEFAULT_COLOR,
                           "%s",
                           &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
            cli_cursor_restore(p_cli);
        }
    }
    else
    {
        static char const cmd_bspace[] = {
            ZM_PRINT_VT100_ASCII_BSPACE, ' ', ZM_PRINT_VT100_ASCII_BSPACE, '\0'};
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "%s", cmd_bspace);
    }
}

static void char_delete(zm_cli_t const * p_cli)
{
    cli_cmd_len_t diff;

    diff = p_cli->m_ctx->cmd_len - p_cli->m_ctx->cmd_cur_pos;

    if (diff == 0)
    {
        return;
    }

    memmove(&p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos],
            &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos + 1],
            diff);

    --p_cli->m_ctx->cmd_len;

    zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);
    bool last_line = p_cons->cur_y == p_cons->cur_y_end ? true : false;

    /* If last line of command is edited, there is no need for saving cursor position. */
    if (last_line)
    {
        zm_cli_printf(p_cli,
                        CLI_DEFAULT_COLOR,
                        "%s",
                        &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
        ZM_PRINT_VT100_CMD(p_cli, ZM_PRINT_VT100_CLEAREOL);
        cursor_left_move(p_cli, --diff);
    }
    else
    {
        cli_cursor_save(p_cli);
        cli_clear_eos(p_cli);
        zm_cli_printf(p_cli,
                        CLI_DEFAULT_COLOR,
                        "%s",
                        &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
        cli_cursor_restore(p_cli);
    }
}

#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY)
__attribute__((weak)) void * cli_malloc(size_t size)
{
    if(size == 0) return NULL;
    return malloc(size);
}

__attribute__((weak)) void cli_free(void *p)
{
    if(p) free(p);
}

static void history_save(zm_cli_t const * p_cli)
{
    cli_cmd_len_t cmd_new_len = cli_strlen(p_cli->m_ctx->cmd_buff);

    history_mode_exit(p_cli);

    if (cmd_new_len == 0)
    {
        return;
    }
    if(p_cli->m_cmd_hist->m_hist_num)
    {
        if(strcmp(p_cli->m_cmd_hist->m_hist_tail->m_cmd, p_cli->m_ctx->cmd_buff) == 0)
        {
            return;
        }
    }
    cli_hist_pool_t * new_hist = (cli_hist_pool_t *)p_cli->m_cmd_hist->m_memory->malloc(sizeof(cli_hist_pool_t));
    if(new_hist)
    {
        new_hist->m_cmd = (char *)p_cli->m_cmd_hist->m_memory->malloc(cmd_new_len + 1);
        if(new_hist->m_cmd)
        {
            strcpy(new_hist->m_cmd, p_cli->m_ctx->cmd_buff);
            new_hist->cmd_len = cmd_new_len;
            new_hist->m_next_hist = NULL;
            new_hist->m_last_hist = p_cli->m_cmd_hist->m_hist_tail;
            if(p_cli->m_cmd_hist->m_hist_num < ZM_CLI_HISTORY_SAVE_ITEM_NUM)
            {
                if(p_cli->m_cmd_hist->m_hist_head == NULL)
                {
                    p_cli->m_cmd_hist->m_hist_head = new_hist;
                }
                else p_cli->m_cmd_hist->m_hist_tail->m_next_hist = new_hist;
                p_cli->m_cmd_hist->m_hist_num++;
            }
            else
            {
                p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_head;
                p_cli->m_cmd_hist->m_hist_head = p_cli->m_cmd_hist->m_hist_head->m_next_hist;
                p_cli->m_cmd_hist->m_hist_head->m_last_hist = NULL;
                p_cli->m_cmd_hist->m_hist_tail->m_next_hist = new_hist;
                p_cli->m_cmd_hist->m_memory->free(p_cli->m_cmd_hist->m_hist_current->m_cmd);
                p_cli->m_cmd_hist->m_memory->free(p_cli->m_cmd_hist->m_hist_current);
                p_cli->m_cmd_hist->m_hist_current = NULL;
            }
            p_cli->m_cmd_hist->m_hist_tail = new_hist;
        }
        else
        {
            p_cli->m_cmd_hist->m_memory->free(new_hist);
        }
    }
}

static void history_handle(zm_cli_t const * p_cli, bool up)
{
    if(p_cli->m_cmd_hist->m_hist_num == 0) return;
    cli_cmd_len_t current_cmd_len;
    bool skip = false;
    cli_hist_pool_t temp_node;
        
    if(up)
    {
        cursor_home_position_move(p_cli);
        if(p_cli->m_cmd_hist->m_hist_current == NULL)
        {
            current_cmd_len = cli_strlen(p_cli->m_ctx->cmd_buff);
            p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_tail;
            if(current_cmd_len > 0)
            {
                strcpy(p_cli->m_ctx->temp_buff, p_cli->m_ctx->cmd_buff);
            }
            else
            {
                p_cli->m_ctx->temp_buff[0] = '\0';
            }
        }
        else if(p_cli->m_cmd_hist->m_hist_current->m_last_hist)
        {
            current_cmd_len = p_cli->m_cmd_hist->m_hist_current->cmd_len;
            p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_current->m_last_hist;
        }
    }
    else
    {
        if(p_cli->m_cmd_hist->m_hist_current == NULL) return;
        
        cursor_home_position_move(p_cli);
        current_cmd_len = p_cli->m_ctx->cmd_len;
        
        p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_current->m_next_hist;
        if(p_cli->m_cmd_hist->m_hist_current == NULL)
        {
            if(cli_strlen(p_cli->m_ctx->temp_buff) > 0)
            {
                strcpy(p_cli->m_ctx->cmd_buff, p_cli->m_ctx->temp_buff);
            }
            else
            {
                p_cli->m_ctx->cmd_buff[0] = '\0';
            }
            temp_node.cmd_len = cli_strlen(p_cli->m_ctx->cmd_buff);
            skip = true;
        }
    }
    if(!skip)
    {
        strcpy(p_cli->m_ctx->cmd_buff, p_cli->m_cmd_hist->m_hist_current->m_cmd);
        temp_node.cmd_len = p_cli->m_cmd_hist->m_hist_current->cmd_len;
    }
    p_cli->m_ctx->cmd_cur_pos = temp_node.cmd_len;
    p_cli->m_ctx->cmd_len = temp_node.cmd_len;
    
    if(current_cmd_len > temp_node.cmd_len)
    {
        cli_clear_eos(p_cli);
    }
    zm_cli_printf(p_cli, CLI_DEFAULT_COLOR, "%s", p_cli->m_ctx->cmd_buff);
    if (cursor_in_empty_line(p_cli) || full_line_cmd(p_cli))
    {
        cursor_next_line_move(p_cli);
    }
}
#endif

/* Function cmd_get shall be used to search commands. It moves the pointer pp_entry to command
 * of static command structure. If the command cannot be found, the function will set pp_entry to NULL.
 *   p_command   Pointer to command which will be processed (no matter the root command).
 *   lvl         Level of the requested command.
 *   idx         Index of the requested command.
 *   pp_entry    Pointer which points to subcommand[idx] after function execution.
 *   p_st_entry  Pointer to the structure where dynamic entry data can be stored.
 */
static void cmd_get(cli_cmd_entry_t const *     p_command,
                    size_t                          lvl,
                    size_t                          idx,
                    cli_static_entry_t const ** pp_entry,
                    cli_static_entry_t *        p_st_entry)
{
    ASSERT (pp_entry != NULL);
    ASSERT (p_st_entry != NULL);

    if (lvl == ZM_CLI_CMD_BASE_LVL)
    {
        if (idx < CLI_DATA_SECTION_ITEM_COUNT)
        {
            cli_cmd_entry_t const * p_cmd = NULL;
            char const * * pp_sorted_cmds = (char const * *)CLI_SORTED_CMD_PTRS_START_ADDR_GET;
            for (size_t i = 0; i < CLI_DATA_SECTION_ITEM_COUNT; i++)
            {
                p_cmd = CLI_DATA_SECTION_ITEM_GET(i);
                if (!strcmp(pp_sorted_cmds[idx], p_cmd->u.m_static_entry->m_syntax))
                {
                    *pp_entry = p_cmd->u.m_static_entry;
                    return;
                }
            }
        }
        *pp_entry = NULL;
        return;
    }

    if (p_command == NULL)
    {
        *pp_entry = NULL;
        return;
    }

    if (p_command->is_dynamic)
    {
        p_command->u.p_dynamic_get(idx, p_st_entry);
        if (p_st_entry->m_syntax == NULL)
        {
            *pp_entry = NULL;
        }
        else
        {
            *pp_entry = p_st_entry;
        }
    }
    else
    {
        if (p_command->u.m_static_entry[idx].m_syntax != NULL)
        {
            *pp_entry = &p_command->u.m_static_entry[idx];
        }
        else
        {
            *pp_entry = NULL;
        }
    }
}


/* Function remove white chars from beginning and end of command buffer. */
static void cmd_trim(zm_cli_t const * p_cli)
{
    cli_cmd_len_t i = 0;

    if (p_cli->m_ctx->cmd_buff[0] == '\0') /* no command in the buffer */
    {
        return;
    }

    /* Counting white characters starting from beginning of the command. */
    while (isspace((int)p_cli->m_ctx->cmd_buff[i++]))
    {
        if (i == 0)
        {
            p_cli->m_ctx->cmd_buff[0] = '\0';
            return;
        }
    }

    /* Removing counted white characters. */
    if (--i > 0)
    {
        memmove(p_cli->m_ctx->cmd_buff,
                p_cli->m_ctx->cmd_buff + i,
                (p_cli->m_ctx->cmd_len + 1) - i); /* +1 for '\0' */
        p_cli->m_ctx->cmd_len = p_cli->m_ctx->cmd_len - i;
        p_cli->m_ctx->cmd_cur_pos = p_cli->m_ctx->cmd_len;
    }

    /* Counting white characters starting from end of command. */
    char * p_end = &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_len - 1];
    i = 0;
    while (isspace((int)*p_end))
    {
        ++i;
        --p_end;
    }

    /* Removing counted white characters. */
    if (p_end != &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_len - 1])
    {
        p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_len - i] = '\0';
        p_cli->m_ctx->cmd_len = p_cli->m_ctx->cmd_len - i;
        p_cli->m_ctx->cmd_cur_pos = p_cli->m_ctx->cmd_len;
    }
}

static char make_argv(size_t * p_argc, char ** pp_argv, char * p_cmd, uint8_t max_argc)
{
    char c;
    char quote = 0;

    *p_argc = 0;
    do
    {
        c = *p_cmd;
        if (c == '\0')
        {
            break;
        }

        if (isspace((int)c))
        {
            *p_cmd++ = '\0';
            continue;
        }

        pp_argv[(*p_argc)++] = p_cmd;
        quote = 0;

        while (1)
        {
            c = *p_cmd;

            if (c == '\0')
            {
                break;
            }

            if (!quote)
            {
                switch (c)
                {
                    case '\\':
                        memmove(p_cmd, p_cmd + 1, cli_strlen(p_cmd));
                        p_cmd += 1;
                        continue;

                    case '\'':
                    case '\"':
                        memmove(p_cmd, p_cmd + 1, cli_strlen(p_cmd));
                        quote = c;
                        continue;
                    default:
                        break;
                }
            }

            if (quote == c)
            {
                memmove(p_cmd, p_cmd + 1, cli_strlen(p_cmd));
                quote = 0;
                continue;
            }

            if (quote && c == '\\')
            {
                char t = *(p_cmd + 1);

                if (t == quote)
                {
                    memmove(p_cmd, p_cmd + 1, cli_strlen(p_cmd));
                    p_cmd += 1;
                    continue;
                }

                if (t == '0')
                {
                    uint8_t i;
                    uint8_t v = 0;

                    for (i = 2; i < (2 + 3); i++)
                    {
                        t = *(p_cmd + i);

                        if (t >= '0' && t <= '7')
                        {
                            v = (v << 3) | (t - '0');
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (i > 2)
                    {
                        memmove(p_cmd, p_cmd + (i - 1), cli_strlen(p_cmd) - (i - 2));
                        *p_cmd++ = v;
                        continue;
                    }
                }

                if (t == 'x')
                {
                    uint8_t i;
                    uint8_t v = 0;

                    for (i = 2; i < (2 + 2); i++)
                    {
                        t = *(p_cmd + i);

                        if (t >= '0' && t <= '9')
                        {
                            v = (v << 4) | (t - '0');
                        }
                        else if (t >= 'a' && t <= 'f')
                        {
                            v = (v << 4) | (t - 'a' + 10);
                        }
                        else if (t >= 'A' && t <= 'F')
                        {
                            v = (v << 4) | (t - 'A' + 10);
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (i > 2)
                    {
                        memmove(p_cmd, p_cmd + (i - 1), cli_strlen(p_cmd) - (i - 2));
                        *p_cmd++ = v;
                        continue;
                    }
                }
            }
            if (!quote && isspace((int)c))
            {
                break;
            }

            p_cmd += 1;
        }
    } while (*p_argc < max_argc);

    ASSERT(*p_argc <= ZM_CLI_ARGC_MAX);
    pp_argv[*p_argc] = 0;

    return quote;

}
/* Function checks how many identical characters have two strings starting from the first character. */
static cli_cmd_len_t str_similarity_check(char const * str_a, char const * str_b)
{
    cli_cmd_len_t cnt = 0;

    while (str_a[cnt] != '\0')
    {
        if (str_a[cnt] != str_b[cnt])
        {
            return cnt;
        }

        if (++cnt == 0)
        {
            return --cnt; /* too long strings */
        }
    }
    return cnt;
}
static void completion_insert(zm_cli_t const * p_cli,
                              char const *      p_compl,
                              cli_cmd_len_t compl_len)
{
    ASSERT (p_compl);

    cli_cmd_len_t diff = p_cli->m_ctx->cmd_len - p_cli->m_ctx->cmd_cur_pos;

    if ((p_cli->m_ctx->cmd_len + compl_len > ZM_CLI_CMD_BUFF_SIZE - 1) ||
        (compl_len == 0))
    {
        return;
    }

    /* Make space for completion. */
    memmove(&p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos + compl_len],
            &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos],
            diff + 1); /* + 1 for '\0' */

    /* Insert completion. */
    memmove(&p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos],
            p_compl,
            compl_len);

    p_cli->m_ctx->cmd_len = cli_strlen(p_cli->m_ctx->cmd_buff);
    zm_cli_printf(p_cli,
                    CLI_DEFAULT_COLOR,
                    "%s",
                    &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
    p_cli->m_ctx->cmd_cur_pos += compl_len;

    if (cursor_in_empty_line(p_cli) || full_line_cmd(p_cli))
    {
        cursor_next_line_move(p_cli);
    }

    if (diff > 0)
    {
        cursor_position_synchronize(p_cli);
    }
}
#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)


static inline int foldcase(int ch, int flags)
{

    if ((flags & FNM_CASEFOLD) != 0 && isupper(ch))
        return tolower(ch);
    return ch;
}

static const char * rangematch(const char *pattern, int test, int flags)
{
    int negate, ok, need;
    char c, c2;

    if (pattern == NULL)
    {
        return NULL;
    }

    /*
     * A bracket expression starting with an unquoted circumflex
     * character produces unspecified results (IEEE 1003.2-1992,
     * 3.13.2).  This implementation treats it like '!', for
     * consistency with the regular expression syntax.
     * J.T. Conklin (conklin@ngai.kaleida.com)
     */
    if ((negate = (*pattern == '!' || *pattern == '^')) != 0)
        ++pattern;
    
    need = 1;
    for (ok = 0; (c = FOLDCASE(*pattern++, flags)) != ']' || need;) {
        need = 0;
        if (c == '/')
            return (void *)-1;
        if (c == '\\' && !(flags & FNM_NOESCAPE))
            c = FOLDCASE(*pattern++, flags);
        if (c == EOS)
            return NULL;
        if (*pattern == '-' 
            && (c2 = FOLDCASE(*(pattern + 1), flags)) != EOS &&
                c2 != ']') {
            pattern += 2;
            if (c2 == '\\' && !(flags & FNM_NOESCAPE))
                c2 = FOLDCASE(*pattern++, flags);
            if (c2 == EOS)
                return NULL;
            if (c <= test && test <= c2)
                ok = 1;
        } else if (c == test)
            ok = 1;
    }
    return ok == negate ? NULL : pattern;
}


static int fnmatchx(const char *pattern, const char *string, int flags, size_t recursion)
{
    const char *stringstart, *r;
    char c, test;

    if ((pattern == NULL) || (string == NULL))
    {
        return FNM_NOMATCH;
    }

    if (recursion-- == 0)
        return FNM_NORES;

    for (stringstart = string;;) {
        switch (c = FOLDCASE(*pattern++, flags)) {
        case EOS:
            if ((flags & FNM_LEADING_DIR) && *string == '/')
                return 0;
            return *string == EOS ? 0 : FNM_NOMATCH;
        case '?':
            if (*string == EOS)
                return FNM_NOMATCH;
            if (*string == '/' && (flags & FNM_PATHNAME))
                return FNM_NOMATCH;
            if (*string == '.' && (flags & FNM_PERIOD) &&
                (string == stringstart ||
                ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
                return FNM_NOMATCH;
            ++string;
            break;
        case '*':
            c = FOLDCASE(*pattern, flags);
            /* Collapse multiple stars. */
            while (c == '*')
                c = FOLDCASE(*++pattern, flags);

            if (*string == '.' && (flags & FNM_PERIOD) &&
                (string == stringstart ||
                ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
                return FNM_NOMATCH;

            /* Optimize for pattern with * at end or before /. */
            if (c == EOS) {
                if (flags & FNM_PATHNAME)
                    return (flags & FNM_LEADING_DIR) ||
                        strchr(string, '/') == NULL ?
                        0 : FNM_NOMATCH;
                else
                    return 0;
            } else if (c == '/' && flags & FNM_PATHNAME) {
                if ((string = strchr(string, '/')) == NULL)
                    return FNM_NOMATCH;
                break;
            }

            /* General case, use recursion. */
            while ((test = FOLDCASE(*string, flags)) != EOS) {
                int e;
                switch ((e = fnmatchx(pattern, string,
                    flags & ~FNM_PERIOD, recursion))) {
                case FNM_NOMATCH:
                    break;
                default:
                    return e;
                }
                if (test == '/' && flags & FNM_PATHNAME)
                    break;
                ++string;
            }
            return FNM_NOMATCH;
        case '[':
            if (*string == EOS)
                return FNM_NOMATCH;
            if (*string == '/' && flags & FNM_PATHNAME)
                return FNM_NOMATCH;
            if ((r = rangematch(pattern,
                FOLDCASE(*string, flags), flags)) == NULL)
                return FNM_NOMATCH;
            if (r == (void *)-1) {
                if (*string != '[')
                    return FNM_NOMATCH;
            } else
                pattern = r;
            ++string;
            break;
        case '\\':
            if (!(flags & FNM_NOESCAPE)) {
                if ((c = FOLDCASE(*pattern++, flags)) == EOS) {
                    c = '\0';
                    --pattern;
                }
            }
            /* FALLTHROUGH */
        default:
            if (c != FOLDCASE(*string++, flags))
                return FNM_NOMATCH;
            break;
        }
    }
    /* NOTREACHED */
}

static int fnmatch(const char *pattern, const char *string, int flags)
{
    return fnmatchx(pattern, string, flags, 64);
}

/* Function returns true if string contains wildcard character: '?' or '*'. */
static bool wildcard_character_exist(char * p_str)
{
    size_t str_len = cli_strlen(p_str);
    for (size_t i = 0; i < str_len; i++)
    {
        if ((p_str[i] == '?') || (p_str[i] == '*'))
        {
            return true;
        }
    }
    return false;
}

static void spaces_trim(char * p_char)
{
    cli_cmd_len_t shift = 0;
    cli_cmd_len_t len = cli_strlen(p_char);

    if (p_char == NULL)
    {
        return;
    }

    for (cli_cmd_len_t i = 0; i < len - 1; i++)
    {
        if (isspace((int)p_char[i]))
        {
            for (cli_cmd_len_t j = i + 1; j < len; j++)
            {
                if (isspace((int)p_char[j]))
                {
                    shift++;
                    continue;
                }
                if (shift > 0)
                {
                    memmove(&p_char[i + 1], &p_char[j], len - shift - i);
                    len -= shift;
                    shift = 0;
                }
                break;
            }
        }
    }
}


/* Adds new command and one space just before pattern */
static bool command_to_tmp_buffer_add(zm_cli_t const * p_cli,
                                      char const *      p_new_cmd,
                                      char const *      p_pattern)
{
    cli_cmd_len_t cmd_len = cli_strlen(p_new_cmd);
    cli_cmd_len_t shift;
    char *            p_cmd_source_addr;

    /* +1 for space */
    if (((size_t)p_cli->m_ctx->cmd_tmp_buff_len + cmd_len + 1) > ZM_CLI_CMD_BUFF_SIZE)
    {
        zm_cli_warn(p_cli,
                     "Command buffer is too short to expand all commands matching "
                     "wildcard pattern");
        return false;
    }

    p_cmd_source_addr = strstr(p_cli->m_ctx->temp_buff, p_pattern);

    if (p_cmd_source_addr == NULL)
    {
        return false;
    }

    shift = cli_strlen(p_cmd_source_addr);

    /* make place for new command:      + 1 for space                 + 1 for EOS */
    memmove(p_cmd_source_addr + cmd_len + 1, p_cmd_source_addr, shift + 1);
    memcpy(p_cmd_source_addr, p_new_cmd, cmd_len);
    p_cmd_source_addr[cmd_len] = ' ';

    p_cli->m_ctx->cmd_tmp_buff_len += cmd_len + 1; // + 1 for space

    return true;
}

/* removes pattern and following space */
static void pattern_from_tmp_buffer_remove(zm_cli_t const * p_cli,
                                           char const *      p_pattern)
{
    size_t shift;
    char * p_pattern_addr = strstr(p_cli->m_ctx->temp_buff, p_pattern);

    cli_cmd_len_t pattern_len = cli_strlen(p_pattern);

    if (p_pattern_addr == NULL)
    {
        return;
    }

    if (p_pattern_addr > p_cli->m_ctx->temp_buff)
    {
        if (*(p_pattern_addr - 1) == ' ')
        {
            pattern_len++;      /* space needs to be removed as well */
            p_pattern_addr--;   /* set pointer to space */
        }
    }

    shift = cli_strlen(p_pattern_addr) - pattern_len + 1; /* +1 for EOS */
    p_cli->m_ctx->cmd_tmp_buff_len -= pattern_len;

    memmove(p_pattern_addr, p_pattern_addr + pattern_len, shift);
}
/**
 * @internal @brief Function for searching and adding commands matching to wildcard pattern.
 *
 * This function is internal to zm_cli module and shall be not called directly.
 *
 * @param[in/out] p_cli         Pointer to the CLI instance.
 * @param[in]     p_cmd         Pointer to command which will be processed
 * @param[in]     cmd_lvl       Command level in the command tree.
 * @param[in]     p_pattern     Pointer to wildcard pattern.
 * @param[out]    p_counter     Number of found and added commands.
 *
 * @retval WILDCARD_CMD_ADDED                   All matching commands added to the buffer.
 * @retval WILDCARD_CMD_ADDED_MISSING_SPACE     Not all matching commands added because
 *                                              ZM_CLI_CMD_BUFF_SIZE is too small.
 * @retval WILDCARD_CMD_NO_MATCH_FOUND          No matching command found.
 */
static wildcard_cmd_status_t commands_expand(zm_cli_t const *           p_cli,
                                             cli_cmd_entry_t const * p_cmd,
                                             size_t                      cmd_lvl,
                                             char *                      p_pattern,
                                             size_t *                    p_counter)
{
    size_t cmd_idx = 0;
    size_t counter = 0;
    bool   success = false;

    cli_static_entry_t         static_entry;
    cli_static_entry_t const * p_static_entry = NULL;
    wildcard_cmd_status_t          ret_val = WILDCARD_CMD_NO_MATCH_FOUND;

    do
    {
        cmd_get(p_cmd,
                cmd_lvl,
                cmd_idx++,
                &p_static_entry,
                &static_entry);

        if (p_static_entry == NULL)
        {
            break;
        }

        if (0 == fnmatch(p_pattern, p_static_entry->m_syntax, 0))
        {
            success = command_to_tmp_buffer_add(p_cli,
                                                p_static_entry->m_syntax,
                                                p_pattern);
            if (!success)
            {
                break;
            }
            counter++;
        }

    } while(cmd_idx != 0);

    if (counter > 0)
    {
        *p_counter = counter;
        pattern_from_tmp_buffer_remove(p_cli, p_pattern);

        if (success)
        {
            ret_val = WILDCARD_CMD_ADDED;
        }
        else
        {
            ret_val = WILDCARD_CMD_ADDED_MISSING_SPACE;
        }
    }

    return ret_val;
}
#endif
static void option_print(zm_cli_t const * p_cli,
                         char const *      p_option,
                         cli_cmd_len_t longest_option)
{
    static char const * tab = "  ";

    /* Function initialization has been requested. */
    if (p_option == NULL)
    {
        p_cli->m_ctx->vt100_ctx.printed_cmd = 0;
        return;
    }
    longest_option += cli_strlen(tab);

    cli_cmd_len_t columns =
        (p_cli->m_ctx->vt100_ctx.cons.terminal_wid - cli_strlen(tab)) / longest_option;
    cli_cmd_len_t diff = longest_option - cli_strlen(p_option);

    if (p_cli->m_ctx->vt100_ctx.printed_cmd++ % columns == 0)
    {
        zm_cli_printf(p_cli, CLI_OPTION_COLOR, "\n%s%s", tab, p_option);
    }
    else
    {
        zm_cli_printf(p_cli, CLI_OPTION_COLOR, "%s", p_option);
    }
    cursor_right_move(p_cli, diff);
}
static inline bool is_completion_candidate(const char *candidate,
                                           const char *str,
                                           size_t len)
{
	return (strncmp(candidate, str, len) == 0) ? true : false;
}

static void cli_tab_handle(zm_cli_t const * p_cli)
{
    size_t cmd_idx;
    size_t cmd_last = 0;
    size_t cmd_first = 0;
    size_t cmd_cnt = 0;

    size_t argc;
    char * argv[ZM_CLI_ARGC_MAX + 1]; /* +1 reserved for NULL in function make_argv */

    cli_cmd_len_t cmd_lvl = ZM_CLI_CMD_BASE_LVL;
    cli_cmd_len_t cmd_longest = 0; /* longest matching command */

    /* Calculating the longest possible completion length. -1 for '\0'. */
    cli_cmd_len_t compl_len = (ZM_CLI_CMD_BUFF_SIZE - 1) - p_cli->m_ctx->cmd_len;

    if (compl_len == 0)
    {
        return;
    }
    
    /* Copy command from its beginning to cursor position. */
    memcpy(p_cli->m_ctx->temp_buff,
           p_cli->m_ctx->cmd_buff,
           p_cli->m_ctx->cmd_cur_pos);

    p_cli->m_ctx->temp_buff[p_cli->m_ctx->cmd_cur_pos] = '\0';
    
    /* Check if the current cursor position points to the 'space' character. */
    bool space = isspace((int)p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos - 1]);

#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY)
    /* If the Tab key is pressed, "history mode" must be terminated because tab and history handlers
       are sharing the same array: temp_buff. */
    history_mode_exit(p_cli);
#endif

    /* Create argument list. */
    (void)make_argv(&argc,
                    &argv[0],
                    p_cli->m_ctx->temp_buff,
                    ZM_CLI_ARGC_MAX);

    cli_cmd_len_t arg_len = cli_strlen(argv[cmd_lvl]);

    /* Variable 'static_entry' is needed to handle dynamic commands. */
    cli_static_entry_t static_entry;

    cli_cmd_entry_t const * p_cmd = NULL;
    cli_static_entry_t const * p_st_cmd = NULL;
    cli_static_entry_t const * p_st_cmd_last = NULL;

    do
    {
        if ((argc == 0) ||
            (cmd_lvl >= argc - 1 + space))
        {
            if (space)
            {
                arg_len = 0;
            }
            else
            {
                arg_len = cli_strlen(argv[cmd_lvl]);
            }

            cmd_idx = 0;

            while (1)
            {
                cmd_get(p_cmd, cmd_lvl, cmd_idx++, &p_st_cmd, &static_entry);

                if (p_st_cmd == NULL)
                {
                    /* No more commands available. */
                    break;
                }

                if (!is_completion_candidate(argv[cmd_lvl],
                                             p_st_cmd->m_syntax,
                                             arg_len))
                {
                    continue;
                }

                cmd_cnt++;

                if (p_st_cmd_last == NULL)
                {
                    cmd_first = cmd_idx - 1;
                    cmd_longest = cli_strlen(p_st_cmd->m_syntax);
                    if (compl_len > (cmd_longest - arg_len))
                    {
                        compl_len = cmd_longest - arg_len;
                    }
                }
                else
                {
                    cli_cmd_len_t len = cli_strlen(p_st_cmd->m_syntax);
                    if (len > cmd_longest)
                    {
                        cmd_longest = len;
                    }

                    if (compl_len > 0)  /* Checking if partial completion is possible */
                    {
                        cli_static_entry_t last_entry;
                        cmd_get(p_cmd, cmd_lvl, cmd_last, &p_st_cmd_last, &last_entry);

                        len = str_similarity_check(p_st_cmd->m_syntax + arg_len,
                                                   p_st_cmd_last->m_syntax + arg_len);
                        if (compl_len > len)
                        {
                            /* Determining the longest possible completion. */
                            compl_len = len;
                        }
                    }
                }
                cmd_last = cmd_idx - 1;
                p_st_cmd_last = p_st_cmd;

                if (cmd_idx == 0) /* Too many possibilities */
                {
                    zm_cli_warn(p_cli, ZM_CLI_MSG_TAB_OVERFLOWED);
                    break;
                }
            }
        }
        else
        {
            cmd_idx = 0;

            while (1)
            {
                cmd_get(p_cmd, cmd_lvl, cmd_idx++, &p_st_cmd, &static_entry);

                if (cmd_idx == 0)
                {
                    /* No match found and commands counter overflowed. */
                    zm_cli_warn(p_cli, ZM_CLI_MSG_TAB_OVERFLOWED);
                    return;
                }

                if (p_st_cmd == NULL) /* No more commands available */
                {
                    return;
                }

#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)
                /* Ignore wildcard character arguments if they are "standalone". Example:
                    1. log enable info b*<tab>  -> "b*"  is treated as a command so no match found
                    2. log enable info b* <tab> -> "b* " is ignored, <tab> will prompt all available
                                                   commands. */
                if (wildcard_character_exist(argv[cmd_lvl]))
                {
                    break;
                }
#endif
                /* Fuction "strcmp" is used because an exact match is required. */
                if (strcmp(argv[cmd_lvl], p_st_cmd->m_syntax) == 0)
                {
                    p_cmd = p_st_cmd->m_subcmd;
                    break;
                }
            }
        }

        if ((p_cmd == NULL) || (p_st_cmd == NULL))
        {
            break;
        }

    } while (++cmd_lvl < argc + space);

    if (cmd_cnt == 0)
    {
        /* No match found. */
        return;
    }

    if (cmd_cnt == 1) /* only one match found */
    {
        if (p_cmd != NULL && p_cmd->is_dynamic)
        {
            /* In case of dynamic entry, function cmd_get shall be called again for matching
             * command index (cmd_last). It is because static_entry is most likely appended by
             * not valid data.
             */
            cmd_get(p_cmd, cmd_lvl, cmd_last, &p_st_cmd_last, &static_entry);
        }
        if (cli_strlen(p_st_cmd_last->m_syntax) != arg_len) /* no exact match found */
        {
            completion_insert(p_cli, p_st_cmd_last->m_syntax + arg_len, compl_len);
        }

        /* Next character in the buffer is not 'space'. */
        if (!isspace((int)p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]))
        {
            if (p_cli->m_ctx->internal.flag.insert_mode)
            {
                p_cli->m_ctx->internal.flag.insert_mode = 0;
                char_insert(p_cli, ' ');
                p_cli->m_ctx->internal.flag.insert_mode = 1;
            }
            else
            {
                char_insert(p_cli, ' ');
            }
        }
        else
        {
            /*  case:
                | | -> cursor
                cons_name $: valid_cmd valid_sub_cmd| |argument  <tab>
             */
            cursor_position_increment(p_cli);
            /* result:
               cons_name $: valid_cmd valid_sub_cmd |a|rgument
             */
        }
        return;
    }
    if(!cli_flag_tab_get(p_cli))
    {
        cli_flag_tab_set(p_cli, true);
        return;
    }
    /* Printing all matching commands (options). */
    option_print(p_cli, ZM_CLI_INIT_OPTION_PRINTER, cmd_longest);
    cmd_idx = cmd_first;
    
    while (cmd_cnt)
    {
        cmd_get(p_cmd, cmd_lvl, cmd_idx++, &p_st_cmd, &static_entry);
        if (!is_completion_candidate(argv[cmd_lvl],
                                     p_st_cmd->m_syntax,
                                     arg_len))
        {
            continue;
        }
        cmd_cnt--;
        option_print(p_cli, p_st_cmd->m_syntax, cmd_longest);
    }

    zm_cli_printf(p_cli, CLI_NAME_COLOR, "\n%s", p_cli->m_name);
    zm_cli_printf(p_cli, CLI_DEFAULT_COLOR, "%s", p_cli->m_ctx->cmd_buff);

    cursor_position_synchronize(p_cli);
    completion_insert(p_cli, p_st_cmd_last->m_syntax + arg_len, compl_len);
}



/* Function is analyzing the command buffer to find matching commands. Next, it invokes the  last recognized
 * command which has a handler and passes the rest of command buffer as arguments. */
static void cli_execute(zm_cli_t const * p_cli)
{
    char quote;
    size_t argc;
    char * argv[ZM_CLI_ARGC_MAX + 1]; /* +1 reserved for NULL added by function make_argv */

    size_t cmd_idx;             /* currently analyzed command in cmd_level */
    size_t cmd_lvl = ZM_CLI_CMD_BASE_LVL; /* currently analyzed command level */
    size_t cmd_handler_lvl = 0; /* last command level for which a handler has been found */

    cli_cmd_entry_t const * p_cmd = NULL;

    cmd_trim(p_cli);

#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY)
    history_save(p_cli);
#endif

#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)
/* Wildcard can be correctly handled under following conditions:
     - wildcard command does not have a handler
     - wildcard command is on the deepest commands level
     - other commands on the same level as wildcard command shall also not have a handler

   Algorithm:
   1. Command buffer is copied to Temp buffer.
   2. Algorithm goes through Command buffer to find handlers and subcommands.
   3. If algorithm will find a wildcard character it switches to Temp buffer.
   4. In the Temp buffer command with found wildcard character is changed into matching command(s).
   5. Algorithm switch back to Command buffer and analyzes next command.
   6. When all arguments are analyzed from Command buffer, Temp buffer is copied to Command buffer.
   7. Last found handler is executed with all arguments in the Command buffer.
*/
    size_t commands_expanded = 0;

    memset(p_cli->m_ctx->temp_buff, 0, sizeof(p_cli->m_ctx->temp_buff));
    memcpy(p_cli->m_ctx->temp_buff,
           p_cli->m_ctx->cmd_buff,
           p_cli->m_ctx->cmd_len);

    /* Function spaces_trim must be used instead of make_argv. At this point it is important to keep
       temp_buff as a one string. It will allow to find wildcard commands easly with strstr
       function. */
    spaces_trim(p_cli->m_ctx->temp_buff);
    p_cli->m_ctx->cmd_tmp_buff_len = cli_strlen(p_cli->m_ctx->temp_buff) + 1;  // +1 for EOS
#endif

    cursor_end_position_move(p_cli);
    if (!cursor_in_empty_line(p_cli))
    {
        cursor_next_line_move(p_cli);
    }

    /* create argument list */
    quote = make_argv(&argc,
                      &argv[0],
                      p_cli->m_ctx->cmd_buff,
                      ZM_CLI_ARGC_MAX);

    if (!argc)
    {
        cursor_next_line_move(p_cli);
        return;
    }

    if (quote != 0)
    {
        zm_cli_error(p_cli, "not terminated: %c", quote);
        return;
    }

    /*  Searching for a matching root command. */
    for (cmd_idx = 0; cmd_idx <= CLI_DATA_SECTION_ITEM_COUNT; ++cmd_idx)
    {
        if (cmd_idx >= CLI_DATA_SECTION_ITEM_COUNT)
        {
            zm_cli_error(p_cli, "%s%s", argv[0], ZM_CLI_MSG_COMMAND_NOT_FOUND);
            return;
        }

        p_cmd = CLI_DATA_SECTION_ITEM_GET(cmd_idx);
        if (strcmp(argv[cmd_lvl], p_cmd->u.m_static_entry->m_syntax) != 0)
        {
            continue;
        }
        break;
    }

    /* Root command shall be always static. */
    ASSERT(p_cmd->is_dynamic == false);

    /* Memory reserved for dynamic commands. */
    cli_static_entry_t static_entry;
    cli_static_entry_t const * p_static_entry = NULL;

    memset(&p_cli->m_ctx->active_cmd, 0, sizeof(p_cli->m_ctx->active_cmd));
    if (p_cmd->u.m_static_entry->m_handler != NULL)
    {
        p_cli->m_ctx->active_cmd = *p_cmd->u.m_static_entry;
    }

    p_cmd = p_cmd->u.m_static_entry->m_subcmd;
    cmd_lvl++;
    cmd_idx = 0;

    while (1)
    {
        if (cmd_lvl >= argc)
        {
            break;
        }

        if (!strcmp(argv[cmd_lvl], "-h") || !strcmp(argv[cmd_lvl], "--help"))
        {
            /* Command called with help option so it makes no sense to search deeper commands. */
            cli_flag_help_set(p_cli);
            break;
        }

#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)
        /* Wildcard character is found */
        if (wildcard_character_exist(argv[cmd_lvl]))
        {
            size_t counter = 0;
            wildcard_cmd_status_t status;

            /* Function will search commands tree for commands matching wildcard pattern stored in
               argv[cmd_lvl]. If match is found wildcard pattern will be replaced by matching
               commands in temp_buffer. If there is no space to add all matching commands function
               will add as many as possible. Next it will continue to search for next wildcard
               pattern and it will try to add matching commands. */
            status = commands_expand(p_cli, p_cmd, cmd_lvl, argv[cmd_lvl], &counter);
            if (WILDCARD_CMD_NO_MATCH_FOUND == status)
            {
                break;
            }
            commands_expanded += counter;
            cmd_lvl++;
            continue;
        }
#endif

        cmd_get(p_cmd,
                cmd_lvl,
                cmd_idx++,
                &p_static_entry,
                &static_entry);

        if ((cmd_idx == 0) || (p_static_entry == NULL))
        {
            break;
        }

        if (strcmp(argv[cmd_lvl], p_static_entry->m_syntax) == 0)
        {
            /* checking if command has a handler */
            if (p_static_entry->m_handler != NULL)
            {
#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)
                if (commands_expanded > 0)
                {
                    cursor_end_position_move(p_cli);
                    if (!cursor_in_empty_line(p_cli))
                    {
                        cursor_next_line_move(p_cli);
                    }
                    /* An error occured, fnmatch argument cannot be followed by argument
                    * with a handler to avoid multiple function calls. */
                    zm_cli_error(p_cli, "Error: requested multiple function executions");
                    cli_flag_help_clear(p_cli);
                    return;
                }
#endif
                /* Storing p_st_cmd->handler is not feasible for dynamic commands. Data will be
                 * invalid with the next loop iteration. */
                cmd_handler_lvl = cmd_lvl;
                p_cli->m_ctx->active_cmd = *p_static_entry;
            }

            cmd_lvl++;
            cmd_idx = 0;
            p_cmd = p_static_entry->m_subcmd;
        }
    }
#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)
    if (commands_expanded > 0)
    {
        /* Copy temp_buff to cmd_buff */
        memcpy(p_cli->m_ctx->cmd_buff,
               p_cli->m_ctx->temp_buff,
               p_cli->m_ctx->cmd_tmp_buff_len);
        p_cli->m_ctx->cmd_len = p_cli->m_ctx->cmd_tmp_buff_len;

        /* calling make_arg function again because cmd_buffer has additional commads */
        (void)make_argv(&argc,
                        &argv[0],
                        p_cli->m_ctx->cmd_buff,
                        ZM_CLI_ARGC_MAX);
    }
 #endif

    if (p_cli->m_ctx->active_cmd.m_handler != NULL)
    {
        p_cli->m_ctx->active_cmd.m_handler(p_cli,
                                         argc - cmd_handler_lvl,
                                         &argv[cmd_handler_lvl]);
    }
    else
    {
        zm_cli_error(p_cli, ZM_CLI_MSG_SPECIFY_SUBCOMMAND);
    }
    cli_flag_help_clear(p_cli);
}


static void cli_cmd_collect(zm_cli_t const * p_cli)
{
    char data;
    
    while(1)
    {
        if(p_cli->m_trans->m_cli_trans->read(&data, sizeof(data)) <= 0)
        {
            return;
        }
        if (ascii_filter(data) != ZM_SUCCESS)
        {
            continue;
        }
        switch(p_cli->m_ctx->receive_state)
        {
            case ZM_CLI_RECEIVE_DEFAULT:
                if (process_nl(p_cli, data))
                {
                    if (p_cli->m_ctx->cmd_len == 0)
                    {
#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY)
                        history_mode_exit(p_cli);
#endif
                        cursor_next_line_move(p_cli);
                    }
                    else
                    {
                        /* Command execution */
                        cli_execute(p_cli);
                    }

                    cli_state_set(p_cli, ZM_CLI_STATE_ACTIVE);
                    return;
                }
                switch(data)
                {
                    case ZM_PRINT_VT100_ASCII_ESC:       /* ESCAPE */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_ESC);
                        break;
                    case '\0':
                        break;
#if ZM_MODULE_ENABLED(ZM_CLI_METAKEYS)
                    case ZM_PRINT_VT100_ASCII_CTRL_A:    /* CTRL + A */
                        cursor_home_position_move(p_cli);
                        break;
                    case ZM_PRINT_VT100_ASCII_CTRL_C:    /* CTRL + C */
                        cursor_end_position_move(p_cli);
                        if (!cursor_in_empty_line(p_cli))
                        {
                            cursor_next_line_move(p_cli);
                        }
                        cli_state_set(p_cli, ZM_CLI_STATE_ACTIVE);
                        break;
                    case ZM_PRINT_VT100_ASCII_CTRL_E:    /* CTRL + E */
                        cursor_end_position_move(p_cli);
                        break;
                    case ZM_PRINT_VT100_ASCII_CTRL_L:    /* CTRL + L */
                        ZM_PRINT_VT100_CMD(p_cli, ZM_PRINT_VT100_CURSORHOME);
                        ZM_PRINT_VT100_CMD(p_cli, ZM_PRINT_VT100_CLEARSCREEN);
                        zm_cli_printf(p_cli, CLI_NAME_COLOR, "%s", p_cli->m_name);
                        if (cli_flag_echo_is_set(p_cli))
                        {
                            zm_cli_printf(p_cli, CLI_DEFAULT_COLOR, "%s", p_cli->m_ctx->cmd_buff);
                            cursor_position_synchronize(p_cli);
                        }
                        break;
                    case ZM_PRINT_VT100_ASCII_CTRL_U:    /* CTRL + U */
                        cursor_home_position_move(p_cli);
                        cli_cmd_buffer_clear(p_cli);
                        cli_clear_eos(p_cli);
                        break;
                    case ZM_PRINT_VT100_ASCII_CTRL_W:    /* CTRL + W */
                        cli_cmd_word_remove(p_cli);
                        break;
#endif
                    case '\t':                          /* TAB */
                        if (cli_flag_echo_is_set(p_cli))
                        {
                                cli_tab_handle(p_cli);
                        }
                        break;
                    case ZM_PRINT_VT100_ASCII_BSPACE:    /* BACKSPACE */
                        if (cli_flag_echo_is_set(p_cli))
                        {
                            char_backspace(p_cli);
                        }
                        break;
                    case ZM_PRINT_VT100_ASCII_DEL:       /* DELETE */
                        if (cli_flag_echo_is_set(p_cli))
                        {
                            #if defined(__GNUC__) //In linux the case is BACKSPACE
                            char_backspace(p_cli);
                            #else
                            char_delete(p_cli);
                            #endif
                        }
                        break;
                    default:
                        if (isprint((int)data))
                        {
                            if (cli_flag_echo_is_set(p_cli))
                            {
                                char_insert(p_cli, data);
                            }
                            else
                            {
                                char_insert_echo_off(p_cli, data);
                            }
                        }
                        break;
                }
                break;
            case ZM_CLI_RECEIVE_ESC:
                if (data == '[')
                {
                    receive_state_change(p_cli, ZM_CLI_RECEIVE_ESC_SEQ);
                }
                else
                {
                    receive_state_change(p_cli, ZM_CLI_RECEIVE_DEFAULT);
                }
                break;
            case ZM_CLI_RECEIVE_ESC_SEQ:
                receive_state_change(p_cli, ZM_CLI_RECEIVE_DEFAULT);

                if (!cli_flag_echo_is_set(p_cli))
                {
                    return;
                }

                switch (data)
                {
#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY)
                    case 'A': /* UP arrow */
                        history_handle(p_cli, true);
                        break;
                    case 'B': /* DOWN arrow */
                        history_handle(p_cli, false);
                        break;
#endif
                    case 'C': /* RIGHT arrow */
                        right_arrow_handle(p_cli);
                        break;
                    case 'D': /* LEFT arrow */
                        left_arrow_handle(p_cli);
                        break;
                    case '4': /* END Button in ESC[n~ mode */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_TILDE_EXP);
                        /* fall through */
                    case 'F': /* END Button in VT100 mode */
                        cursor_end_position_move(p_cli);
                        break;
                    case '1': /* HOME Button in ESC[n~ mode */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_TILDE_EXP);
                        /* fall through */
                    case 'H': /* HOME Button in VT100 mode */
                        cursor_home_position_move(p_cli);
                        break;
                    case '2': /* INSERT Button in ESC[n~ mode */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_TILDE_EXP);
                        /* fall through */
                    case 'L': /* INSERT Button in VT100 mode */
                        p_cli->m_ctx->internal.flag.insert_mode ^= 1;
                        break;
                    case '3':/* DELETE Button in ESC[n~ mode */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_TILDE_EXP);
                        if (cli_flag_echo_is_set(p_cli))
                        {
                            char_delete(p_cli);
                        }
                        break;
                        default:
                        break;
                }
                break;
            case ZM_CLI_RECEIVE_TILDE_EXP:
                receive_state_change(p_cli, ZM_CLI_RECEIVE_DEFAULT);
                break;
            default:
                receive_state_change(p_cli, ZM_CLI_RECEIVE_DEFAULT);
                break;
        }
    }
}

static inline void  char_insert_echo_off(zm_cli_t const * p_cli, char data)
{
    if (p_cli->m_ctx->cmd_len >= (ZM_CLI_CMD_BUFF_SIZE - 1))
    {
        return;
    }

    p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos++] = data;
    p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos] = '\0';
    ++p_cli->m_ctx->cmd_len;
}


static void char_insert(zm_cli_t const * p_cli, char data)
{
    cli_cmd_len_t diff;
    bool ins_mode = (bool)p_cli->m_ctx->internal.flag.insert_mode;

    diff = p_cli->m_ctx->cmd_len - p_cli->m_ctx->cmd_cur_pos;

    if (!ins_mode)
    {
        if (p_cli->m_ctx->cmd_len >= (ZM_CLI_CMD_BUFF_SIZE - 1))
        {
            return;
        }
        if (diff > 0)
        {
            memmove(&p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos + 1],
                    &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos],
                    diff);
        }
    }
    else
    {
        if ((p_cli->m_ctx->cmd_len >= (ZM_CLI_CMD_BUFF_SIZE - 1)) &&
            (diff == 0))
        {
            /* If cmd buffer is full, it is possible to replace chars but adding new
               is not allowed. */
            return;
        }
    }

    p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos] = data;

    if (!ins_mode)
    {
        p_cli->m_ctx->cmd_buff[++p_cli->m_ctx->cmd_len] = '\0';
    }

    if (diff > 0)
    {
        zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);
        bool last_line = p_cons->cur_y == p_cons->cur_y_end ? true : false;

        /* Below if-else statement is to minimize esc codes transmission. */
        if (last_line)
        {
            zm_cli_printf(p_cli,
                          CLI_DEFAULT_COLOR,
                          "%s",
                          &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
            /* Move cursor one position left less in case of insert mode. */
            cursor_left_move(p_cli, diff - ins_mode);
        }
        else
        {
            /* Save the current cursor position in order to get back after fprintf function. */
            cli_cursor_save(p_cli);
            zm_cli_printf(p_cli,
                          CLI_DEFAULT_COLOR,
                          "%s",
                          &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
            cli_cursor_restore(p_cli);
            /* Move cursor right by one position to edit the next character. */
            cursor_right_move(p_cli, 1);
        }
    }
    else
    {
        /* New char appended at the end of buffer. */
        if (ins_mode)
        {
            p_cli->m_ctx->cmd_buff[++p_cli->m_ctx->cmd_len] = '\0';
        }
        cli_putc(p_cli, data);
    }

    /* Incrementation needs to be executed before invoking function: cursor_in_empty_line. */
    ++p_cli->m_ctx->cmd_cur_pos;

    /* Forcing terminal to switch to a new line if the command is too long. */
    if (cursor_in_empty_line(p_cli))
    {
        cursor_next_line_move(p_cli);
        return;
    }

    if (full_line_cmd(p_cli))
    {
        zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);

        /* The code below will force the terminal to scroll one line down when the currently entered command
         * reaches lower right corner of the terminal screen. */
        cursor_down_move(p_cli, p_cons->cur_y_end - p_cons->cur_y - 1);
        cursor_next_line_move(p_cli);
        cursor_up_move(p_cli, p_cons->cur_y_end - p_cons->cur_y);
        cursor_right_move(p_cli, p_cons->cur_x - 1);
        return;
    }
}

static void cli_state_set(zm_cli_t const * p_cli, cli_state_t state)
{
    p_cli->m_ctx->state = state;

    if (state == ZM_CLI_STATE_ACTIVE)
    {
            cli_cmd_buffer_clear(p_cli);
            zm_cli_printf(p_cli, CLI_NAME_COLOR, "%s", p_cli->m_name);
    }
}
#if ZM_MODULE_ENABLED(ZM_PRINT_VT100_COLORS)
static void vt100_color_set(zm_cli_t const * p_cli, zm_cli_vt100_color_t color)
{
    if (color != ZM_CLI_DEFAULT)
    {
        if (p_cli->m_ctx->vt100_ctx.col.col == color)
        {
            return;
        }

        uint8_t cmd[] = ZM_PRINT_VT100_COLOR(color - 1);

        p_cli->m_ctx->vt100_ctx.col.col = color;
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "%s", cmd);
    }
    else
    {
        static uint8_t const cmd[] = ZM_PRINT_VT100_MODESOFF;

        p_cli->m_ctx->vt100_ctx.col.col = color;
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "%s", cmd);
    }
}
static void vt100_bgcolor_set(zm_cli_t const * p_cli, zm_cli_vt100_color_t bgcolor)
{
    if (bgcolor != ZM_CLI_DEFAULT)
    {
        if (p_cli->m_ctx->vt100_ctx.col.bgcol == bgcolor)
        {
            return;
        }
         /* -1 because default value is first in enum */
        uint8_t cmd[] = ZM_PRINT_VT100_BGCOLOR(bgcolor - 1);

        p_cli->m_ctx->vt100_ctx.col.bgcol = bgcolor;
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "%s", cmd);
    }
}
static void vt100_colors_restore(zm_cli_t const *              p_cli,
                                 zm_cli_vt100_colors_t const * p_color)
{
    vt100_color_set(p_cli, p_color->col);
    vt100_bgcolor_set(p_cli, p_color->bgcol);
}

static inline void vt100_colors_store(zm_cli_t const *        p_cli,
                                      zm_cli_vt100_colors_t * p_color)
{
    memcpy(p_color, &p_cli->m_ctx->vt100_ctx.col, sizeof(zm_cli_vt100_colors_t));
}


#endif

static void cli_write(zm_cli_t const *  p_cli,
                      void const *      p_data,
                      size_t            length,
                      size_t *          p_cnt)
{
    size_t offset = 0;
    size_t cnt;
    while (length)
    {
        cnt = p_cli->m_trans->m_cli_trans->write(&((uint8_t const *)p_data)[offset], length);
        offset += cnt;
        length -= cnt;
    }
    if(p_cnt) *p_cnt = cnt;
}



static void cli_cmd_buffer_clear(zm_cli_t const * p_cli)
{
    p_cli->m_ctx->cmd_buff[0] = '\0';  /* clear command buffer */
    p_cli->m_ctx->cmd_cur_pos = 0;
    p_cli->m_ctx->cmd_len = 0;
}


/* Function required by qsort. */
static int string_cmp(void const * pp_a, void const * pp_b)
{
    ASSERT(pp_a);
    ASSERT(pp_b);

    char const ** pp_str_a = (char const **)pp_a;
    char const ** pp_str_b = (char const **)pp_b;

    return strcmp(*pp_str_a, *pp_str_b);
}

void zm_cli_printf(zm_cli_t const *      p_cli,
                   zm_cli_vt100_color_t  color,
                   char const *          p_fmt,
                                         ...)
{
    ASSERT(p_fmt);
    ASSERT(p_cli);
    //ASSERT(p_cli->m_ctx && p_cli->p_iface && p_cli->p_name);

    va_list args = {0};
    va_start(args, p_fmt);
#if ZM_MODULE_ENABLED(ZM_PRINT_VT100_COLORS)
    if ((p_cli->m_ctx->internal.flag.use_colors) &&
        (color != p_cli->m_ctx->vt100_ctx.col.col))
    {
        zm_cli_vt100_colors_t col;

        vt100_colors_store(p_cli, &col);
        vt100_color_set(p_cli, color);

        zm_printf_fmt(p_cli->m_printf_ctx->printf_ctx, p_fmt, &args);

        vt100_colors_restore(p_cli, &col);
    }
    else
#endif
    {
        zm_printf_fmt(p_cli->m_printf_ctx->printf_ctx, p_fmt, &args);
    }

    va_end(args);
}

ret_code_t zm_cli_start(zm_cli_t const * p_cli)
{
    if (p_cli->m_ctx->state != ZM_CLI_STATE_INITIALIZED)
    {
        return ZM_ERROR_INVALID_STATE;
    }
#if ZM_MODULE_ENABLED(ZM_PRINT_VT100_COLORS)
    vt100_color_set(p_cli, CLI_DEFAULT_COLOR);
    vt100_bgcolor_set(p_cli, CLI_BGCOLOR_COLOR);
#endif
    zm_printf(p_cli->m_printf_ctx->printf_ctx, "\n\n");
    cli_state_set(p_cli, ZM_CLI_STATE_ACTIVE);
    
    return ZM_SUCCESS;
}
ret_code_t zm_cli_stop(zm_cli_t const * p_cli)
{
    ASSERT(p_cli);
    //ASSERT(p_cli->m_ctx && p_cli->p_iface && p_cli->m_name);

    if (p_cli->m_ctx->state == ZM_CLI_STATE_INITIALIZED ||
        p_cli->m_ctx->state == ZM_CLI_STATE_UNINITIALIZED)
    {
        return ZM_ERROR_INVALID_STATE;
    }

    cli_state_set(p_cli, ZM_CLI_STATE_INITIALIZED);
    return ZM_SUCCESS;
}



static inline void transport_buffer_flush(zm_cli_t const * p_cli)
{
    zm_printf_buffer_flush(p_cli->m_printf_ctx->printf_ctx);
}

/* Function prints a string on terminal screen with requested margin.
 * It takes care to not divide words.
 *   p_cli               Pointer to CLI instance.
 *   p_str               Pointer to string to be printed.
 *   terminal_offset     Requested left margin.
 *   offset_first_line   Add margin to the first printed line.
 */
static void format_offset_string_print(zm_cli_t const * p_cli,
                                       char const *      p_str,
                                       size_t            terminal_offset,
                                       bool              offset_first_line)
{
    if (p_str == NULL)
    {
        return;
    }

    if (offset_first_line)
    {
        cursor_right_move(p_cli, terminal_offset);
    }

    size_t length;
    size_t offset = 0;

    /* Skipping whitespace. */
    while (isspace((int)*(p_str + offset)))
    {
       ++offset;
    }

    while (1)
    {
        size_t idx = 0;
        length = cli_strlen(p_str) - offset;

        if (length <= p_cli->m_ctx->vt100_ctx.cons.terminal_wid - terminal_offset)
        {
            for (idx = 0; idx < length; idx++)
            {
                if (*(p_str + offset + idx) == '\n')
                {
                    transport_buffer_flush(p_cli);
                    cli_write(p_cli, p_str + offset, idx, NULL);
                    offset += idx + 1;
                    cursor_next_line_move(p_cli);
                    cursor_right_move(p_cli, terminal_offset);
                    break;
                }
            }
            /* String will fit in one line. */
            zm_printf(p_cli->m_printf_ctx->printf_ctx, p_str + offset);
            break;
        }
        else
        {
            /* String is longer than terminal line so text needs to divide in the way
               to not divide words. */
            length = p_cli->m_ctx->vt100_ctx.cons.terminal_wid - terminal_offset;

            while (1)
            {
                /* Determining line break. */
                if (isspace((int)(*(p_str + offset + idx))))
                {
                    length = idx;
                    if (*(p_str + offset + idx) == '\n')
                    {
                        break;
                    }
                }
                if ((idx + terminal_offset) >= p_cli->m_ctx->vt100_ctx.cons.terminal_wid)
                {
                    /* End of line reached. */
                    break;
                }
                ++idx;
            }

            /* Writing one line, fprintf IO buffer must be flushed before calling cli_write. */
            transport_buffer_flush(p_cli);
            cli_write(p_cli, p_str + offset, length, NULL);
            offset += length;

            /* Calculating text offset to ensure that next line will not begin with a space. */
            while (isspace((int)(*(p_str + offset))))
            {
                ++offset;
            }
            cursor_next_line_move(p_cli);
            cursor_right_move(p_cli, terminal_offset);
        }
    }
    cursor_next_line_move(p_cli);
}

void zm_cli_help_print(zm_cli_t const *               p_cli,
                    zm_cli_getopt_option_t const * p_opt,
                    size_t                          opt_len)
{
    ASSERT(p_cli);
    //ASSERT(p_cli->m_ctx && p_cli->p_iface && p_cli->p_name);

    static uint8_t const tab_len = 2;
    static char const opt_sep[] =", "; /* options separator */
    static char const help[] = "-h, --help";
    static char const cmd_sep[] = " - "; /* command separator */
    uint16_t field_width = 0;
    uint16_t longest_string = cli_strlen(help) - cli_strlen(opt_sep);

    /* Printing help string for command. */
    zm_cli_printf(p_cli,
                    CLI_HELP_PRINT_COLOR,
                    "%s%s",
                    p_cli->m_ctx->active_cmd.m_syntax,
                    cmd_sep);

    field_width = cli_strlen(p_cli->m_ctx->active_cmd.m_syntax) + cli_strlen(cmd_sep);
    format_offset_string_print(p_cli, p_cli->m_ctx->active_cmd.m_help, field_width, false);

    zm_cli_print(p_cli, "Options:");

    /* Looking for the longest option string. */
    if ((opt_len > 0) && (p_opt != NULL))
    {
        for (size_t i = 0; i < opt_len; ++i)
        {
            if (cli_strlen(p_opt[i].p_optname_short) + cli_strlen(p_opt[i].p_optname)
                    > longest_string)
            {
                longest_string = cli_strlen(p_opt[i].p_optname_short)
                    + cli_strlen(p_opt[i].p_optname);
            }
        }
    }
    longest_string += cli_strlen(opt_sep) + tab_len;

    zm_cli_printf(p_cli,
                    CLI_HELP_PRINT_COLOR,
                    "  %-*s:",
                    longest_string,
                    help);

    /* Print help string for options (only -h and --help). */
    field_width = longest_string + tab_len + 1; /* tab_len + 1 == "  " and ':' from: "  %-*s:" */
    format_offset_string_print(p_cli, "Show command help.", field_width, false);

    /* Formating and printing all available options (except -h, --help). */
    if (p_opt != NULL)
    {
        for (size_t i = 0; i < opt_len; ++i)
        {
            if ((p_opt[i].p_optname_short != NULL) && (p_opt[i].p_optname != NULL))
            {
                zm_cli_printf(p_cli,
                                CLI_HELP_PRINT_COLOR,
                                "  %s%s%s",
                                p_opt[i].p_optname_short,
                                opt_sep,
                                p_opt[i].p_optname);
                field_width = longest_string + tab_len;
                cursor_right_move(p_cli,
                                  field_width - ( cli_strlen(p_opt[i].p_optname_short)
                                                + cli_strlen(p_opt[i].p_optname)
                                                + tab_len
                                                + cli_strlen(opt_sep)));
                cli_putc(p_cli, ':');
                ++field_width;  /* incrementing because char ':' was already printed above */
            }
            else if (p_opt[i].p_optname_short != NULL)
            {
                zm_cli_printf(p_cli,
                                CLI_HELP_PRINT_COLOR,
                                "  %-*s:",
                                longest_string,
                                p_opt[i].p_optname_short);
                /* tab_len + 1 == "  " and ':' from: "  %-*s:" */
                field_width = longest_string + tab_len + 1;
            }
            else if (p_opt[i].p_optname != NULL)
            {
                zm_cli_printf(p_cli,
                                CLI_HELP_PRINT_COLOR,
                                "  %-*s:",
                                longest_string,
                                p_opt[i].p_optname);
                /* tab_len + 1 == "  " and ':' from: "  %-*s:" */
                field_width = longest_string + tab_len + 1;
            }
            else
            {
                /* Do nothing. */
            }

            if (p_opt[i].p_optname_help != NULL)
            {
                format_offset_string_print(p_cli, p_opt[i].p_optname_help, field_width, false);
            }
            else
            {
                cursor_next_line_move(p_cli);
            }
        }
    }

    /* Checking if there are any subcommands avilable. */
    if (p_cli->m_ctx->active_cmd.m_subcmd == NULL)
    {
        return;
    }

    /* Printing formatted help of one level deeper subcommands. */
    cli_static_entry_t static_entry;
    cli_cmd_entry_t const * p_cmd = p_cli->m_ctx->active_cmd.m_subcmd;
    cli_static_entry_t const * p_st_cmd = NULL;

    field_width = 0;
    longest_string = 0;

    size_t cmd_idx = 0;

    /* Searching for the longest subcommand to print. */
    while (1)
    {
        cmd_get(p_cmd, !ZM_CLI_CMD_BASE_LVL, cmd_idx++, &p_st_cmd, &static_entry);

        if (p_st_cmd == NULL)
        {
            break;
        }
        if (cli_strlen(p_st_cmd->m_syntax) > longest_string)
        {
            longest_string = cli_strlen(p_st_cmd->m_syntax);
        }
    }

    /* Checking if there are dynamic subcommands. */
    if (cmd_idx == 1)
    {
        /* No dynamic subcommands available. */
        return;
    }

    zm_cli_print(p_cli, "Subcommands:");

    /* Printing subcommands and help string (if exists). */
    cmd_idx = 0;
    while (1)
    {
        cmd_get(p_cmd, !ZM_CLI_CMD_BASE_LVL, cmd_idx++, &p_st_cmd, &static_entry);

        if (p_st_cmd == NULL)
        {
            break;
        }

        field_width = longest_string + tab_len;
        zm_cli_printf(p_cli, CLI_HELP_PRINT_COLOR,"  %-*s:", field_width, p_st_cmd->m_syntax);
        field_width += tab_len + 1; /* tab_len + 1 == "  " and ':' from: "  %-*s:" */

        if (p_st_cmd->m_help != NULL)
        {
            format_offset_string_print(p_cli, p_st_cmd->m_help, field_width, false);
        }
        else
        {
            cursor_next_line_move(p_cli);
        }
    }
}

#if ZM_MODULE_ENABLED(ZM_CLI_HISTORY)
static void cli_history_show(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    //print_usage(p_cli, argv[0], "");
    if(p_cli->m_cmd_hist->m_hist_num)
    {
        uint8_t cnt;
        
        p_cli->m_printf_ctx->printf(p_cli, ZM_PRINT_VT100_COLOR_BLUE, ZM_NEW_LINE"<<<<<<<<<< history >>>>>>>>>>"ZM_NEW_LINE);
        p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_head;
        for(cnt = 0; cnt < p_cli->m_cmd_hist->m_hist_num && p_cli->m_cmd_hist->m_hist_current; cnt++)
        {
            p_cli->m_printf_ctx->printf(p_cli, ZM_PRINT_VT100_COLOR_BLUE, "[%3d]  %s\r"ZM_NEW_LINE, cnt+1, p_cli->m_cmd_hist->m_hist_current->m_cmd);
            p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_current->m_next_hist;
        }
        p_cli->m_cmd_hist->m_hist_current = NULL;
        p_cli->m_printf_ctx->printf(p_cli, ZM_PRINT_VT100_COLOR_BLUE, "<<<<<<<<<<<< end >>>>>>>>>>>>"ZM_NEW_LINE);
    }
}

static void cli_clear_history(zm_cli_t const * p_cli, size_t argc, char **argv)
{
    //print_usage(p_cli, argv[0], "");
    if(p_cli->m_cmd_hist->m_hist_num)
    {
        uint8_t cnt;
        p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_head;
        for(cnt = 0; p_cli->m_cmd_hist->m_hist_current; cnt++)
        {
            p_cli->m_cmd_hist->m_hist_head = p_cli->m_cmd_hist->m_hist_head->m_next_hist;
            p_cli->m_cmd_hist->m_memory->free(p_cli->m_cmd_hist->m_hist_current->m_cmd);
            p_cli->m_cmd_hist->m_memory->free(p_cli->m_cmd_hist->m_hist_current);
            p_cli->m_cmd_hist->m_hist_current = p_cli->m_cmd_hist->m_hist_head;
            p_cli->m_cmd_hist->m_hist_num--;
        }
        p_cli->m_cmd_hist->m_hist_current = NULL;
    }
    p_cli->m_printf_ctx->printf(p_cli, ZM_CLI_INFO, "ok!"ZM_NEW_LINE);
}

CLI_CREATE_STATIC_SUBCMD_SET(sub_history)
{
    CLI_CMD_LOAD_PARA(clear, NULL, "clear history", cli_clear_history),
    
    CLI_SUBCMD_SET_END
};


CLI_CMD_REGISTER(history, &sub_history, "get cmd history", cli_history_show);
#endif

/****************************************************** END OF FILE ******************************************************/                                                                                   
