/*****************************************************************
* Copyright (C) 2020 WangZiming. All rights reserved.            *
******************************************************************
* zm_cli.h
*
* DESCRIPTION:
*     zm_cli.h
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
#include <stdbool.h>
#include "zm_config.h"
#include "zm_section.h"
#include "zm_printf.h"
/*************************************************************************************************************************
 *                                                        MACROS                                                         *
 *************************************************************************************************************************/
#define CLI_PRIMAIRE_VERSION    1
#define CLI_SUB_VERSION         2
#define CLI_REVISED_VERSION     0
#define VERSION_STRING          STRINGIFY(CLI_PRIMAIRE_VERSION.CLI_SUB_VERSION.CLI_REVISED_VERSION)

#define COMMAND_SECTION_NAME    cli_command
#define PARA_SECTION_NAME       cli_sorted_cmd

/** Cli color configure **/
#define CLI_NAME_COLOR          ZM_CLI_DEFAULT  ///< Cli name cloor
#define CLI_DEFAULT_COLOR       ZM_CLI_DEFAULT  ///< Cli default color
#define CLI_OPTION_COLOR        ZM_CLI_OPTION   ///< Cli option color
#define CLI_HELP_PRINT_COLOR    ZM_CLI_DEFAULT  ///< Cli help printf color
#define CLI_INFO_COLOR          ZM_CLI_INFO     ///< Cli information color
#define CLI_WARN_COLOR          ZM_CLI_WARNING  ///< Cli warnning color
#define CLI_ERROR_COLOR         ZM_CLI_ERROR    ///< Cli error color
#define CLI_BGCOLOR_COLOR       ZM_PRINT_VT100_COLOR_DEFAULT    ///< Cli background color
    
   
/** @defgroup ZM_ERRORS_BASE Error Codes Base number definitions
 * @{ */
#define ZM_ERROR_BASE_NUM      (0x0)       ///< Global error base
#define ZM_ERROR_SDM_BASE_NUM  (0x1000)    ///< SDM error base
#define ZM_ERROR_SOC_BASE_NUM  (0x2000)    ///< SoC error base
#define ZM_ERROR_STK_BASE_NUM  (0x3000)    ///< STK error base
/** @} */

#define ZM_SUCCESS                           (ZM_ERROR_BASE_NUM + 0)  ///< Successful command
#define ZM_ERROR_SVC_HANDLER_MISSING         (ZM_ERROR_BASE_NUM + 1)  ///< SVC handler is missing
#define ZM_ERROR_SOFTDEVICE_NOT_ENABLED      (ZM_ERROR_BASE_NUM + 2)  ///< SoftDevice has not been enabled
#define ZM_ERROR_INTERNAL                    (ZM_ERROR_BASE_NUM + 3)  ///< Internal Error
#define ZM_ERROR_NO_MEM                      (ZM_ERROR_BASE_NUM + 4)  ///< No Memory for operation
#define ZM_ERROR_NOT_FOUND                   (ZM_ERROR_BASE_NUM + 5)  ///< Not found
#define ZM_ERROR_NOT_SUPPORTED               (ZM_ERROR_BASE_NUM + 6)  ///< Not supported
#define ZM_ERROR_INVALID_PARAM               (ZM_ERROR_BASE_NUM + 7)  ///< Invalid Parameter
#define ZM_ERROR_INVALID_STATE               (ZM_ERROR_BASE_NUM + 8)  ///< Invalid state, operation disallowed in this state
#define ZM_ERROR_INVALID_LENGTH              (ZM_ERROR_BASE_NUM + 9)  ///< Invalid Length
#define ZM_ERROR_INVALID_FLAGS               (ZM_ERROR_BASE_NUM + 10) ///< Invalid Flags
#define ZM_ERROR_INVALID_DATA                (ZM_ERROR_BASE_NUM + 11) ///< Invalid Data
#define ZM_ERROR_DATA_SIZE                   (ZM_ERROR_BASE_NUM + 12) ///< Data size exceeds limit
#define ZM_ERROR_TIMEOUT                     (ZM_ERROR_BASE_NUM + 13) ///< Operation timed out
#define ZM_ERROR_NULL                        (ZM_ERROR_BASE_NUM + 14) ///< Null Pointer
#define ZM_ERROR_FORBIDDEN                   (ZM_ERROR_BASE_NUM + 15) ///< Forbidden Operation
#define ZM_ERROR_INVALID_ADDR                (ZM_ERROR_BASE_NUM + 16) ///< Bad Memory Address
#define ZM_ERROR_BUSY                        (ZM_ERROR_BASE_NUM + 17) ///< Busy


