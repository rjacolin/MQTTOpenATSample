/**********************************************************************************************************************/
/*  cfg_gprs.c     -  Copyright Sierra Wireless Inc. (c) 2010                                                         */
/*                                                                                                                    */
/*                                                                                                                    */
/* LICENSE                                                                                                            */
/* =======                                                                                                            */
/* If no specific agreement has been signed, this Source Code is subject to the following license terms:              */
/* http://www.sierrawireless.com/productsandservices/AirPrime/Sierra_Wireless_Software_Suite/License/Userlicense.aspx */
/* If you modify this file, you are subject to the contribution rules described into this license                     */
/**********************************************************************************************************************/

/***************************************************************************/
/*  File       : cfg_gprs.c                                                
 *
 *  Object     : GPRS bearer configuration:
 *
 *  [GPRS_APN]:      Network provider APN
 *  [GPRS_USER]:     login name
 *  [GPRS_PASSWORD]: password
 *  [GPRS_PINCODE]:  PIN code of the SIM card, or NULL if not required
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
#define GPRS_APN        "orange.m2m.spec"
#define GPRS_USER       "user"
#define GPRS_PASSWORD   "password"
#define GPRS_PINCODE    "0000"
#define GPRS_BEARER     "GPRS"

#define REG_STATE_REG   1
#define REG_STATE_ROAM  5

/* in 100ms steps */
#define CREG_POLLING_PERIOD     20

#define NORMAL_TRACE_LEVEL      4
#define ERROR_TRACE_LEVEL       1

#define ASSERT( pred )  if( !(pred)) TRACE(( ERROR_TRACE_LEVEL, "ASSERTION FAILURE line %i: " #pred "\n", __LINE__))
#define ASSERT_OK( v )  ASSERT( OK == (v))


/*****************************************************************************/
/*  Prototypes                                                               */
/*****************************************************************************/

/* Function to be called once the bearer is up and running. */
static void ( * AppliEntryPoint) ( void ) ;

static void PollCreg ( u8 Id );
static void OpenAndStartBearer ( void ) ;

/*****************************************************************************/
/*  Initialization-related event handlers                                    */
/*****************************************************************************/
static void cbEvhSim ( u8 event ) ;
static bool cbPollCreg ( adl_atResponse_t *Rsp ) ;
static void cbEvhBearer ( wip_bearer_t b, s8 event, void *ctx ) ;

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
/*  event             | X |   |   | bearer event type                        */
/*--------------------+---+---+---+------------------------------------------*/
/*  ctx               |   |   |   | unused                                   */
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
/*  Function   : OpenAndStartBearer()                                        */
/*---------------------------------------------------------------------------*/
/*  Object : Open and start the GPRS bearer. Normally, the bearer will       */
/*           answer IN_PROGRESS, and the initialization will be finished     */
/*           by the callback cbEvhBearer.                                    */
/*                                                                           */
/*  Return : None                                                            */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static void OpenAndStartBearer ( void )
{
    s8 bearerRet;
    wip_bearer_t br;

    /* Open the GPRS bearer */
    bearerRet = wip_bearerOpen ( &br, GPRS_BEARER, cbEvhBearer, NULL ) ;
    ASSERT_OK( bearerRet);

    /* Set the bearer configuration */
    bearerRet = wip_bearerSetOpts ( br,
                            WIP_BOPT_GPRS_APN, GPRS_APN,
                            WIP_BOPT_LOGIN, GPRS_USER,
                            WIP_BOPT_PASSWORD, GPRS_PASSWORD,
                            WIP_BOPT_END );
    ASSERT_OK( bearerRet );
 
    /* Start the bearer */
    bearerRet = wip_bearerStart ( br );
    ASSERT( OK == bearerRet || WIP_BERR_OK_INPROGRESS == bearerRet);
}

