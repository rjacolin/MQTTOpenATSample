/**********************************************************************************************************************/
/*  appli.c   -  Copyright Sierra Wireless Inc. (c) 2010                                                              */
/*                                                                                                                    */
/*                                                                                                                    */
/* LICENSE                                                                                                            */
/* =======                                                                                                            */
/* If no specific agreement has been signed, this Source Code is subject to the following license terms:              */
/* http://www.sierrawireless.com/productsandservices/AirPrime/Sierra_Wireless_Software_Suite/License/Userlicense.aspx */
/* If you modify this file, you are subject to the contribution rules described into this license                     */
/**********************************************************************************************************************/

/*****************************************************************************/
/*  File       : appli.c                                                     */
/*                                                                           */
/*  This is the main ADL entry point:                                        */
/*    Before WIP application AppliEntryPoint() is called, any or none        */
/*    of the flags could be activated for a bearer type configuration:       */
/*                                                                           */
/*    The TCP/IP stack will be initialized over a :                          */
/*                                                                           */
/*    [OVER_UART_PPP_SERV]:   PPP UART server connection mode                */
/*    [OVER_UART_PPP_CLIENT]: PPP UART client connection mode                */
/*    [OVER_GSM_PPP_SERV]:    PPP GSM DATA server connection mode            */
/*    [OVER_GSM_PPP_CLIENT]:  PPP GSM DATA client connection mode            */
/*    [OVER_GPRS]:            GPRS connection mode                           */
/*    [ETHERNET_BEARER_USED]: Other case where ETHERNET driver is used       */
/*****************************************************************************/


/*****************************************************************************/
/*  Header files                                                             */
/*****************************************************************************/

#include "adl_global.h"
#include "wip.h"

/*****************************************************************************/
/*  Defines                                                                  */
/*****************************************************************************/
 
#define NORMAL_TRACE_LEVEL      4
#define ERROR_TRACE_LEVEL       1
 
/* ----------------------------------------------- */
/* Activate any or none of the concerned following */
/* flags for the proper case configuration.        */
/* ----------------------------------------------- */

/* #define OVER_UART_PPP_SERV           */
/* #define OVER_UART_PPP_CLIENT         */
/* #define OVER_GSM_PPP_SERV            */
/* #define OVER_GSM_PPP_CLIENT          */
/* #define OVER_ETHERNET                */
#define OVER_GPRS
/* #define ETHERNET_BEARER_USED         */

/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/
extern void CfgUartPppClient ( void ( *EntryPoint ) ( void ) ) ;
extern void CfgUartPppServ ( void ( *EntryPoint ) ( void ) ) ;
extern void CfgGsmPppClient ( void ( *EntryPoint ) ( void ) ) ;
extern void CfgGsmPppServ ( void ( *EntryPoint ) ( void ) ) ;
extern void CfgGprs ( void ( *EntryPoint ) ( void ) ) ;
extern void CfgEth ( void (* EntryPoint) ( void ) );
extern void MainTask ( void );

extern void whenNetworkUp ( void );

/***************************************************************************/
/*  Mandatory Stack size Declaration                                       */
/*-------------------------------------------------------------------------*/
/*  No more WIP version < 4.00 supported                                   */
/*-------------------------------------------------------------------------*/
#ifdef __GNU_GCC__
/*The GCC compiler and GNU Newlib (standard C library) implementation
 require more stack size than ARM compilers. If the GCC compiler is used,
 the Open AT® application has to be declared with greater stack sizes.*/
#define DECLARE_CALL_STACK(X)       (X*3)
#define DECLARE_LOWIRQ_STACK(X)     const u32 wm_apmIRQLowLevelStackSize = X*3
#define DECLARE_HIGHIRQ_STACK(X)    const u32 wm_apmIRQHighLevelStackSize = X*3
#else /* #ifndef __GNU_GCC__ */
#define DECLARE_CALL_STACK(X)       (X)
#define DECLARE_LOWIRQ_STACK(X)     const u32 wm_apmIRQLowLevelStackSize = X
#define DECLARE_HIGHIRQ_STACK(X)    const u32 wm_apmIRQHighLevelStackSize = X
#endif /* #ifndef __GNU_GCC__ */
/* Call stack size declaration */
/***************************************************************************/

#if  defined ( ETHERNET_BEARER_USED ) || defined ( OVER_ETHERNET )
/* Ethernet driver uses high level interrupt handler */
DECLARE_LOWIRQ_STACK ( 1024 );
DECLARE_HIGHIRQ_STACK ( 1024 );
#endif

/************************************************************************************/
/* Task table definition of tasks entry point, stack size, task name, task priority */
/************************************************************************************/
const adl_InitTasks_t adl_InitTasks [ ] =
{
      { MainTask, DECLARE_CALL_STACK(8192), "MainTask", 1 },
      { 0, 0, 0, 0 }
};


/***************************************************************************/
/*  Function   : MainTask                                                  */
/*-------------------------------------------------------------------------*/
/*  Object     : Customer application initialisation                       */
/*                                                                         */
/*  Return     : None                                                      */
/*-------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilisation                           */
/*--------------------+---+---+---+----------------------------------------*/
/*  InitType          | x |   |   |  Application start mode reason         */
/*--------------------+---+---+---+----------------------------------------*/
/***************************************************************************/
void MainTask ( void )
{
    s8 ret;

    TRACE ( ( NORMAL_TRACE_LEVEL, "Embedded Application : Main" ) );

    /* Initialize the stack */
    ret = wip_netInitOpts ( WIP_NET_OPT_DEBUG_PORT, WIP_NET_DEBUG_PORT_UART1, WIP_NET_OPT_END );

    adl_InitType_e type = adl_InitGetType();

    wip_debug("Type: %d\n\r", type);

    if ( OK == ret )
    {
        /* Depending on the activated flag, take the proper config */
#if defined OVER_UART_PPP_SERV
        /* Initialize PPP server over UART */
    CfgUartPppServ( whenNetworkUp ) ;

#elif defined OVER_UART_PPP_CLIENT
        /* Initialize PPP client over UART */
    CfgUartPppClient( whenNetworkUp ) ;

#elif defined OVER_GSM_PPP_SERV
        /* Initialize PPP server over GSM  */
    CfgGsmPppServ( whenNetworkUp ) ;

#elif defined OVER_GSM_PPP_CLIENT
        /* Initialize PPP client over GSM  */
    CfgGsmPppClient( whenNetworkUp ) ;

#elif defined OVER_GPRS
        /* Initialize GPRS connection      */
    CfgGprs ( whenNetworkUp ) ;

#elif defined OVER_ETHERNET
        /* initialize Ethernet connection */
    CfgEth( whenNetworkUp ) ;

#else
    /* Don't initialize any bearer; only        */
    /* localhost=127.0.0.1 will be reachable.   */
    whenNetworkUp ( );

#endif
    }
}

