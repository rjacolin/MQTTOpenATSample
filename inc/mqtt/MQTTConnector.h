/*
 * MQTTConnector.h
 *
 *  Created on: Apr 11, 2014
 *      Author: rjacolin
 */

#ifndef MQTTCONNECTOR_H_
#define MQTTCONNECTOR_H_

/**
 * sdb_sendCallbackInfo_t structure received by user.
 * @param kind :
 * 		-  SDB_SND_OK     : data have been successfully sent to the server (content.status = 200)
 * 		-  SDB_SND_KO     : fail to send the data to the server, the user may check the content.status.val value to know more the explanation
 * 		-  SDB_SND_DATA   : receive a new data from the server, the new data is stored in the content.data structure.
 * 		The user must acknowledge to the server with the content.data.ticketId value. See @content.data
 *
 * @param content.status.val     ( reserve for SDB_SND_OK and SDB_SND_KO event )
 * 		-  200 : see SDB_SND_OK
 * 		-  400 : Cannot parse the message from the server, the device cannot bind the message in the m3da protocol
 * 		-  407 : There is no authentication field in the message.
 * 		-  500 : Reject the sending request of the device by the server, the socket is closed by the server, the error may be due to the server timeout OR the incompatible declaration (security, protocol, password ...) between the peers
 * 		- -1000: Tried to read/write an aborted TCP client
 * 		- -999 : The channel is not in WIP_CSTATE_READY state
 * 		- -998 : The option is not supported by channel
 * 		- -997 : The option value is out of range
 * 		- -996 : adl_memGet() memory allocation failure
 * 		- -995 : WIP internal error
 * 		- -994 : Invalid option or parameter value
 * 		- -993 : Couldn't resolve a name to an IP address
 * 		- -992 : No more TCP buffers available
 * 		- -991 : TCP server port already used
 * 		- -990 : TCP connection refused by server
 * 		- -989 : No route to host
 * 		- -988 : No network reachable at all
 * 		- -987 : TCP connection broken
 * 		- -986 : Timeout (for DNS request, TCP connection, PING response...)
 *
 * @param content.status.n_tables: number of table is given by sdb_sendTable() or sdb_sendTables()
 *
 * @param content.status.tables: list of tables is given by sdb_sendTable() or sdb_sendTables().
 *
 * @param content.data
 */
typedef struct sdb_sendCallbackInfo_t {
	enum mqtt_callbackKind_t {
		MQTT_OK,
		MQTT_KO
	} kind;
	union {
		struct mqtt_status_t{	     ///<  for OK and KO
			int val;
		} status;
		char* data;
	} content ;
	void *userdata;
} mqtt_callbackInfo_t;

/**
 * Callback prototype for reception
 * */
typedef void mqtt_callback_t( struct mqtt_callbackInfo_t *callback_info);

typedef struct MQTTConfig {
	const char *deviceId;			 ///<  '\0'-terminated unique device id, ex : "BH3280010308101"
	const char *serverAddress;
	unsigned short serverPort;
	char* buffer;
	size_t bufferSize;
	mqtt_callback_t *callback;       ///<  callback user function to get the responses from the server
	void *callbackCtx;				 ///<  context user data */
	const char * password;           ///<  '\0'-terminated password for authentication & encryption

} MQTTConfig_t;

void mqtt_connect(MQTTConfig_t* config);
void mqtt_subscribe();
void mqtt_publish(char* payload, int payloadSize);
void mqtt_disconnect();

#endif /* MQTTCONNECTOR_H_ */