/*****************************************************************************/
/*  Function   : cbPollCreg()                                                */
/*---------------------------------------------------------------------------*/
/*  Object : A call to "AT+CREG?" has been done, to check the registration   */
/*           status, and the answer comes back to this handler.              */
/*           Either the registration is completed, and we can actually       */
/*           open and start the bearer, or it isn't, and we shall poll       */
/*           at "AT+CREG?" command again through a timer.                    */
/*                                                                           */
/*  Return : FALSE                                                           */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  Rsp               | x |   |   |  AT command response                     */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static bool cbPollCreg ( adl_atResponse_t *Rsp )
{
    ascii *rsp;
    ascii regStateString [ 3 ];
    s32 regStateInt;

    TRACE ( ( NORMAL_TRACE_LEVEL, "(cbPollCreg) Enter." ) ) ;

    rsp = ( ascii * ) adl_memGet ( Rsp->StrLength );
    wm_strRemoveCRLF ( rsp, Rsp->StrData, Rsp->StrLength );

    wm_strGetParameterString ( regStateString, Rsp->StrData, 2 );
    regStateInt = wm_atoi ( regStateString );

    if ( REG_STATE_REG == regStateInt || REG_STATE_ROAM == regStateInt )
    {
        TRACE( ( NORMAL_TRACE_LEVEL, "(cbPollCreg) Registered on GPRS network." ) ) ;
        /* Registration is complete so open and start bearer */
        OpenAndStartBearer ( ) ;
    }
    else
    {
        /* Not ready yet, we'll check again later. Set a one-off timer. */
        adl_tmrSubscribe ( FALSE, CREG_POLLING_PERIOD, ADL_TMR_TYPE_100MS, (adl_tmrHandler_t ) PollCreg );
    }
    return FALSE;
}

/*****************************************************************************/
/*  Function   : PollCreg                                                    */
/*---------------------------------------------------------------------------*/
/*  Object : Monitor the network registration; the only way to do that is    */
/*           through an AT command, so we send that "AT+CREG?" command.      */
/*           Actual reaction will be performed by the callback               */
/*           cbPollCreg().                                                   */
/*                                                                           */
/*  Return : None                                                            */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  Id                | X |   |   | Dummy parameter that makes the function  */
/*                    |   |   |   | callable by a timer's adl_tmrSubscribe() */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static void PollCreg ( u8 Id )
{
    adl_atCmdCreate ( "AT+CREG?", FALSE, cbPollCreg, ADL_STR_CREG, NULL );
}

/*****************************************************************************/
/*  Function   : cbEvhSim                                                    */
/*---------------------------------------------------------------------------*/
/*  Object     : sim events:                                                 */
/*               when SIM initialization is completed, check the registration*/
/*               status; PollCreg()'s callback will actually                 */
/*               open the bearer once registration is completed.             */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  event             | X |   |   |SIM event                                 */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
static void cbEvhSim ( u8 event )
{
    TRACE( ( NORMAL_TRACE_LEVEL, "(cbEvhSim) Enter." ) ) ;
    if ( ADL_SIM_EVENT_FULL_INIT == event )
    {
        /* argument 0 is dummy, see PollCreg() "Object" comment */
        PollCreg ( 0 );
    }
}

/*****************************************************************************/
/*  Function   : CfgGprs                                                     */
/*---------------------------------------------------------------------------*/
/*  Object     : initialize GPRS connection, then launch EntryPoint() on     */
/*               success.                                                    */
/*                                                                           */
/*  Return     : None                                                        */
/*---------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilization                             */
/*--------------------+---+---+---+------------------------------------------*/
/*  EntryPoint        | X |   |   |Function run after successful connection  */
/*--------------------+---+---+---+------------------------------------------*/
/*****************************************************************************/
void CfgGprs ( void ( * EntryPoint ) ( void ) )
{
    TRACE ( ( NORMAL_TRACE_LEVEL, "(CfgGprs) Enter." ) ) ;
    AppliEntryPoint = EntryPoint;
    adl_simSubscribe ( cbEvhSim, GPRS_PINCODE ) ;
}

