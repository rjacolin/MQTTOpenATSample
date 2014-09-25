/**********************************************************************************************************************/
/*  AVMQTTConnector.c    -  Copyright Sierra Wireless Inc. (c) 2014                                                   */
/*                                                                                                                    */
/*                                                                                                                    */
/* LICENSE                                                                                                            */
/* =======                                                                                                            */
/* If no specific agreement has been signed, this Source Code is subject to the following license terms:              */
/* http://www.sierrawireless.com/productsandservices/AirPrime/Sierra_Wireless_Software_Suite/License/Userlicense.aspx */
/* If you modify this file, you are subject to the contribution rules described into this license                     */
/**********************************************************************************************************************/

/****************************************************************************
 * File    :   AVMQTTConnector.c
 *
 */

/*****************************************************************************/
/*  Include files                                                            */
/*****************************************************************************/

#include "adl_global.h"
#include "wip.h"
#include "mqtt/MQTTConnector.h"
#include "mqtt/MQTTPacket.h"
#include "mqtt/MQTTConnect.h"
#include "mqtt/MQTTPing.h"
#include "mqtt/mqttutils.h"

static wip_channel_t socket;
char* mqtt_buffer;
static MQTTConfig_t configuration;

enum state {
  MQTT_CONNECTED,
  MQTT_SESSION_OPENED,
  MQTT_SUBSCRIBED,
  MQTT_READY_TO_PUBLISH,
  MQTT_PUBLISHING,
  MQTT_PING,
  MQTT_DISCONNECTED
};

enum request {
	MQTT_RQT_CONNACK = 0x20,
	MQTT_RQT_PUBLISH = 0x30,
	MQTT_RQT_PUBACK = 0x40,
	MQTT_RQT_PUBREC = 0x50,
	MQTT_RQT_PUBREL = 0x60,
	MQTT_RQT_PUBCOMP = 0x70,
	MQTT_RQT_SUBACK = 0x90,
	MQTT_RQT_UNSUBACK = 0xB0,
	MQTT_RQT_PINGREQ = 0xC0,
	MQTT_RQT_PINGRESP = 0xD0
};
adl_tmr_t *timer_t_ping = NULL;
#define PING_PERIOD 		30  /* in seconds */
static enum state state = MQTT_DISCONNECTED;

/***************************************************************************/
/*  Defines                                                                */
/***************************************************************************/

#define RCV_BUFFER_SIZE         10240

#define NORMAL_TRACE_LEVEL      4
#define ERROR_TRACE_LEVEL       1

#define BS_MEM_ALLOC( size) adl_memGet( size)


/***************************************************************************/
/*  Globals                                                                */
/***************************************************************************/

static char rcv_buffer [ RCV_BUFFER_SIZE ];
static int rcv_offset = 0;
static int read_offset = 0;

/***************************************************************************/
/*  Function prototypes                                                    */
/***************************************************************************/
static void cbevh ( wip_event_t *ev, void *ctx );
int mqtt_pingresp(char* rcv_buffer, int nread);
int mqtt_publishreq(char* rcv_buffer, int nread);

static void pingHandler( u8 ID, void *ctx) {

	wip_debug("[MQTT] Ping server\n");
	int length = MQTTSerialize_pingreq(mqtt_buffer, 65535, 0, 0, 0);

	if(length > 0 && state != MQTT_PUBLISHING)
	{
		debug_packet(mqtt_buffer, length);
		int nread = wip_write(socket, mqtt_buffer, length);
		state = MQTT_PING;
		if(nread < 0)
		{
			wip_debug("[MQTT] Can't send PING (%d).\n", nread);
			socket_reload(socket);
			return;
		}
		else if(nread == 0)
			wip_debug("[MQTT] Waiting before sending");
		else
			wip_debug("[MQTT] %d/%dBytes has been sent!\n", nread, length);
	}
	else
		wip_debug("[MQTT] Can't ping.\n");

}

