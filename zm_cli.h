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
#include <stdbool.h>
#include "zm_section.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#define CLI_PRIMAIRE_VERSION    1
#define CLI_SUB_VERSION         2
#define CLI_REVISED_VERSION     0
#define VERSION_STRING          STRINGIFY(CLI_PRIMAIRE_VERSION.CLI_SUB_VERSION.CLI_REVISED_VERSION)

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
     * @param[in] a_buf       Pointer to the destination buffer.
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
     * printf function.
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
  cli_ctx_t * m_ctx;    //!< Internal context.
  cli_transport_t * m_trans;    //!< Transport interface.
  cli_history_t * m_cmd_hist;     //!< Memory reserved for commands history.
}zm_cli_t;
/**
 * Cli command handler prototype.
 */
typedef void (*cli_cmd_handler)(zm_cli_t const * p_cli, size_t argc, char **argv);
/**
 * Cli static command descriptor.
 */
typedef struct cli_def_entry_T
{
    char const * m_syntax;  //!< Command syntax strings.
    char const * m_help;    //!< Command help string.
    struct cli_cmd_entry_t const * m_subcmd; //!< Pointer to subcommand.
    cli_cmd_handler m_handler;  //!< Command handler.
}cli_def_entry_t;

/*
 *
 */
typedef struct
{
    union
    {
        cli_def_entry_t const * m_static_entry;
    }u;
}cli_cmd_entry_t;
/*************************************************************************************************************************
 *                                                  AFTER MACROS                                                         *
 *************************************************************************************************************************/
/**
 * @brief Macro for defining a command line interface instance. 
 *
 * @param[in] name              Instance name.
 * @param[in] cli_prefix        CLI prefix string.
 */
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

/**
 * @brief Macro for register cli transport api(@ref cli_trans_api_t).
 *
 * @param[in]   cli_name      a command line interface instance.
 * @param[in]   trans_iface   struct cli_trans_api_t api.
 */
#define CLI_REGISTER_TRANS(cli_name, trans_iface) \
        CONCAT_2(cli_name, _trans).m_cli_trans = trans_iface
          
          
/**@brief   Macro for creating a cli section.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variables to be registered in the section.
 */
#define CLI_SECTION_DEF(section_name, data_type) \
        ZM_SECTION_DEF(section_name, data_type)
        
/**
 * @brief Initializes a CLI command (@ref cli_def_entry_t).
 *
 * @param[in] _syntax  Command syntax (for example: history).
 * @param[in] _subcmd  Pointer to a subcommands array.
 * @param[in] _help    Pointer to a command help string.
 * @param[in] _handler Pointer to a function handler.
 */
#define CLI_CMD_LOAD_PARA(_syntax, _subcmd, _help, _handler) \
        { \
            .m_syntax = (const char *)STRINGIFY(_syntax), \
            .m_help = (const char *)_help, \
            .m_subcmd = _subcmd, \
            .m_handler = _handler, \
        }
/**
 * @brief Macro for defining and adding a root command (level 0).
 *
 * @note Each root command shall have unique syntax.
 *
 * @param[in] syntax  Command syntax (for example: history).
 * @param[in] subcmd  Pointer to a subcommands array.
 * @param[in] help    Pointer to a command help string.
 * @param[in] handler Pointer to a function handler.
 */
#define CLI_CMD_REGISTER(syntax, subcmd, help, handler) \
        cli_def_entry_t const CONCAT_3(cli_, syntax, _raw) = \
            CLI_CMD_LOAD_PARA(syntax, subcmd, help, handler); \
        ZM_SECTION_ITEM_REGISTER(COMMAND_SECTION_NAME, \
                                 cli_cmd_entry_t const CONCAT_3(cli_, syntax, _const)) = { \
                                     .u = {.m_static_entry = &CONCAT_3(cli_, syntax, _raw)} \
                                     };\
        ZM_SECTION_ITEM_REGISTER(PARA_SECTION_NAME, char const * CONCAT_2(syntax, _str_ptr))

/**
 * @brief Macro for creating a subcommand set. It must be used outside of any function body.
 *
 * @param[in] name  Name of the subcommand set.
 */        
#define CLI_CREATE_STATIC_SUBCMD_SET(name) \
        static cli_def_entry_t const CONCAT_2(name, _raw)[]; \
        static cli_def_entry_t const name = CONCAT_2(name, _raw); \
        static cli_def_entry_t const CONCAT_2(name, _raw)[] = 
          
/**
 * Define ending subcommands set.
 *
 */
#define CLI_SUBCMD_SET_END {NULL}
        
/*************************************************************************************************************************
 *                                                  EXTERNAL VARIABLES                                                   *
 *************************************************************************************************************************/
  
/*************************************************************************************************************************
 *                                                   PUBLIC FUNCTIONS                                                    *
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
int cli_init(cli_trans_api_t * transApi);
        
     
#ifdef __cplusplus
}
#endif
#endif /* zm_cli.h */
