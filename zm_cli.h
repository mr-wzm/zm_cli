#ifndef __ZM_CLI_H__
#define __ZM_CLI_H__

#include "zm_section.h"

/**
 * @brief CLI command handler prototype.
 */
//typedef void (*cli_cmd_handler)(nrf_cli_t const * p_cli, size_t argc, char **argv);


typedef struct cli_def_att_T
{
    char const * m_syntax;  //!< Command syntax strings.
    char const * m_help;    //!< Command help string.
    struct cli_def_att_T const * m_subcmd; //!< Pointer to subcommand.
}cli_def_att_t;


#define CLI_SECTION_DEF(section_name, data_type) \
        SECTION_ITEM_REGISTER(CONCAT_2(section_name, _s0.end), const data_type CONCAT_2(section_name, $$Base)) = (const data_type){0};\
        SECTION_ITEM_REGISTER(CONCAT_2(section_name, _s1.end), const data_type CONCAT_2(section_name, $$Limit)) = (const data_type){0}

#define CLI_CMD_REGISTER
        

#endif

