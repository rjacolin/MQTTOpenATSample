
#include "mqtt/mqttutils.h"

#define DEBUG true

char mqtt_decodeType(char* buf)
{
	char* curdata = buf;
	return readChar(&curdata);
}

int mqtt_decodeLength(char* buf)
{
	char* curdata = buf;
	int mylen;

	readChar(&curdata);

	MQTTPacket_decodeBuf(curdata, &mylen); /* read remaining length */

	return (mylen + 2);
}

void debug_packet(char* buffer, size_t length)
{
	if(DEBUG == true)
	{
		int i = 0;
		for(i=0; i < length;i++)
		{
			wip_debug(" %0X", buffer[i]);
			if(i % 10 == 0 && i > 0)
				wip_debug("\n");
		}
		wip_debug("\n");
	}
}