/***************************************************************************/
/*  Function   : AppliEntryPoint                                           */
/*-------------------------------------------------------------------------*/
/*  Object     : Called once the WIP IP stack is fully initialized.        */
/*               This is the starting point of user applications.          */
/*-------------------------------------------------------------------------*/
/***************************************************************************/
void mqtt_connect ( MQTTConfig_t* config )
{
	mqtt_buffer = malloc(65535);
	if(mqtt_buffer == 0)
		wip_debug("[MQTT] bad mqtt buffer\n");
    wip_debug ("[MQTT] connecting to client %s:%i\n", config->serverAddress, config->serverPort );
    size_t deviceId_len 		= strlen(config->deviceId);
    size_t serverAddress_len 	= strlen(config->serverAddress);
    size_t password_len 		= config->password==NULL ? 0 : strlen(config->password);
    memcpy(&configuration, config, sizeof(MQTTConfig_t) );
    configuration.deviceId = BS_MEM_ALLOC(deviceId_len + 1);
    configuration.password = BS_MEM_ALLOC(password_len +1 );
    configuration.serverAddress = BS_MEM_ALLOC(serverAddress_len+1);
    configuration.callbackCtx = config->callbackCtx;
    configuration.callback = config->callback;

	strcpy( (char*) configuration.deviceId, config->deviceId);
	strcpy( (char*) configuration.password, config->password);
	strcpy( (char*) configuration.serverAddress, config->serverAddress);
//    configuration.buffer = config->buffer;
//    configuration.bufferSize = config->bufferSize;
    configuration.serverPort = config->serverPort;

    socket = wip_TCPClientCreate ( config->serverAddress, config->serverPort, cbevh, NULL );
    if ( !socket )
    {
        wip_debug("[MQTT] Can't connect");
        return;
    }
}

void mqtt_subscribe(wip_channel_t socket)
{
	MQTTString channel = MQTTString_initializer;
	channel.lenstring.len = 0;
	channel.cstring = BS_MEM_ALLOC((15 + 12));
	wm_sprintf(channel.cstring, "%s/tasks/json", configuration.deviceId);

	int qos[1];
	qos[0]=0;
	wip_debug("[MQTT] Subscribing to %s\n", channel.cstring);
	char buffer[1024];
	int length = MQTTSerialize_subscribe(buffer, 1024, 1, 1, 1, &channel, qos);
	if(length > 0)
	{
		int nread = wip_write(socket, buffer, length);
		if(nread < 0)
		{
			wip_debug("[MQTT] Can't send SUBSCRIBE (%d).\n", nread);
			return;
		}
		state = MQTT_SUBSCRIBED;
	}
	else
		wip_debug("[MQTT] Can't subscribe to %s\n", channel.cstring);
}

void mqtt_publish(char* payload, int payloadSize)
{
	MQTTString channel = MQTTString_initializer;
	channel.lenstring.len = 0;
	channel.cstring = BS_MEM_ALLOC((15 + 15));
	wm_sprintf(channel.cstring, "%s/messages/json", configuration.deviceId);

	if(state == MQTT_READY_TO_PUBLISH)
	{
		state = MQTT_PUBLISHING;
		int length = MQTTSerialize_publish(mqtt_buffer, 65535, 0, 0, 0, 0, channel, payload, payloadSize);
		if(length > 0)
		{
			debug_packet(mqtt_buffer, length);
			int nwritten = wip_write(socket, mqtt_buffer, length);
			if(nwritten < 0)
			{
				wip_debug("[MQTT] Can't send PUBLISH (%d).\n", nwritten);
				return;
			}
			else if(nwritten == 0)
				wip_debug("[MQTT] Waiting before sending");
			else
				wip_debug("[MQTT] %d/%dBytes has been sent!\n", nwritten, length);
		}
		else
			wip_debug("[MQTT] Can't publish.\n");
		state = MQTT_READY_TO_PUBLISH;
	}
}

void mqtt_disconnect()
{
	adl_tmrUnSubscribe ( timer_t_ping, pingHandler, ADL_TMR_TYPE_100MS );
	int ret = MQTTSerialize_disconnect(mqtt_buffer, 65535);
	if(ret <= 0)
		wip_debug("[MQTT] Error disconnect: %d\n", ret);
	else
	{
		int nread = 0;

		debug_packet(mqtt_buffer, ret);
		nread = wip_write(socket, mqtt_buffer, 65535);
		if(nread <= 0)
		{
			wip_debug("[MQTT] Can't send DISCONNECT (%d).\n", nread);
			return;
		}
		state = MQTT_DISCONNECTED;
	}

}

