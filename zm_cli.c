/*****************************************************************
* Copyright (C) 2020 ZM Technology Personal.                     *
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
#define ZM_CLI_INIT_OPTION_PRINTER                  (NULL)

#define ZM_CLI_MAX_TERMINAL_SIZE       (250u)
#define ZM_CLI_CURSOR_POSITION_BUFFER  (10u)   /* 10 == {esc, [, 2, 5, 0, ;, 2, 5, 0, '\0'} */
#define ZM_CLI_DEFAULT_TERMINAL_WIDTH  (80u)   /* Default PuTTY width. */
#define ZM_CLI_DEFAULT_TERMINAL_HEIGHT (24u)   /* Default PuTTY height. */
#define ZM_CLI_INITIAL_CURS_POS        (1u)    /* Initial cursor position is: (1, 1). */

/* Macro to send VT100 commands. */
#define ZM_CLI_VT100_CMD(_p_cli_, _cmd_)   {       \
    ASSERT(_p_cli_);                                \
    ASSERT(_p_cli_->m_printf_ctx->printf_ctx);                 \
    static char const cmd[] = _cmd_;                \
    zm_printf(_p_cli_->m_printf_ctx->printf_ctx, "%s", cmd); \
}

/*  */
CLI_SECTION_DEF(COMMAND_SECTION_NAME, cli_def_entry_t);
#define CLI_DATA_SECTION_ITEM_GET(i) ZM_SECTION_ITEM_GET(COMMAND_SECTION_NAME, cli_cmd_entry_t, (i))
#define CLI_DATA_SECTION_ITEM_COUNT  ZM_SECTION_ITEM_COUNT(COMMAND_SECTION_NAME, cli_cmd_entry_t)
/*  */
CLI_SECTION_DEF(PARA_SECTION_NAME, const char);
#define CLI_SORTED_CMD_PTRS_ITEM_GET(i) ZM_SECTION_ITEM_GET(PARA_SECTION_NAME, const char, (i))
#define CLI_SORTED_CMD_PTRS_START_ADDR_GET ZM_SECTION_START_ADDR(PARA_SECTION_NAME)
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
static void cli_cmd_init(void);
static void cli_cmd_collect(zm_cli_t const * p_cli);
static void char_insert(zm_cli_t const * p_cli, char data);
static void cli_cmd_buffer_clear(zm_cli_t const * p_cli);
static inline void vt100_colors_store(zm_cli_t const *        p_cli,
                                      zm_cli_vt100_colors_t * p_color);
static void vt100_color_set(zm_cli_t const * p_cli, zm_cli_vt100_color_t color);
static void vt100_bgcolor_set(zm_cli_t const * p_cli, zm_cli_vt100_color_t bgcolor);
static void vt100_colors_restore(zm_cli_t const *              p_cli,
                                 zm_cli_vt100_colors_t const * p_color);
static void cli_state_set(zm_cli_t const * p_cli, cli_state_t state);
static void cli_write(zm_cli_t const *  p_cli,
                      void const *      p_data,
                      size_t            length,
                      size_t *          p_cnt);
static void zm_cli_printf(zm_cli_t const *      p_cli,
                          zm_cli_vt100_color_t  color,
                          char const *          p_fmt,
                                                 ...);
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
int cli_init(zm_cli_t const * p_cli)
{
    ASSERT(p_cli);
    
    p_cli->m_cmd_hist->m_hist_head = NULL;
    p_cli->m_cmd_hist->m_hist_tail = NULL;
    p_cli->m_cmd_hist->m_hist_current = NULL;
    p_cli->m_cmd_hist->m_hist_num = 0;
    p_cli->m_ctx->state = ZM_CLI_STATE_INITIALIZED;
    p_cli->m_printf_ctx->printf = zm_cli_printf;
    p_cli->m_ctx->internal.flag.use_colors = true;
    p_cli->m_ctx->internal.flag.insert_mode = true;
    p_cli->m_ctx->vt100_ctx.cons.terminal_wid = ZM_CLI_DEFAULT_TERMINAL_WIDTH;
    p_cli->m_ctx->vt100_ctx.cons.terminal_hei = ZM_CLI_DEFAULT_TERMINAL_HEIGHT;

    cli_cmd_init();

    cli_cmd_entry_t const * p_cmd = NULL;
    const char ** pp_tes_sorted_cmds = CLI_SORTED_CMD_PTRS_START_ADDR_GET;
    p_cli->m_printf_ctx->printf(p_cli, ZM_CLI_INFO, "cli init ok!, version : %s, cmd :%d\r\n", VERSION_STRING, CLI_DATA_SECTION_ITEM_COUNT);
    for(uint8_t i = 0; i < CLI_DATA_SECTION_ITEM_COUNT; i++)
    {
        p_cmd = CLI_DATA_SECTION_ITEM_GET(i);
        p_cli->m_printf_ctx->printf(p_cli, ZM_CLI_INFO, "%s -- %s : %s\r\n", p_cmd->u.m_static_entry->m_syntax, pp_tes_sorted_cmds[i], p_cmd->u.m_static_entry->m_help);
        if(p_cmd->u.m_static_entry->m_handler) p_cmd->u.m_static_entry->m_handler(p_cli, NULL, NULL);
    }

    p_cli->m_printf_ctx->printf_ctx->auto_flush = true;
    
    return 0;
}

