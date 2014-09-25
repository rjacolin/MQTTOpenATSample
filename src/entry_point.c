/*
 * entrypoint.c
 *
 *  Created on: Apr 11, 2014
 *      Author: rjacolin
 */

#include "adl_global.h"
#include "wip_net.h"
#include "mqtt/MQTTConnector.h"
#include "json/json_deserializer.h"
#include "json/json_serializer.h"

/* handlers for timer */
adl_tmr_t *timer_t_reading 	= NULL;
adl_tmr_t *timer_t_uploading = NULL;
#define READING_MESURES_PERIOD 		10  /* in seconds */
#define UPLOADING_MESURES_PERIOD 	30  /* in seconds */

char payload[1024];

/*Device Id*/
static char deviceId[20];

/* events from the environment */
int getTemperature();
double getHumidity();
double getLuminosity();
int getTimeStamp();
void temperatureHandler( adl_atCmdPreParser_t * paras );

/* variables reserve for parsing the messages comming from the server*/
static bool dataFailure = 0; /* Failure to parse a data message */
static bool lightState  = 0; /* Initially off */

int temp_ts[1024];
char* temp_values[1024];
int temp_size = 0;

int hum_ts[1024];
char* hum_values[1024];
int hum_size = 0;

/****************************************************************/
/*  Object     : Parse the message value coming from the server */
/****************************************************************/
void onIncomingData(char *path, char** keys, bsd_data_t **values, size_t nbofvalues){
	wip_debug("[callBackFct]     PATH = %s (%d) \r\n", path, nbofvalues);
	int i;
	for(i=0; i < nbofvalues; i++)
	{
		if( ! strcmp( keys[i], "Light") )
		{
			if( values[i]->content.boolean != lightState)
				wip_debug("[callBackFct]     Light is now %s \r\n", values[i]->content.boolean == true ? "ON" : "OFF");
		}

		else { /* Data path not handled */
	        dataFailure = 1;
	    }
	}
}

/*******************************************************************************************************/
/*  Object     : This function will be called at each time we get a message value from the server      */
/*               The user must check the returned context to know the type message. At the end of the  */
/*				 message, the user must acknowledge to the server		   							   */
/*******************************************************************************************************/
static void serverCallback( mqtt_callbackInfo_t *info)
{
	wip_debug("\r\n[callBackFct] Callback user is called\r\n");
	wip_debug("%s\r\n", info->content.data);
	mqtt_data_t data = json_deserialize(info->content.data);
	switch (info->kind)
	{
		case MQTT_OK   : wip_debug("[callBackFct]   SDB_SND_OK = %d  \n", info->content.status.val); break;
		case MQTT_KO   : wip_debug("[callBackFct]   SDB_SND_KO = %d  \n", info->content.status.val); break;
	}
//	if(data != NULL)
//	{
		onIncomingData( data.path, data.keys, data.values, data.nbofvalues);

		/*acknowledge to the server after receiving data*/
		//onDataAcknowledgment(data.ticketId);
//	}
	json_free(data);
	wip_debug("[callBackFct] End callback user  \r\n\n\n\n");
}

int readMeasures()
{
	int temperature   = getTemperature();
	double humidity   = getHumidity();
	double luminosity = getLuminosity() ;

	char str[250];
	wm_sprintf(str," temperature=%d, humidity=%lf, luminosity=%lf",  temperature, humidity, luminosity);
	wip_debug("[Entry_point] Reading measures : %s\n", str);

	/*payload =
		"["
	    "{"
	    "\"machine.temperature\": [{"
	    "  \"timestamp\" : %d, "
	    "  \"value\" : \"%lf\""
	    "}]"
	    "}"
	    "]";*/
	int timestamp = getTimeStamp();
	temp_ts[temp_size] = timestamp;
	temp_values[temp_size] = BS_MEM_ALLOC(10);
	wm_sprintf(temp_values[temp_size], "%d", temperature);
	temp_size++;

	hum_ts[hum_size] = timestamp;
	hum_values[hum_size] = BS_MEM_ALLOC(10);
	wm_sprintf(hum_values[hum_size], "%lf", humidity);
	hum_size++;

	return 0;
}

