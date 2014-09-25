/*
 * mqttutils.h
 *
 *  Created on: Jun 5, 2014
 *      Author: rjacolin
 */

#include "adl_global.h"

#ifndef MQTTUTILS_H_
#define MQTTUTILS_H_

char mqtt_decodeType(char* buf);
int mqtt_decodeLength(char* buf);
void debug_packet(char* buffer, size_t length);

#endif /* MQTTUTILS_H_ */
