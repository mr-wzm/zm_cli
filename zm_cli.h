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
#include <stdint.h>
#include "zm_section.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#define COMMAND_SECTION_NAME    cli_command
#define PARA_SECTION_NAME       cli_sorted_cmd
 
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

/*
 * Cli transport api.
 */
typedef struct
{
    /**
     * @param[in] a_data       Pointer to the destination buffer.
     * @param[in] length       Destination buffer length.
     * @return read data bytes.
     */
    int (* read)(void *a_buf, uint16_t length);
    /**
     * @param[in] a_data       Pointer to the destination buffer.
     * @param[in] length       Destination buffer length.
     * @return write data bytes.
     */
    int (* write)(void *a_data, uint16_t length);
    /**
     * printf
     */
    int (* printf)(const char * format, ...);
}cli_trans_api_t;

/**
 * Unified CLI transport interface.
 */
typedef struct
{
    cli_trans_api_t * m_cli_trans;
}cli_transport_t;
/**
 * Cli instance context.
 */
typedef struct
{
  cli_cmd_len_t cmd_len; //!< Command length.
  cli_cmd_len_t cmd_cur_pos; //!< Command buffer cursor position.
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
  cli_ctx_t * m_ctx;
  cli_transport_t * m_trans;
  cli_history_t const * m_cmd_hist;     //!< Memory reserved for commands history.
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
#define ZM_CLI_DEF(name, cli_prefix) \
        static zm_cli_t const name; \
        static cli_transport_t CONCAT_2(name, _trans); \
        static cli_ctx_t CONCAT_2(name, _ctx); \
        static cli_history_t CONCAT_2(name, _hist) = { \
            .m_hist_num = 0, \
            .m_hist_head = NULL, \
            .m_hist_tail = NULL, \
            .m_hist_current = NULL, \
        }; \
        static zm_cli_t const name = { \
            .m_name = cli_prefix, \
            .m_ctx = &CONCAT_2(name, _ctx), \
            .m_trans = &CONCAT_2(name, _trans), \
            .m_cmd_hist = &CONCAT_2(name, _hist), \
        }
        
#define CLI_REGISTER_TRANS(cli_name, trans_iface) \
        CONCAT_2(cli_name, _trans).m_cli_trans = trans_iface

            
#define CLI_MEMBER_SECTION(section_name)    CONCAT_2(section_name, _s1)    

#define CLI_SECTION_DEF(section_name, data_type) \
        ZM_SECTION_ITEM_REGISTER(CONCAT_2(section_name, _s0_), const data_type CONCAT_2(section_name, $$Base)) = (const data_type){0};\
        ZM_SECTION_ITEM_REGISTER(CONCAT_2(section_name, _s1_), const data_type CONCAT_2(section_name, $$Limit)) = (const data_type){0}

#define CLI_CMD_LOAD_PARA(syntax, subcmd, help, handler) \
        { \
            .m_syntax = (const char *)STRINGIFY(syntax), \
            .m_help = (const char *)help, \
            .m_subcmd = subcmd, \
            .m_handler = handler, \
        }
        
#define CLI_CMD_REGISTER(syntax, subcmd, help, handler) \
        ZM_SECTION_ITEM_REGISTER(CLI_MEMBER_SECTION(COMMAND_SECTION_NAME), \
        cli_def_att_t const CONCAT_3(cli_, syntax, _raw)) = \
            CLI_CMD_LOAD_PARA(syntax, subcmd, help, handler); \
        ZM_SECTION_ITEM_REGISTER(CLI_MEMBER_SECTION(PARA_SECTION_NAME), char const * CONCAT_2(syntax, _str_ptr))
        
                                 
        
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
  
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
 *************************************************************************************************************************/
 int cli_init(cli_trans_api_t * transApi);
        
     
#ifdef __cplusplus
}
#endif
#endif /* zm_cli.h */