#if defined(__CC_ARM)
#define ZM_NEW_LINE     "\n"
#elif defined(__GNUC__)
#define ZM_NEW_LINE     "\n"
#elif defined(__ICCARM__)
#define ZM_NEW_LINE     "\n"
#endif


/*************************************************************************************************************************
 *                                                      CONSTANTS                                                        *
 *************************************************************************************************************************/
 
/*************************************************************************************************************************
 *                                                       TYPEDEFS                                                        *
 *************************************************************************************************************************/
/**
 * @brief   Aliases.
 *          Must be created here to satisfy module declaration order dependencies.
 */
typedef struct zm_cli zm_cli_t;
typedef struct cli_cmd_entry cli_cmd_entry_t;
typedef struct cli_def_entry cli_static_entry_t;
/**
 * @brief API Result.
 *
 * @details Indicates success or failure of an API procedure. In case of failure, a comprehensive
 *          error code indicating cause or reason for failure is provided.
 *
 *          Though called an API result, it could used in Asynchronous notifications callback along
 *          with asynchronous callback as event result. This mechanism is employed when an event
 *          marks the end of procedure initiated using API. API result, in this case, will only be
 *          an indicative of whether the procedure has been requested successfully.
 */
typedef uint32_t ret_code_t;

/**
 * Cli printf.
 */
typedef void (* _cli_printf)(zm_cli_t const * p_cli,zm_cli_vt100_color_t  color, char const * p_fmt, ...);

/**
 * @internal @brief Internal CLI state.
 */
typedef enum
{
    ZM_CLI_STATE_UNINITIALIZED,      //!< State uninitialized.
    ZM_CLI_STATE_INITIALIZED,        //!< State initialized but not active.
    ZM_CLI_STATE_ACTIVE,             //!< State active.
    ZM_CLI_STATE_PANIC_MODE_ACTIVE,  //!< State panic mode activated.
    ZM_CLI_STATE_PANIC_MODE_INACTIVE //!< State panic mode requested but not supported.
}cli_state_t;
    
/**
 * @internal @brief Internal CLI state in response to data received from the terminal.
 */
typedef enum
{
    ZM_CLI_RECEIVE_DEFAULT,
    ZM_CLI_RECEIVE_ESC,
    ZM_CLI_RECEIVE_ESC_SEQ,
    ZM_CLI_RECEIVE_TILDE_EXP
} cli_receive_t;

/**
 * @brief CLI dynamic command descriptor.
 *
 * @details Function shall fill the received @ref zm_cli_static_entry structure with requested (idx)
 *          dynamic subcommand data. If there is more than one dynamic subcommand available,
 *          the function shall ensure that the returned commands: p_static->p_syntax are sorted in
 *          alphabetical order. If idx exceeds the available dynamic subcommands, the function must write
 *          to p_static->p_syntax NULL value. This will indicate to the CLI module that
 *          there are no more dynamic commands to read.
 */
typedef void (*cli_dynamic_get)(size_t idx, cli_static_entry_t * p_static);

/**
 * Cli command handler prototype.
 */
typedef void (*cli_cmd_handler)(zm_cli_t const * p_cli, size_t argc, char **argv);
/**
 * Cli static command descriptor.
 */
struct cli_def_entry
{
    char const * m_syntax;  //!< Command syntax strings.
    char const * m_help;    //!< Command help string.
    cli_cmd_entry_t const * m_subcmd; //!< Pointer to subcommand.
    cli_cmd_handler m_handler;  //!< Command handler.
};

