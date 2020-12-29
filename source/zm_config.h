/*****************************************************************
* Copyright (C) 2020 WangZiming. All rights reserved.            *
******************************************************************
* zm_config.h
*
* DESCRIPTION:
*     zm cli config
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
#ifndef __ZM_CONFIG_H__
#define __ZM_CONFIG_H__
#ifdef __cplusplus
extern "C"
{
#endif

#define ZM_ENABLE    1
#define ZM_DISABLE   0


    
// <o> ZM_CLI_ARGC_MAX - Maximum number of parameters passed to the command handler. 
#ifndef ZM_CLI_ARGC_MAX
#define ZM_CLI_ARGC_MAX 12
#endif

// <o> ZM_CLI_CMD_BUFF_SIZE - Maximum buffer size for a single command. 
#ifndef ZM_CLI_CMD_BUFF_SIZE
#define ZM_CLI_CMD_BUFF_SIZE 256
#endif
    
// <o> ZM_CLI_PRINTF_BUFF_SIZE - Maximum print buffer size. 
#ifndef ZM_CLI_PRINTF_BUFF_SIZE
#define ZM_CLI_PRINTF_BUFF_SIZE 23
#endif
// <q> ZM_FPRINTF_FLAG_AUTOMATIC_CR_ON_LF_ENABLED  - For each printed LF, function will add CR.
#ifndef ZM_FPRINTF_FLAG_AUTOMATIC_CR_ON_LF_ENABLED
#define ZM_FPRINTF_FLAG_AUTOMATIC_CR_ON_LF_ENABLED ZM_ENABLE
#endif

// <q> ZM_CLI_METAKEYS_ENABLED  - Enable additional control keys for CLI commands like ctrl+a, ctrl+e, ctrl+w, ctrl+u
#ifndef ZM_CLI_METAKEYS_ENABLED
#define ZM_CLI_METAKEYS_ENABLED ZM_ENABLE
#endif

// <e> ZM_CLI_HISTORY_ENABLED - Enable CLI history mode.
//==========================================================
#ifndef ZM_CLI_HISTORY_ENABLED
#define ZM_CLI_HISTORY_ENABLED 1
#endif

// <o> ZM_CLI_HISTORY_SAVE_ITEM_NUM - Number of history memory objects.
#if ZM_CLI_HISTORY_ENABLED == ZM_ENABLE
#ifndef ZM_CLI_HISTORY_SAVE_ITEM_NUM
#define ZM_CLI_HISTORY_SAVE_ITEM_NUM 30
#endif
#endif

// <q> ZM_PRINT_VT100_COLORS_ENABLED  - CLI VT100 colors.
 
#ifndef ZM_PRINT_VT100_COLORS_ENABLED
#define ZM_PRINT_VT100_COLORS_ENABLED ZM_ENABLE
#endif


#define ASSERT(expr)                                                          \
do{                                                                           \
    if (expr)                                                                 \
    {                                                                         \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      /*printf("error in %s--%d line\r\n", __FILE__, __LINE__);*/                 \
    }                                                                         \
}while(0)
    

#ifdef __cplusplus
}
#endif
#endif /* zm_config.h */