void mqtt_openSession(wip_channel_t channel)
{
	MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
	options.MQTTVersion = 3;
	options.clientID.lenstring.len = 0;
	options.clientID.cstring = configuration.deviceId;
	options.password.lenstring.len = 0;
	options.password.cstring = configuration.password;
	options.username.lenstring.len = 0;
	options.username.cstring = configuration.deviceId;
	options.keepAliveInterval = 30;
	options.willFlag = 0;

	int ret = MQTTSerialize_connect(mqtt_buffer, 65535, &options);
	if(ret <= 0)
		wip_debug("[MQTT] Error connect: %d\n", ret);
	else
	{
		int nread = 0;

		debug_packet(mqtt_buffer, ret);
		nread = wip_write(channel, mqtt_buffer, ret);
		if(nread <= 0)
		{
			wip_debug("[MQTT] Can't send CONNECT (%d).\n", nread);
			return;
		}
		else
			wip_debug("[MQTT] %dBytes have been sent for Connection.\n", nread);
		state = MQTT_SESSION_OPENED;
	}
}

/*void write(enum state state,  wip_channel_t channel)
{
	switch(state)
	{
		case MQTT_CONNECTED:
			mqtt_openSession(channel);
			break;
		default:
		{
			wip_debug("default\n");
			break;
		}
	}
}*/

/**
 * @return 1 if a command has been read, 0 if nothing has been done, waiting new byte to continue.
 */
int mqtt_unstack_buffer(char* rcv_buffer, wip_channel_t channel, int nread)
{
	wip_debug("[MQTT] state: %d, read_offset:%d, rcv_offset: %d\r\n", state, read_offset, rcv_offset);

	int length = mqtt_decodeLength(rcv_buffer + read_offset);
	wip_debug("Length: %d\r\n", length);
	if( length > (rcv_offset - read_offset))
		return 0; // Waiting new bytes

	debug_packet(rcv_buffer + read_offset, length);

	char type = mqtt_decodeType(rcv_buffer + read_offset);
	// Check Ping
	if((type & MQTT_RQT_PINGRESP) == MQTT_RQT_PINGRESP)
	{
		mqtt_pingresp(rcv_buffer + read_offset, (rcv_offset - read_offset));
		read_offset += length;
		return 1;
	}

	// Check Publish
	if((type & MQTT_RQT_PUBLISH) == MQTT_RQT_PUBLISH)
	{
		mqtt_publishreq(rcv_buffer + read_offset, (rcv_offset - read_offset));
		read_offset += length + 1;
		return 1;
	}

	// Check connack
	if((type & MQTT_RQT_CONNACK) == MQTT_RQT_CONNACK)
	{
		int connack;

		int ret = MQTTDeserialize_connack(&connack, rcv_buffer + read_offset, rcv_offset);
		if(ret == 1)
		{
			mqtt_subscribe(channel);
			timer_t_ping = adl_tmrSubscribe(true, PING_PERIOD*10, ADL_TMR_TYPE_100MS, pingHandler);
			wip_debug("[MQTT] Waiting publish (%d)\r\n", connack);
			state = MQTT_READY_TO_PUBLISH;
			read_offset += length;
			return 1;
		}
	}
	// Check Subscription ack
	if((type & MQTT_RQT_SUBACK) == MQTT_RQT_SUBACK)
	{
		int packetid;
		int count;
		int granted;
		int ret = MQTTDeserialize_suback(&packetid, 5, &count, &granted, rcv_buffer + read_offset, rcv_offset);
		if(ret == 1)
		{
			wip_debug("[MQTT] id=%d, count=%d, granted=%d\r\n", packetid, count, granted);
			read_offset += length;
			return 1;
		}
	}

	// Don't care about this request
	read_offset += length;
	wip_debug("[MQTT] Request skipped (type: %X)...\r\n", type);

	// Nothing more.
	return 0;
}

