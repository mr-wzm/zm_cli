/*****************************************************************
* Copyright (C) 2017 60Plus Technology Co.,Ltd.*
******************************************************************
* zm_cli.h
*
* DESCRIPTION:
*     zm_cli.h
* AUTHOR:
*     gang.cheng
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
#ifndef __ZM_CLI_H__
#define __ZM_CLI_H__
#ifdef __cplusplus
extern "C"
{
#endif
/*************************************************************************************************************************
 *                                                       INCLUDES                                                        *
 *************************************************************************************************************************/
#include <stdlib.h>
#include "zm_section.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
// <o> CLI_CMD_BUFF_SIZE - Maximum buffer size for a single command. 
#ifndef CLI_CMD_BUFF_SIZE
#define CLI_CMD_BUFF_SIZE 255
#endif
   
#define CLI_NAME "zm_cli >" //!< Terminal name.
/**
 * Memory poll function.
 */
#define cli_malloc          malloc
#define cli_free            free
/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
#if (CLI_CMD_BUFF_SIZE > 65535)
    typedef uint32_t nrf_cli_cmd_len_t;
#elif (CLI_CMD_BUFF_SIZE > 255)
    typedef uint16_t cli_cmd_len_t;
#else
    typedef uint8_t cli_cmd_len_t;
#endif
/**
 * Cli instance context.
 */
typedef struct
{
  cli_cmd_len_t cmd_len; //!< Command length.
  cli_cmd_len_t cmd_pos; //!< Command buffer cursor position.
}cli_ctx_t;
/**
 * Cli history pool.
 */
typedef struct cli_hist_pool_T
{
  char * m_cmd;         //!< Pointer to string with cmd name.
  struct cli_hist_pool_T * m_next_hist; //!< Pointer to next cmd.
  struct cli_hist_pool_T * m_last_hist; //!< Pointer to last cmd.
}cli_hist_pool_t;
/**
 * Cli history.
 */
typedef struct
{
  uint8_t m_hist_num;   //!< number of history.
  cli_hist_pool_t * m_hist_head;    //!< Pointer to first history.
  cli_hist_pool_t * m_hist_tail;     //!< Pointer to the tail of history list.
  cli_hist_pool_t * m_hist_current; //!< Pointer to current history.
}cli_history_t;
  
typedef struct
{
  char const * const m_name; //!< Terminal name.
  cli_history_t m_cmd_hist;     //!< Memory reserved for commands history.
}zm_cli_t;
/**
 * Cli command handler prototype.
 */
typedef void (*cli_cmd_handler)(zm_cli_t const * p_cli, size_t argc, char **argv);

typedef struct cli_def_att_T
{
    char const * m_syntax;  //!< Command syntax strings.
    char const * m_help;    //!< Command help string.
    struct cli_def_att_T const * m_subcmd; //!< Pointer to subcommand.
    cli_cmd_handler m_handler;  //!< Command handler.
}cli_def_att_t;
/*************************************************************************************************************************
 *                                                  AFTER MACROS                                                         *
 *************************************************************************************************************************/
#define CLI_SECTION_DEF(section_name, data_type) \
        SECTION_ITEM_REGISTER(CONCAT_2(section_name, _s0.end), const data_type CONCAT_2(section_name, $$Base)) = (const data_type){0};\
        SECTION_ITEM_REGISTER(CONCAT_2(section_name, _s1.end), const data_type CONCAT_2(section_name, $$Limit)) = (const data_type){0}

#define CLI_CMD_REGISTER
        
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
  
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 
#ifdef __cplusplus
}
#endif
#endif /* zm_cli.h */
