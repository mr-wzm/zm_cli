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

#include "zm_cli.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
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
//ZM_CLI_DEF(zm_cli, CLI_NAME);
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
static void zm_cli_fprintf(zm_cli_t const *      p_cli,
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
    
    p_cli->m_ctx->state = ZM_CLI_STATE_INITIALIZED;
    
    cli_cmd_init();
    //p_cli->m_cmd_hist->m_hist_head = (cli_hist_pool_t *)malloc(50);
    if(p_cli->m_trans->m_cli_trans->printf)
    {
        cli_cmd_entry_t const * p_cmd = NULL;
        const char ** pp_tes_sorted_cmds = CLI_SORTED_CMD_PTRS_START_ADDR_GET;
        p_cli->m_trans->m_cli_trans->printf("cli init ok!, version : %s, cmd :%d\r\n", VERSION_STRING, CLI_DATA_SECTION_ITEM_COUNT);
        for(uint8_t i = 0; i < CLI_DATA_SECTION_ITEM_COUNT; i++)
        {
            p_cmd = CLI_DATA_SECTION_ITEM_GET(i);
            p_cli->m_trans->m_cli_trans->printf("%s -- %s : %s\r\n", p_cmd->u.m_static_entry->m_syntax, pp_tes_sorted_cmds[i], p_cmd->u.m_static_entry->m_help);
            if(p_cmd->u.m_static_entry->m_handler) p_cmd->u.m_static_entry->m_handler(p_cli, NULL, NULL);
        }
    }
    p_cli->m_printf_ctx->auto_flush = true;
    cli_state_set(p_cli, ZM_CLI_STATE_ACTIVE);
    return 0;
}

void cli_process(zm_cli_t const * p_cli)
{
    ASSERT(p_cli);
    
    cli_cmd_collect(p_cli);
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

static void cli_cmd_collect(zm_cli_t const * p_cli)
{
    char data;
    
    while(1)
    {
        if(p_cli->m_trans->m_cli_trans->read(&data, sizeof(data)) < 0)
        {
            return;
        }
        
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
        zm_printf(p_cli->m_printf_ctx, "%s", cmd);
    }
    else
    {
        static uint8_t const cmd[] = ZM_CLI_VT100_MODESOFF;

        p_cli->m_ctx->vt100_ctx.col.col = color;
        zm_printf(p_cli->m_printf_ctx, "%s", cmd);
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

static void zm_cli_fprintf(zm_cli_t const *      p_cli,
                           zm_cli_vt100_color_t  color,
                           char const *          p_fmt,
                                                 ...)
{
    ASSERT(p_fmt);
    ASSERT(p_cli);
    //ASSERT(p_cli->p_ctx && p_cli->p_iface && p_cli->p_name);

    va_list args = {0};
    va_start(args, p_fmt);

    if ((color != p_cli->m_ctx->vt100_ctx.col.col))
    {
        zm_cli_vt100_colors_t col;

        vt100_colors_store(p_cli, &col);
        vt100_color_set(p_cli, color);

        zm_printf_fmt(p_cli->m_printf_ctx, p_fmt, &args);

        vt100_colors_restore(p_cli, &col);
    }
    else
    {
        zm_printf_fmt(p_cli->m_printf_ctx, p_fmt, &args);
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
            zm_cli_fprintf(p_cli, ZM_CLI_INFO, "%s", p_cli->m_name);
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
        zm_printf(p_cli->m_printf_ctx, "%s", cmd);
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