int mqtt_publishreq(char* rcv_buffer, int nread)
{
	wip_debug("[MQTT] Notification (%d)!\n", nread);

	MQTTString topicName = MQTTString_initializer;
	int dup;
	int qos;
	int retained;
	int packetid;
	int payloadlen;
	char payload[1024];
	char* pay = payload;
	int ret = MQTTDeserialize_publish(&dup, &qos, &retained, &packetid, &topicName,
			&pay, &payloadlen, rcv_buffer, nread);

	if(ret == 1)
	{
		wip_debug("[MQTT] Published (%d,%d)\n", dup, qos);

		mqtt_callbackInfo_t info;
		info.kind = MQTT_OK;
		info.userdata = configuration.callbackCtx;
		info.content.status.val = 200;
		info.content.data = pay;

		// Call user callback
		(*configuration.callback)(&info);
	}
	else
		wip_debug("[MQTT] Not a Publish.\n");
	return ret;
}

int mqtt_pingresp(char* rcv_buffer, int nread)
{
	int dup;
	int qos;
	int retained;
	int ret = MQTTDeserialize_pingresp(&dup, &qos, &retained, rcv_buffer, nread);
	if(ret == 1)
	{
		wip_debug("[MQTT] Ping RESP with %d, %d, %d\n", dup, qos, retained);
	}
	else
		wip_debug("[MQTT] Ping not acked\n");

	state = MQTT_READY_TO_PUBLISH;
	return ret;
}

/***************************************************************************/
/*  Function   : cbevh                                                     */
/*-------------------------------------------------------------------------*/
/*  Object     : Handling events happenning on the TCP client socket.      */
/*-------------------------------------------------------------------------*/
/*  Variable Name     |IN |OUT|GLB|  Utilisation                           */
/*--------------------+---+---+---+----------------------------------------*/
/*  ev                | X |   |   |  WIP event                             */
/*--------------------+---+---+---+----------------------------------------*/
/*  ctx               | X |   |   |  user data (unused)                    */
/*--------------------+---+---+---+----------------------------------------*/
/***************************************************************************/
static void cbevh ( wip_event_t *ev, void *ctx )
{
    switch ( ev->kind )
    {
        case WIP_CEV_OPEN:
        {
            wip_debug("[MQTT] Connection established successfully\n");
            state = MQTT_CONNECTED;
            break;
        }

        case WIP_CEV_READ:
        {
            int nread = 0;
            int ntotalread = 0;
            wip_debug("[TCP] Some data arrived:\n");
            do
            {
            	nread = wip_read ( ev->channel, rcv_buffer + rcv_offset, sizeof ( rcv_buffer ) - rcv_offset );
            	if ( nread < 0 )
				{
					wip_debug ( "[MQTT] read error %i\n", nread );
					return;
				}
            	wip_debug("[TCP] %i bytes\n", nread);
            	ntotalread +=nread;
            	rcv_offset += nread;
            }
            while(nread != 0);

            if ( rcv_offset == sizeof ( rcv_buffer ) )
            {
            	wip_debug ( "[MQTT] Reception capacity exceeded, won't read more\n" );
            }
            else
            {
            	wip_debug ( "[MQTT] %i bytes received.\n", ntotalread);
            	wip_debug ( "[MQTT] Wrote %i bytes of data from network to rcv_buffer. "
                                "%i bytes remain available in rcv_buffer\n", ntotalread, sizeof ( rcv_buffer ) - rcv_offset );
            }

            int request = 0;
            do
            {
            	request = mqtt_unstack_buffer(rcv_buffer, ev->channel, (rcv_offset - read_offset));
            }
            while(request != 0 && rcv_offset != read_offset);

            if( rcv_offset == read_offset)
            {
            	rcv_offset = 0;
            	read_offset = 0;
            }
            break;
        }

        case WIP_CEV_WRITE:
        {
        	if(state == MQTT_CONNECTED)
        		mqtt_openSession(ev->channel);
            break;
        }

        case WIP_CEV_ERROR:
        {
        	wip_debug ( "[MQTT] Error %i on socket. Closing.\n", ev->content.error.errnum ) ;
            socket_reload(ev->channel);
            break;
        }

        case WIP_CEV_PEER_CLOSE:
        {
            wip_debug("[MQTT] Connection closed by peer.\n" );
            socket_reload(ev->channel);
            break;
        }

        default:
        {
            break;
        }
    }
}

void socket_reload(wip_channel_t channel)
{
	wip_close (channel );
	state = MQTT_DISCONNECTED;
	socket = wip_TCPClientCreate ( configuration.serverAddress, configuration.serverPort, cbevh, NULL );
	if ( !socket )
	{
		wip_debug("[MQTT] Can't connect");
		return;
	}
}
