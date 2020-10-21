/*****************************************************************
* Copyright (C) 20120 ZM Technology Co.,Ltd.*
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
#define SECTION_OFFSET      0    //nonuse base para
/*  */
CLI_SECTION_DEF(COMMAND_SECTION_NAME, cli_def_att_t);
#define CLI_DATA_SECTION_ITEM_GET(i) ZM_SECTION_ITEM_GET(COMMAND_SECTION_NAME, cli_def_att_t, (i + SECTION_OFFSET))
#define CLI_DATA_SECTION_ITEM_COUNT  (ZM_SECTION_ITEM_COUNT(COMMAND_SECTION_NAME, cli_def_att_t) - SECTION_OFFSET)
/*  */
CLI_SECTION_DEF(PARA_SECTION_NAME, const char *);
#define CLI_SORTED_CMD_PTRS_ITEM_GET(i) ZM_SECTION_ITEM_GET(PARA_SECTION_NAME, const char *, (i + SECTION_OFFSET))
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
ZM_CLI_DEF(zm_cli, CLI_NAME);
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                    LOCAL VARIABLES                                                    *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                 FUNCTION DECLARATIONS                                                 *
 *************************************************************************************************************************/
 
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
*     transApi : transport api(refer to @cli_trans_api_t).
* OUTPUTS:
*     null
* RETURNS:
*     state
* NOTE:
*     null
*****************************************************************/
int cli_init(cli_trans_api_t * transApi)
{
    CLI_REGISTER_TRANS(zm_cli, transApi);
    
    const char ** pp_sorted_cmds = CLI_SORTED_CMD_PTRS_START_ADDR_GET;
    for(uint8_t i = 0; i < CLI_DATA_SECTION_ITEM_COUNT; i++)
    {
        const cli_def_att_t * cmd;
        cmd = CLI_DATA_SECTION_ITEM_GET(i);

        pp_sorted_cmds[i] = cmd->m_syntax;
    }
    if(zm_cli.m_trans->m_cli_trans->printf)
    {
        cli_def_att_t const * p_cmd = NULL;
        const char ** pp_tes_sorted_cmds = CLI_SORTED_CMD_PTRS_START_ADDR_GET;
        zm_cli.m_trans->m_cli_trans->printf("cli init ok!, version : %s, cmd :%d\r\n", VERSION_STRING, CLI_DATA_SECTION_ITEM_COUNT);
        for(uint8_t i = 0; i < CLI_DATA_SECTION_ITEM_COUNT; i++)
        {
            p_cmd = CLI_DATA_SECTION_ITEM_GET(i);
            zm_cli.m_trans->m_cli_trans->printf("%s -- %s : %s\r\n", p_cmd->m_syntax, pp_tes_sorted_cmds[i], p_cmd->m_help);
            if(p_cmd->m_handler) p_cmd->m_handler(&zm_cli, NULL, NULL);
        }
    }
    return 0;
}

CLI_CMD_REGISTER(help, NULL, "get help", NULL);
CLI_CMD_REGISTER(history, NULL, "get history", NULL);
/****************************************************** END OF FILE ******************************************************/                                                                                   