/*
 *
 */
struct cli_cmd_entry
{
    bool is_dynamic;
    union
    {
        cli_dynamic_get p_dynamic_get;  //!< Pointer to function returning dynamic commands.
        cli_static_entry_t const * m_static_entry;
    }u;
};
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
    int (* write)(const void *a_data, uint16_t length);
    
}cli_trans_api_t;

/**
 * Unified CLI transport interface.
 */
typedef struct
{
    cli_trans_api_t * m_cli_trans;
}cli_transport_t;

/**
 * @internal @brief Flags for internal CLI usage.
 */
typedef struct
{
    uint32_t insert_mode    : 1; //!< Enables or disables console insert mode for text introduction.
    uint32_t show_help      : 1; //!< Shows help if the command was called with -h or --help parameter.
    uint32_t use_colors     : 1; //!< Enables or disables colored syntax.
    uint32_t echo           : 1; //!< Enables or disables CLI echo.
    uint32_t processing     : 1; //!< CLI is executing process function.
    uint32_t tx_rdy         : 1;
    uint32_t tab            : 1;
    uint32_t last_nl        : 8; //!< The last received newline character.
} cli_flag_t;
STATIC_ASSERT(sizeof(cli_flag_t) == sizeof(uint32_t));
/**
 * @internal @brief Union for internal CLI usage.
 */
typedef union
{
    uint32_t value;
    cli_flag_t flag;
} cli_internal_t;
/**
 * Cli instance context.
 */
typedef struct
{
    cli_state_t state;  //!< Internal module state.
    cli_receive_t receive_state;    //!< Escape sequence indicator.
    
    cli_static_entry_t active_cmd;      //!< Currently executed command
    cli_cmd_len_t cmd_len; //!< Command length.
    cli_cmd_len_t cmd_cur_pos; //!< Command buffer cursor position.
#if ZM_MODULE_ENABLED(ZM_CLI_WILDCARD)
    cli_cmd_len_t cmd_tmp_buff_len;     //!< Command length in tmp buffer
#endif
    char cmd_buff[ZM_CLI_CMD_BUFF_SIZE];       //!< Command input buffer.
    char temp_buff[ZM_CLI_CMD_BUFF_SIZE];      //!< Temporary buffer used by various functions.
    char printf_buff[ZM_CLI_PRINTF_BUFF_SIZE]; //!< Printf buffer size.
    
    zm_cli_vt100_ctx_t vt100_ctx;          //!< VT100 color and cursor position, terminal width.
    
    volatile cli_internal_t internal;   //!< Internal CLI data
}cli_ctx_t;

typedef struct
{
    zm_printf_ctx_t *printf_ctx;
    _cli_printf printf;
}cli_printf_ctx_t;

typedef struct
{
    void *(* malloc)(size_t);
    void (* free)(void *);
}cli_memory_api_t;
/**
 * Cli history pool.
 */
typedef struct cli_hist_pool_T
{
    char * m_cmd;         //!< Pointer to string with cmd name.
    cli_cmd_len_t cmd_len;
    struct cli_hist_pool_T * m_next_hist; //!< Pointer to next cmd.
    struct cli_hist_pool_T * m_last_hist; //!< Pointer to last cmd.
}cli_hist_pool_t;
/**
 * Cli history.
 */
typedef struct
{
    uint8_t m_hist_num;   //!< number of history.
    cli_memory_api_t const * m_memory;
    cli_hist_pool_t * m_hist_head;    //!< Pointer to first history.
    cli_hist_pool_t * m_hist_tail;     //!< Pointer to the tail of history list.
    cli_hist_pool_t * m_hist_current; //!< Pointer to current history.
}cli_history_t;
  
struct zm_cli
{
    char const * const m_name; //!< Terminal name.
    cli_ctx_t * m_ctx;    //!< Internal context.
    cli_transport_t const * m_trans;    //!< Transport interface.
    cli_printf_ctx_t * m_printf_ctx;  //!< fprintf context.
    cli_history_t * m_cmd_hist;     //!< Memory reserved for commands history.
};


