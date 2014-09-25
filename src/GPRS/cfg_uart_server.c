/**********************************************************************************************************************/
/*  cfg_uart_server.c    -  Copyright Sierra Wireless Inc. (c) 2010                                                   */
/*                                                                                                                    */
/*                                                                                                                    */
/* LICENSE                                                                                                            */
/* =======                                                                                                            */
/* If no specific agreement has been signed, this Source Code is subject to the following license terms:              */
/* http://www.sierrawireless.com/productsandservices/AirPrime/Sierra_Wireless_Software_Suite/License/Userlicense.aspx */
/* If you modify this file, you are subject to the contribution rules described into this license                     */
/**********************************************************************************************************************/ 

/***************************************************************************/
/*  File       : cfg_uart_server.c            
 *
 *  Object     : PPP GSM DATA server bearer configuration:
 *
 *  [PPP_BEARER]:        UART Id
 *  [PPP_USER]:          login name
 *  [PPP_PASSWORD]:      password
 *  [PPP_LOCAL_STRADDR]: Module's address
 *  [PPP_DEST_STRADDR]:  PPP client's address
 */
/***************************************************************************/


/*****************************************************************************/
/*  Header files                                                             */
/*****************************************************************************/

#include "adl_global.h"
#include "wip.h"

/***************************************************************************/
/*  Defines                                                                */
/***************************************************************************/
#define PPP_BEARER          "UART2"
#define PPP_USER            "wipuser"
#define PPP_PASSWORD        "WU#passwd"
#define PPP_LOCAL_STRADDR   "192.168.1.4"   /* Module's address     */
#define PPP_DEST_STRADDR    "192.168.1.5"   /* PPP client's address */

#define NORMAL_TRACE_LEVEL      4
#define ERROR_TRACE_LEVEL       1

#define ASSERT( pred) if( !(pred)) TRACE(( ERROR_TRACE_LEVEL, "ASSERTION FAILURE line %i: " #pred "\n", __LINE__))
#define ASSERT_OK( v) ASSERT( 0 == (v))


/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/
/* Function to be called when the bearer is connected */
static void (* AppliEntryPoint) ( void );

/*****************************************************************************/
/*  Initialization-related event handlers                                    */
/*****************************************************************************/
static s8 cbEvhPppAuth ( wip_bearer_t b, wip_bearerServerEvent_t *ev, void *ctx );
static void cbEvhBearer ( wip_bearer_t b, s8 event, void *ctx );

/*****************************************************************************/
/*  Function   : EvhPppAuth                                                  */
/*---------------------------------------------------------------------------*/
/*  Object     : ppp authentication events: in PPP server mode, when         */
/*               a client authenticates itself, fill the password so that    */
/*               it can be checked                                           */
/*                                                                           */
/*  Return     : 0 or 1 based on the authentication events                   */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  b                 | X |   |   | bearer identifier                        */
/*--------------------+---+---+---+------------------------------------------*/
/*  ev                | X |   |   | bearer event                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  ctx               | X |   |   | passed context                           */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static s8 cbEvhPppAuth ( wip_bearer_t b, wip_bearerServerEvent_t *ev, void *ctx )
{

    if ( WIP_BEV_PPP_AUTH_PEER == ev->kind )
    {
        /* Make sure the user name is [PPP_USER] */
        if ( strncmp ( ev->content.ppp_auth.user, PPP_USER, ev->content.ppp_auth.userlen ) )
        {
            return OK;
        }
        /* Fill in expected password. */
        ev->content.ppp_auth.secret = PPP_PASSWORD;
        ev->content.ppp_auth.secretlen = sizeof ( PPP_PASSWORD ) - 1;
        return 1;
    }

    return OK;
}

/*****************************************************************************/
/*  Function   : cbEvhBearer                                                 */
/*---------------------------------------------------------------------------*/
/*  Object     : bearer events handler: when the bearer connection           */
/*                is completed, start application                            */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  b                 | X |   |   | bearer identifier                        */
/*--------------------+---+---+---+------------------------------------------*/
/*  event             | X |   |   | bearer event                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  ctx               | X |   |   | passed context                           */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static void cbEvhBearer ( wip_bearer_t b, s8 event, void *ctx )
{
    if ( WIP_BEV_IP_CONNECTED == event )
    {
        AppliEntryPoint ( );
    }
}

/*****************************************************************************/
/*  Function   : CfgUartPppServ                                              */
/*---------------------------------------------------------------------------*/
/*  Object     : initialize PPP server, then launch EntryPoint() upon        */
/*               successful client connection.                               */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  EntryPoint        | X |   |   |Function run after successful connection  */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
void CfgUartPppServ ( void(*EntryPoint) ( void ) )
{
    s8 bearerRet;
    wip_in_addr_t localAddr, destAddr;
    wip_bearer_t br;

    TRACE ( ( NORMAL_TRACE_LEVEL, "(CfgUartPppServ) Enter." ) );
    AppliEntryPoint = EntryPoint;

    wip_inet_aton ( PPP_LOCAL_STRADDR, &localAddr );
    wip_inet_aton ( PPP_DEST_STRADDR, &destAddr );

    /* Open the UART bearer */
    bearerRet = wip_bearerOpen ( &br, PPP_BEARER, cbEvhBearer, NULL );
    ASSERT_OK ( bearerRet );

    /* Set bearer configuration
     * if Open AT application on WCPU aims at addressing an other IP @
     * than PPP peer or 127.0.0.1, it has to enable the IP Gateway
     * functionality. This is done using WIP_BOPT_IP_SETGW at TRUE
     */
    bearerRet = wip_bearerSetOpts ( br,
                                    WIP_BOPT_IP_ADDR, localAddr,
                                    WIP_BOPT_IP_DST_ADDR, destAddr,
                                    WIP_BOPT_IP_SETDNS, FALSE,
                                    WIP_BOPT_IP_SETGW, TRUE,
                                    WIP_BOPT_RESTART, FALSE,
                                    WIP_BOPT_END );
    ASSERT_OK ( bearerRet );

    /* Start the bearer server */
    bearerRet = wip_bearerStartServer ( br, cbEvhPppAuth, NULL );
    ASSERT_OK ( bearerRet );
}