ret_code_t nrf_cli_start(zm_cli_t const * p_cli)
{
    if (p_cli->m_ctx->state != ZM_CLI_STATE_INITIALIZED)
    {
        return ZM_ERROR_INVALID_STATE;
    }
    
    vt100_color_set(p_cli, ZM_CLI_NORMAL);
    vt100_bgcolor_set(p_cli, ZM_CLI_VT100_COLOR_BLACK);

    zm_printf(p_cli->m_printf_ctx->printf_ctx, "\n\n");
    cli_state_set(p_cli, ZM_CLI_STATE_ACTIVE);
    
    return ZM_SUCCESS;
}

void cli_process(zm_cli_t const * p_cli)
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
    ZM_CLI_VT100_CMD(p_cli, ZM_CLI_VT100_SAVECURSOR);
}
/* Function sends VT100 command to restore saved cursor position. */
static inline void cli_cursor_restore(zm_cli_t const * p_cli)
{
    ZM_CLI_VT100_CMD(p_cli, ZM_CLI_VT100_RESTORECURSOR);
}
/* Function sends VT100 command to clear the screen from cursor position to end of the screen. */
static inline void cli_clear_eos(zm_cli_t const * p_cli)
{
    ZM_CLI_VT100_CMD(p_cli, ZM_CLI_VT100_CLEAREOS);
}
/* Functions returns true if new line character shall be processed */
static bool process_nl(zm_cli_t const * p_cli, uint8_t data)
{
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

static inline void history_mode_exit(zm_cli_t const * p_cli)
{
    p_cli->m_cmd_hist->m_hist_current = NULL;
}

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
        cli_putc(p_cli, ZM_CLI_VT100_ASCII_BSPACE);

        zm_cli_multiline_cons_t const * p_cons = multiline_console_data_check(p_cli);
        bool last_line = p_cons->cur_y == p_cons->cur_y_end ? true : false;

        if (last_line)
        {
            zm_cli_printf(p_cli,
                          ZM_CLI_NORMAL,
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
                           ZM_CLI_NORMAL,
                           "%s",
                           &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
            cli_cursor_restore(p_cli);
        }
    }
    else
    {
        static char const cmd_bspace[] = {
            ZM_CLI_VT100_ASCII_BSPACE, ' ', ZM_CLI_VT100_ASCII_BSPACE, '\0'};
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
                        ZM_CLI_NORMAL,
                        "%s",
                        &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
        ZM_CLI_VT100_CMD(p_cli, ZM_CLI_VT100_CLEAREOL);
        cursor_left_move(p_cli, --diff);
    }
    else
    {
        cli_cursor_save(p_cli);
        cli_clear_eos(p_cli);
        zm_cli_printf(p_cli,
                        ZM_CLI_NORMAL,
                        "%s",
                        &p_cli->m_ctx->cmd_buff[p_cli->m_ctx->cmd_cur_pos]);
        cli_cursor_restore(p_cli);
    }
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
                        history_mode_exit(p_cli);
                        cursor_next_line_move(p_cli);
                    }
                    else
                    {
                        /* Command execution */
                        //cli_execute(p_cli);
                    }

                    cli_state_set(p_cli, ZM_CLI_STATE_ACTIVE);
                    return;
                }
                switch(data)
                {
                    case ZM_CLI_VT100_ASCII_ESC:       /* ESCAPE */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_ESC);
                        break;
                    case '\0':
                        break;
                    case ZM_CLI_VT100_ASCII_CTRL_A:    /* CTRL + A */
                        //cursor_home_position_move(p_cli);
                        break;
                    case ZM_CLI_VT100_ASCII_CTRL_C:    /* CTRL + C */
                        //cursor_end_position_move(p_cli);
                        //if (!cursor_in_empty_line(p_cli))
                        {
                            cursor_next_line_move(p_cli);
                        }
                        cli_state_set(p_cli, ZM_CLI_STATE_ACTIVE);
                        break;
                    case ZM_CLI_VT100_ASCII_CTRL_E:    /* CTRL + E */
                        //cursor_end_position_move(p_cli);
                        break;
                    case '\t':                          /* TAB */
                        //if (cli_flag_echo_is_set(p_cli))
                        {
                          //  cli_tab_handle(p_cli);
                        }
                        break;
                    case ZM_CLI_VT100_ASCII_BSPACE:    /* BACKSPACE */
                        //if (cli_flag_echo_is_set(p_cli))
                        {
                            char_backspace(p_cli);
                        }
                        break;
                    case ZM_CLI_VT100_ASCII_DEL:       /* DELETE */
                        //if (cli_flag_echo_is_set(p_cli))
                        {
                            char_delete(p_cli);
                        }
                        break;
                    default:
                        if (isprint((int)data))
                        {
                            //if (cli_flag_echo_is_set(p_cli))
                            {
                                char_insert(p_cli, data);
                            }
                            //else
                            {
                              //  char_insert_echo_off(p_cli, data);
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

                //if (!cli_flag_echo_is_set(p_cli))
                {
                  //  return;
                }

                switch (data)
                {
                    case 'A': /* UP arrow */
                        //history_handle(p_cli, true);
                        break;
                    case 'B': /* DOWN arrow */
                        //history_handle(p_cli, false);
                        break;
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
                        //cursor_end_position_move(p_cli);
                        break;
                    case '1': /* HOME Button in ESC[n~ mode */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_TILDE_EXP);
                        /* fall through */
                    case 'H': /* HOME Button in VT100 mode */
                        //cursor_home_position_move(p_cli);
                        break;
                    case '2': /* INSERT Button in ESC[n~ mode */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_TILDE_EXP);
                        /* fall through */
                    case 'L': /* INSERT Button in VT100 mode */
                        p_cli->m_ctx->internal.flag.insert_mode ^= 1;
                        break;
                    case '3':/* DELETE Button in ESC[n~ mode */
                        receive_state_change(p_cli, ZM_CLI_RECEIVE_TILDE_EXP);
                        //if (cli_flag_echo_is_set(p_cli))
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
                          ZM_CLI_NORMAL,
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
                          ZM_CLI_NORMAL,
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

static inline void vt100_colors_store(zm_cli_t const *        p_cli,
                                      zm_cli_vt100_colors_t * p_color)
{
    memcpy(p_color, &p_cli->m_ctx->vt100_ctx.col, sizeof(zm_cli_vt100_colors_t));
}

static void vt100_color_set(zm_cli_t const * p_cli, zm_cli_vt100_color_t color)
{
    if (color != ZM_CLI_DEFAULT)
    {
        if (p_cli->m_ctx->vt100_ctx.col.col == color)
        {
            return;
        }

        uint8_t cmd[] = ZM_CLI_VT100_COLOR(color - 1);

        p_cli->m_ctx->vt100_ctx.col.col = color;
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "%s", cmd);
    }
    else
    {
        static uint8_t const cmd[] = ZM_CLI_VT100_MODESOFF;

        p_cli->m_ctx->vt100_ctx.col.col = color;
        zm_printf(p_cli->m_printf_ctx->printf_ctx, "%s", cmd);
    }
}

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