/**
 * @brief Option descriptor.
 */
typedef struct zm_cli_getopt_option
{
    char const * p_optname;         //!< Option long name.
    char const * p_optname_short;   //!< Option short name.
    char const * p_optname_help;    //!< Option help string.
} zm_cli_getopt_option_t;



/*************************************************************************************************************************
 *                                                  AFTER MACROS                                                         *
 *************************************************************************************************************************/
/**
 * @brief Macro for register cli transport api(@ref cli_trans_api_t).
 *
 * @param[in]   name            a command line interface instance.
 * @param[in]   trans_iface     struct cli_trans_api_t api.
 */
#define CLI_REGISTER_TRANS(name, trans_iface) \
        static cli_transport_t const CONCAT_2(name, _trans) = { \
            .m_cli_trans = &trans_iface, \
        }
        
#define CLI_HISTORY_DEF(name) \
        static const cli_memory_api_t CONCAT_2(name, _mem); \
        static cli_history_t CONCAT_2(name, _hist) = { \
            .m_memory = &CONCAT_2(name, _mem), \
        }; \
        static const cli_memory_api_t CONCAT_2(name, _mem) = { \
            .malloc = cli_malloc, \
            .free = cli_free, \
        };
        
        
/**
 * @brief Macro for defining a command line interface instance. 
 *
 * @param[in] name              Instance name.
 * @param[in] cli_prefix        CLI prefix string.
 */
#define ZM_CLI_DEF(name, cli_prefix, trans_iface) \
        static zm_cli_t const name; \
        CLI_REGISTER_TRANS(name, trans_iface); \
        static cli_ctx_t CONCAT_2(name, _ctx); \
        ZM_PRINTF_DEF(CONCAT_2(name, _fprintf_ctx),                           \
                        &name,                                                  \
                        CONCAT_2(name, _ctx).printf_buff,                       \
                        ZM_CLI_PRINTF_BUFF_SIZE,                               \
                        false,                                                  \
                        zm_cli_print_stream); \
        static cli_printf_ctx_t CONCAT_2(name, _print) = { \
            .printf_ctx = &CONCAT_2(name, _fprintf_ctx), \
        };\
        CLI_HISTORY_DEF(name); \
        static zm_cli_t const name = { \
            .m_name = cli_prefix, \
            .m_ctx = &CONCAT_2(name, _ctx), \
            .m_trans = &CONCAT_2(name, _trans), \
            .m_printf_ctx = &CONCAT_2(name, _print),\
            .m_cmd_hist = &CONCAT_2(name, _hist), \
        }
        



          
/**@brief   Macro for creating a cli section.
 *
 * @param[in]   section_name    Name of the section.
 * @param[in]   data_type       Data type of the variables to be registered in the section.
 */
#define CLI_SECTION_DEF(section_name, data_type) \
        ZM_SECTION_DEF(section_name, data_type)
        
/**
 * @brief Initializes a CLI command (@ref cli_static_entry_t).
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
        cli_static_entry_t const CONCAT_3(cli_, syntax, _raw) = \
            CLI_CMD_LOAD_PARA(syntax, subcmd, help, handler); \
        ZM_SECTION_ITEM_REGISTER(COMMAND_SECTION_NAME, \
                                 cli_cmd_entry_t const CONCAT_3(cli_, syntax, _const)) = { \
                                     .is_dynamic = false,\
                                     .u = {.m_static_entry = &CONCAT_3(cli_, syntax, _raw)} \
                                     };\
        ZM_SECTION_ITEM_REGISTER(PARA_SECTION_NAME, char const * CONCAT_2(syntax, _str_ptr))

/**
 * @brief Macro for creating a subcommand set. It must be used outside of any function body.
 *
 * @param[in] name  Name of the subcommand set.
 */        
#define CLI_CREATE_STATIC_SUBCMD_SET(name) \
        static cli_static_entry_t const CONCAT_2(name, _raw)[]; \
        static cli_cmd_entry_t const name = { \
            .is_dynamic = false, \
            .u = { \
                .m_static_entry = CONCAT_2(name, _raw)\
            } \
        };\
        static cli_static_entry_t const CONCAT_2(name, _raw)[] = 
          
