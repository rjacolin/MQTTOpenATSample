/**********************************************************************************************************************/
/*  cfg_gsm_server.c    -  Copyright Sierra Wireless Inc. (c) 2010                                                     */
/*                                                                                                                    */
/*                                                                                                                    */
/* LICENSE                                                                                                            */
/* =======                                                                                                            */
/* If no specific agreement has been signed, this Source Code is subject to the following license terms:              */
/* http://www.sierrawireless.com/productsandservices/AirPrime/Sierra_Wireless_Software_Suite/License/Userlicense.aspx */
/* If you modify this file, you are subject to the contribution rules described into this license                     */
/**********************************************************************************************************************/

/*****************************************************************************/
/*  File       : cfg_gsm_server.c                                            */
/*                                                                           */
/*  Object     : PPP GSM DATA server bearer configuration:                   */
/*                                                                           */
/*  [PPP_USER]:          login name                                          */
/*  [PPP_PASSWORD]:      password                                            */
/*  [PPP_LOCAL_STRADDR]: Module's address                                    */
/*  [PPP_DEST_STRADDR]:  PPP client's address                                */
/*  [GSM_REMOTE_DIALER]: Remote GSM caller number to check                   */
/*  [GSM_PINCODE]:       PIN code of the SIM card, or NULL if not required   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Header files                                                             */
/*****************************************************************************/

#include "adl_global.h"
#include "wip.h"

/*****************************************************************************/
/*  Defines                                                                  */
/*****************************************************************************/
#define PPP_USER            "wipuser"       /* Expected user name       */
#define PPP_PASSWORD        "WU#passwd"     /* Password server checks   */
#define PPP_LOCAL_STRADDR   "192.168.1.4"   /* Module's address         */
#define PPP_DEST_STRADDR    "192.168.1.5"   /* PPP client's address     */
#define GSM_REMOTE_DIALER   "0612019964"    /* Remote GSM caller number */
#define GSM_PINCODE         "8464"          /* GSM pin code             */
#define GSM_BEARER          "GSM"           /* GSM bearer name          */

#define NORMAL_TRACE_LEVEL      4
#define ERROR_TRACE_LEVEL       1

#define ASSERT( pred) if( !(pred)) TRACE((ERROR_TRACE_LEVEL, "ASSERTION FAILURE line %i: " #pred "\n", __LINE__))
#define ASSERT_OK( v) ASSERT( 0 == (v))


/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/
/* Function to be called when the bearer is connected */
static void (* AppliEntryPoint) ( void );

/*****************************************************************************/
/*  Initialization-related event handlers                                    */
/*****************************************************************************/
static void cbEvhSim ( u8 event );
static s8 cbEvhPppAuth ( wip_bearer_t b, wip_bearerServerEvent_t *ev, void *ctx );
static void cbEvhBearer ( wip_bearer_t b, s8 event, void *ctx );

/*****************************************************************************/
/*  Function   : cbEvhSim                                                    */
/*---------------------------------------------------------------------------*/
/*  Object     : sim events:                                                 */
/*               when SIM initialisation is completed, open the GSM          */
/*               server bearer                                               */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  event             | X |   |   | SIM event                                */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static void cbEvhSim ( u8 event )
{
    s8 bearerRet;
    wip_in_addr_t localAddr, destAddr;
    wip_bearer_t br;

    TRACE ( ( NORMAL_TRACE_LEVEL, "(cbEvhSim) Enter." ) );
    if ( ADL_SIM_EVENT_FULL_INIT != event )
    {
        return;
    }

    wip_inet_aton ( PPP_LOCAL_STRADDR, &localAddr );
    wip_inet_aton ( PPP_DEST_STRADDR, &destAddr );

    /* Open the GSM bearer */
    bearerRet = wip_bearerOpen ( &br, GSM_BEARER, cbEvhBearer, NULL );
    ASSERT_OK ( bearerRet);

    /* Set the bearer configuration */
    bearerRet = wip_bearerSetOpts ( br,
                                    WIP_BOPT_IP_ADDR, localAddr,
                                    WIP_BOPT_IP_DST_ADDR, destAddr,
                                    WIP_BOPT_IP_SETDNS, FALSE,
                                    WIP_BOPT_IP_SETGW, FALSE,
                                    WIP_BOPT_RESTART, TRUE,
                                    WIP_BOPT_END );
    ASSERT_OK ( bearerRet );

    /* Start the GSM server */
    bearerRet = wip_bearerStartServer ( br, cbEvhPppAuth, NULL );
    ASSERT_OK ( bearerRet );
}

/*****************************************************************************/
/*  Function   : cbEvhPppAuth                                                */
/*---------------------------------------------------------------------------*/
/*  Object     : ppp authentication events: in PPP server mode, when a       */
/*               client authenticates itself, fill the password              */
/*               so that it can be checked.                                  */
/*                                                                           */
/*  Return     : 0 or 1 based on the authentication events                   */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  b                 | X |   |   | bearer identifier                        */
/*--------------------+---+---+---+------------------------------------------*/
/*  event             | X |   |   | bearer event type                        */
/*--------------------+---+---+---+------------------------------------------*/
/*  ctx               | X |   |   | unused                                   */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static s8 cbEvhPppAuth ( wip_bearer_t b, wip_bearerServerEvent_t *ev, void *ctx )
{

    if ( WIP_BEV_DIAL_CALL == ev->kind )
    {
        /* Make sure that we're called by [GSM_REMOTE_DIALER], not by
         * some random hacker... */
        ev->content.dial_call.phonenb = GSM_REMOTE_DIALER;
        return 1;
    }

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
/*  Function   :  cbEvhBearer                                                */
/*---------------------------------------------------------------------------*/
/*  Object     : bearer events handler: when the bearer connection           */
/*               is completed, start IP services                             */
/*               (used both in PPP and GPRS modes).                          */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  b                 | X |   |   | bearer identifier                        */
/*--------------------+---+---+---+------------------------------------------*/
/*  event             | X |   |   | bearer event type                        */
/*--------------------+---+---+---+------------------------------------------*/
/*  ctx               | X |   |   | unused                                   */
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
/*  Function   : CfgGsmPppServ                                               */
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
void CfgGsmPppServ ( void(*EntryPoint) ( void ) )
{
    TRACE ( ( NORMAL_TRACE_LEVEL, "(CfgGsmPppServ) Enter." ) );
    AppliEntryPoint = EntryPoint;
    adl_simSubscribe ( cbEvhSim, GSM_PINCODE );
}