static void zm_cli_printf(zm_cli_t const *      p_cli,
                           zm_cli_vt100_color_t  color,
                           char const *          p_fmt,
                                                 ...)
{
    ASSERT(p_fmt);
    ASSERT(p_cli);
    //ASSERT(p_cli->m_ctx && p_cli->p_iface && p_cli->p_name);

    va_list args = {0};
    va_start(args, p_fmt);

    if ((color != p_cli->m_ctx->vt100_ctx.col.col))
    {
        zm_cli_vt100_colors_t col;

        vt100_colors_store(p_cli, &col);
        vt100_color_set(p_cli, color);

        zm_printf_fmt(p_cli->m_printf_ctx->printf_ctx, p_fmt, &args);

        vt100_colors_restore(p_cli, &col);
    }
    else
    {
        zm_printf_fmt(p_cli->m_printf_ctx->printf_ctx, p_fmt, &args);
    }

    va_end(args);
}

static void cli_cmd_buffer_clear(zm_cli_t const * p_cli)
{
    p_cli->m_ctx->cmd_buff[0] = '\0';  /* clear command buffer */
    p_cli->m_ctx->cmd_cur_pos = 0;
    p_cli->m_ctx->cmd_len = 0;
}

static void cli_state_set(zm_cli_t const * p_cli, cli_state_t state)
{
    p_cli->m_ctx->state = state;

    if (state == ZM_CLI_STATE_ACTIVE)
    {
            cli_cmd_buffer_clear(p_cli);
            zm_cli_printf(p_cli, ZM_CLI_INFO, "%s", p_cli->m_name);
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
        uint8_t cmd[] = ZM_CLI_VT100_BGCOLOR(bgcolor - 1);

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
/* Function required by qsort. */
static int string_cmp(void const * pp_a, void const * pp_b)
{
    ASSERT(pp_a);
    ASSERT(pp_b);

    char const ** pp_str_a = (char const **)pp_a;
    char const ** pp_str_b = (char const **)pp_b;

    return strcmp(*pp_str_a, *pp_str_b);
}

CLI_CMD_REGISTER(help, NULL, "get help", NULL);
CLI_CMD_REGISTER(history, NULL, "get history", NULL);
CLI_CMD_REGISTER(ares, NULL, "get ares", NULL);
/****************************************************** END OF FILE ******************************************************/                                                                                   