/**
 * Define ending subcommands set.
 *
 */
#define CLI_SUBCMD_SET_END {NULL}
        

/**
 * @brief Option structure initializer @ref zm_cli_getopt_option.
 *
 * @param[in] _p_optname    Option name long.
 * @param[in] _p_shortname  Option name short.
 * @param[in] _p_help       Option help string.
 */
#define ZM_CLI_OPT(_p_optname, _p_shortname, _p_help) { \
        .p_optname       = _p_optname,   \
        .p_optname_short = _p_shortname, \
        .p_optname_help  = _p_help,      \
}

/**
 * @brief Print an info message to the CLI.
 *
 * See @ref zm_cli_fprintf.
 *
 * @param[in] _p_cli    Pointer to the CLI instance.
 * @param[in] _ft       Format string.
 * @param[in] ...       List of parameters to print.
 */
#define zm_cli_info(_p_cli, _ft, ...) \
        zm_cli_printf(_p_cli, CLI_INFO_COLOR, _ft "\n", ##__VA_ARGS__)

/**
 * @brief Print a normal message to the CLI.
 *
 * See @ref zm_cli_fprintf.
 *
 * @param[in] _p_cli    Pointer to the CLI instance.
 * @param[in] _ft       Format string.
 * @param[in] ...       List of parameters to print.
 */
#define zm_cli_print(_p_cli, _ft, ...) \
        zm_cli_printf(_p_cli, CLI_DEFAULT_COLOR, _ft "\n", ##__VA_ARGS__)

/**
 * @brief Print a warning message to the CLI.
 *
 * See @ref zm_cli_fprintf.
 *
 * @param[in] _p_cli    Pointer to the CLI instance.
 * @param[in] _ft       Format string.
 * @param[in] ...       List of parameters to print.
 */
#define zm_cli_warn(_p_cli, _ft, ...) \
        zm_cli_printf(_p_cli, ZM_CLI_WARNING, _ft "\n", ##__VA_ARGS__)
/**
 * @brief Print an error message to the CLI.
 *
 * See @ref zm_cli_fprintf.
 *
 * @param[in] _p_cli    Pointer to the CLI instance.
 * @param[in] _ft       Format string.
 * @param[in] ...       List of parameters to print.
 */
#define zm_cli_error(_p_cli, _ft, ...) \
        zm_cli_printf(_p_cli, ZM_CLI_ERROR, _ft "\n", ##__VA_ARGS__)
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
*     p_cli : CLI instance internals.
* OUTPUTS:
*     null
* RETURNS:
*     state
* NOTE:
*     null
*****************************************************************/
int zm_cli_init(zm_cli_t const * p_cli);
ret_code_t zm_cli_start(zm_cli_t const * p_cli);
ret_code_t zm_cli_stop(zm_cli_t const * p_cli);
void zm_cli_process(zm_cli_t const * p_cli);
void zm_cli_printf(zm_cli_t const *      p_cli,
                   zm_cli_vt100_color_t  color,
                   char const *          p_fmt,
                                         ...);
void zm_cli_print_stream(void const * p_user_ctx, char const * p_data, size_t data_len);
void zm_cli_help_print(zm_cli_t const *               p_cli,
                    zm_cli_getopt_option_t const * p_opt,
                    size_t                          opt_len);
                    
static inline void print_usage(zm_cli_t const * p_cli,
                               const char * p_command,
                               const char * p_help_string)
{
    zm_cli_help_print(p_cli, NULL, 0);
    zm_cli_printf(p_cli, ZM_CLI_NORMAL, "Usage:\r\n");
    zm_cli_printf(p_cli, ZM_CLI_NORMAL, "   %s %s\r\n", p_command, p_help_string);
}
/**
 * memory management function. 
 * They're weak functions, you can redefine them.
 */
void *cli_malloc(size_t size);
void cli_free(void *p);
                    
                    
     
#ifdef __cplusplus
}
#endif
#endif /* zm_cli.h */
