/**********************************************************************************************************************/
/*  cfg_gsm_client.c     -  Copyright Sierra Wireless Inc. (c) 2010                                                   */
/*                                                                                                                    */
/*                                                                                                                    */
/* LICENSE                                                                                                            */
/* =======                                                                                                            */
/* If no specific agreement has been signed, this Source Code is subject to the following license terms:              */
/* http://www.sierrawireless.com/productsandservices/AirPrime/Sierra_Wireless_Software_Suite/License/Userlicense.aspx */
/* If you modify this file, you are subject to the contribution rules described into this license                     */
/**********************************************************************************************************************/

/*****************************************************************************/
/*  File       : cfg_gsm_client.c                                            */
/*                                                                           */
/*  Object     : PPP GSM DATA client bearer configuration:                   */
/*                                                                           */
/*  [REMOTE_GSM_DIALNB]: Remote number to call                               */
/*  [PPP_USER]:          login name                                          */
/*  [PPP_PASSWORD]:      password                                            */
/*  [GSM_PINCODE]:       PIN code of the SIM card, or NULL if not required   */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Header files                                                             */
/*****************************************************************************/

#include "adl_global.h"
#include "wip.h"

/***************************************************************************/
/*  Defines                                                                */
/***************************************************************************/

#define PPP_USER            "wipuser"       /* Expected user name         */
#define PPP_PASSWORD        "WU#passwd"     /* Password server checks     */
#define GSM_PINCODE         "0000"          /* GSM pin code               */
#define REMOTE_GSM_DIALNB   "0615546937"    /* Remote GSM module phone nb */
#define GSM_BEARER          "GSM"           /* GSM bearer name            */

#define NORMAL_TRACE_LEVEL      4
#define ERROR_TRACE_LEVEL       1

#define ASSERT( pred)   if( !(pred)) TRACE(( ERROR_TRACE_LEVEL, "ASSERTION FAILURE line %i: " #pred "\n", __LINE__))
#define ASSERT_OK( v)   ASSERT( 0 == (v))


/***************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/
/* Function to be called when the bearer is connected */
static void ( * AppliEntryPoint ) ( void ) ;

/*****************************************************************************/
/*  Initialization-related event handlers                                  */
/*****************************************************************************/
static void cbEvhSim ( u8 event ) ;
static void cbEvhBearer ( wip_bearer_t b, s8 event, void *ctx ) ;

/*****************************************************************************/
/*  Function   : cbEvhSim                                                    */
/*---------------------------------------------------------------------------*/
/*  Object     : sim events:                                               */
/*               when SIM initialization is completed, open the GSM          */
/*               client bearer                                             */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  event             | X |   |   | SIM event                              */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static void cbEvhSim ( u8 event )
{
    s8 bearerRet;
    wip_bearer_t br;

    if ( ADL_SIM_EVENT_FULL_INIT != event )   return;
    
    TRACE ( ( NORMAL_TRACE_LEVEL, "(cbEvhSim) Enter." ) ) ;

    /* Open GSM bearer */
    bearerRet = wip_bearerOpen ( &br, GSM_BEARER, cbEvhBearer, NULL ) ;
    ASSERT_OK ( bearerRet ) ;

    /* Set the bearer configuration */
    bearerRet = wip_bearerSetOpts ( br, 
                            WIP_BOPT_LOGIN, PPP_USER,
                            WIP_BOPT_PASSWORD, PPP_PASSWORD,
                            WIP_BOPT_DIAL_PHONENB, REMOTE_GSM_DIALNB,
                            WIP_BOPT_END );

    ASSERT_OK ( bearerRet );

    /* Start the bearer */
    bearerRet = wip_bearerStart ( br );
    ASSERT ( OK == bearerRet || WIP_BERR_OK_INPROGRESS == bearerRet);
}

/*****************************************************************************/
/*  Function   : cbEvhBearer                                                 */
/*---------------------------------------------------------------------------*/
/*  Object     : bearer events handler:                                    */
/*               when the bearer connection is completed,                  */
/*               start IP services                                         */
/*                                                                         */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  b                 | X |   |   | bearer identifier                      */
/*  event             | X |   |   | bearer event type                      */
/*  ctx               |   |   |   | unused                                 */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static void cbEvhBearer ( wip_bearer_t b, s8 event, void *ctx )
{
    if ( WIP_BEV_IP_CONNECTED == event )
    {
        AppliEntryPoint ( ) ;
    }
}

/*****************************************************************************/
/*  Function   : CfgGsmPppClient                                             */
/*---------------------------------------------------------------------------*/
/*  Object     : initialize PPP client connection, then launch             */
/*               EntryPoint() on success                                     */
/*                                                                         */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  EntryPoint        | X |   |   |Function run after successful connection  */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
void CfgGsmPppClient ( void ( * EntryPoint ) ( void ) )
{
    TRACE ( ( NORMAL_TRACE_LEVEL, "(CfgGsmPppClient) Enter." ) ) ;
    AppliEntryPoint = EntryPoint;
    adl_simSubscribe ( cbEvhSim, GSM_PINCODE ) ;
}

