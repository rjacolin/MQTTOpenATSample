/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include "mqtt/MQTTPacket.h"
#include "mqtt/StackTrace.h"

#include <string.h>

/**
  * Serializes the supplied publish data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param qos integer - the MQTT QoS value
  * @param retained integer - the MQTT retained flag
  * @return the length of the serialized data.  <= 0 indicates error
  */
int MQTTSerialize_pingreq(char* buf, int buflen, int dup, int qos, int retained)
{
	char *ptr = buf;
	MQTTHeader header;
	int rc = 0;

	FUNC_ENTRY;

	if (buflen < 2)
	{
		rc = MQTTPACKET_BUFFER_TOO_SHORT;
		goto exit;
	}
	header.bits.type = PINGREQ;
	header.bits.dup = dup;
	header.bits.qos = qos;
	header.bits.retain = retained;
	writeChar(&ptr, header.byte); /* write header */

	ptr += MQTTPacket_encode(ptr, 0); /* write remaining length */;

	rc = ptr - buf;

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}

/**
  * Serializes the ack packet into the supplied buffer.
  * @param type integer - the MQTT packet type
  * @param dup integer - the MQTT dup flag
  * @param retained integer - the MQTT retained flag
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
int MQTTDeserialize_pingresp(int* dup, int* qos, int* retained, char* buf, int len)
{
	MQTTHeader header;
	char* curdata = buf;
	int rc = 0;
	int mylen;

	FUNC_ENTRY;
	header.byte = readChar(&curdata);

	curdata += (rc = MQTTPacket_decodeBuf(curdata, &mylen)); /* read remaining length */
	if (mylen != 0)
		goto exit;

	*dup = header.bits.dup;
	*qos = header.bits.qos;
	*retained = header.bits.retain;
	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}

