/*****************************************************************
* Copyright (C) 2020 ZM Technology Personal.                     *
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
    
    
// <o> ZM_CLI_CMD_BUFF_SIZE - Maximum buffer size for a single command. 
#ifndef ZM_CLI_CMD_BUFF_SIZE
#define ZM_CLI_CMD_BUFF_SIZE 256
#endif
    
// <o> ZM_CLI_PRINTF_BUFF_SIZE - Maximum print buffer size. 
#ifndef ZM_CLI_PRINTF_BUFF_SIZE
#define ZM_CLI_PRINTF_BUFF_SIZE 23
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