/*******************************************************************************/
/*  Object     : Push the general measures data of the house to the server	   */
/*******************************************************************************/
void uploadMeasures()
{
	char temp_payload[1024];
	// Push values using full data path.
	json_data(temp_payload, "machine.temperature", temp_ts, temp_values, temp_size);
	char hum_payload[1024];
	json_data(hum_payload, "machine.humidity", hum_ts, hum_values, hum_size);
	wm_sprintf(payload, "%s, %s", temp_payload, hum_payload);
	char* buffer = json_encapsulate(payload);
	wip_debug("[SAMPLES] %s\n", buffer);

	mqtt_publish(buffer, strlen(buffer));

	adl_memRelease(buffer);
	hum_size = 0;
	temp_size = 0;
}

/*******************************************************************************/
/*  Object     : Hanlder timer												   */
/*******************************************************************************/
static void readMeasuresHandler( u8 ID, void *ctx) {
	int r = readMeasures();
	if (r) wip_debug("[entry_point] Error Feed data to the table [status=%d]\n", r);
}

static void uploadMeasuresHandler( u8 ID, void *ctx) { uploadMeasures(); }

/********************************************************************************/
/*  Object     : Release the sending context and the data tables				*/
/* 				 Cancel all timer handlers									 	*/
/********************************************************************************/
void stopApplication()
{
	adl_tmrUnSubscribe ( timer_t_reading, readMeasuresHandler, ADL_TMR_TYPE_100MS );
	adl_tmrUnSubscribe ( timer_t_uploading, uploadMeasuresHandler, ADL_TMR_TYPE_100MS );
	//MQTT_disconnect();
}

/***************************************************************************/
/*  Function   : AppliEntryPoint                                           */
/*-------------------------------------------------------------------------*/
/*  Object     : Called once the WIP IP stack is fully initialized.        */
/*               This is the starting point of user applications.          */
/*-------------------------------------------------------------------------*/
/***************************************************************************/
void startApplication ( void )
{
	static char buffer[65535];
	MQTTConfig_t config = {
				.bufferSize 	= sizeof( buffer),
				.buffer			= buffer,
				.callback		= serverCallback,
				.callbackCtx	= NULL,
				.deviceId		= deviceId,  // imei of device
				.serverAddress 	= "eu.m2mop.net",
				.serverPort 	= 1883,
				.password 		= "myPassword"
		};
	wip_debug("[entry_point] Device id %s connecting\n", config.deviceId);
	mqtt_connect(&config);

	/* Read Measures all every READING_MESURES_PERIOD */
	timer_t_reading = adl_tmrSubscribe(true, READING_MESURES_PERIOD*10, ADL_TMR_TYPE_100MS, readMeasuresHandler);

	/* Upload Measures all every UPLOADING_MESURES_PERIOD to the server */
	timer_t_uploading = adl_tmrSubscribe(true, UPLOADING_MESURES_PERIOD*10, ADL_TMR_TYPE_100MS, uploadMeasuresHandler);
}

/************************************************************************************************************/
/*  Object     : The purpose of application is to subscribe one AT command and start the application	    */
/************************************************************************************************************/
void application ( u8 ID, void *ctx )
{
	wip_debug("[entry_point] Network up, starting application...\n\n");
	startApplication();

	/* Subscribe to a new AT command, named "at+temp" to simulate a temperature sensor, enter "at+temp=50" if user want to set the temperature = 50*/
	adl_atCmdSubscribe( "at+temperature", temperatureHandler, ADL_CMD_TYPE_PARA | ADL_CMD_TYPE_READ | 0x11);
}

/************************************************************************************************************/
/*  Object     :    set the numero of imei for deviceID     		    									*/
/************************************************************************************************************/
static bool cbImeiATHandler ( adl_atResponse_t *Rsp )
{
	if (Rsp->IsTerminal) {
	     return FALSE;
	}
	wm_strGetParameterString ( deviceId, Rsp->StrData, 1 );
	wip_debug("[entry_point] Device id %s\n", deviceId);
	return TRUE;
}

void whenNetworkUp ( void )
{
	adl_atCmdCreate ( "at+wimei?", TRUE, cbImeiATHandler, "*", NULL );

	adl_tmrSubscribe(false, 50, ADL_TMR_TYPE_100MS, application);
}
