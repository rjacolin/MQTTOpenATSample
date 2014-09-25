/**********************************************************************************************************************/
/*  cfg_uart_client.c    -  Copyright Sierra Wireless Inc. (c) 2010                                                   */
/*                                                                                                                    */
/*                                                                                                                    */
/* LICENSE                                                                                                            */
/* =======                                                                                                            */
/* If no specific agreement has been signed, this Source Code is subject to the following license terms:              */
/* http://www.sierrawireless.com/productsandservices/AirPrime/Sierra_Wireless_Software_Suite/License/Userlicense.aspx */
/* If you modify this file, you are subject to the contribution rules described into this license                     */
/**********************************************************************************************************************/ 

/***************************************************************************/
/*  File       : cfg_uart_client.c            
 *
 *  Object     : PPP UART client bearer configuration:
 *
 *  [PPP_BEARER]:    UART Id
 *  [PPP_USER]:      login name
 *  [GPRS_PASSWORD]: password
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
#define PPP_BEARER      "UART2"
#define PPP_USER        "wipuser"
#define PPP_PASSWORD    "WU#passwd"

#define NORMAL_TRACE_LEVEL      4
#define ERROR_TRACE_LEVEL       1

#define ASSERT( pred)   if( !(pred)) TRACE(( ERROR_TRACE_LEVEL, "ASSERTION FAILURE line %i: " #pred "\n", __LINE__))
#define ASSERT_OK( v)   ASSERT( 0 == (v))


/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/
/* Function to be called when the bearer is connected */
static void ( * AppliEntryPoint ) ( void );

/*****************************************************************************/
/*  Initialization-related event handlers                                    */
/*****************************************************************************/
static void cbEvhBearer ( wip_bearer_t b, s8 event, void *ctx );

/*****************************************************************************/
/*  Function   : cbEvhBearer                                                 */
/*---------------------------------------------------------------------------*/
/*  Object     : bearer events handler: when the bearer connection is        */
/*               completed, start IP services                                */
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
        AppliEntryPoint ();
    }
}

/*****************************************************************************/
/*  Function   : CfgUartPppClient                                            */
/*---------------------------------------------------------------------------*/
/*  Object     : initialize PPP client connection, then launch               */
/*               EntryPoint() on success                                     */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  EntryPoint        | X |   |   |Function run after successful connection  */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
void CfgUartPppClient ( void(* EntryPoint) ( void ) )
{
    s8 bearerRet;
    wip_bearer_t br;

    TRACE( ( NORMAL_TRACE_LEVEL, "(CfgUartPppClient) Enter." ) );
    AppliEntryPoint = EntryPoint;

    /* Open UART bearer */
    bearerRet = wip_bearerOpen ( &br, PPP_BEARER, cbEvhBearer, NULL );
    ASSERT_OK ( bearerRet );

    /* Set bearer configuration */
    bearerRet = wip_bearerSetOpts ( br,
                                    WIP_BOPT_LOGIN, PPP_USER,
                                    WIP_BOPT_PASSWORD, PPP_PASSWORD,
                                    WIP_BOPT_END );
    ASSERT_OK ( bearerRet );

    /* Start the bearer */
    bearerRet = wip_bearerStart ( br );
    ASSERT ( OK == bearerRet || WIP_BERR_OK_INPROGRESS == bearerRet);
}

