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

#ifndef MQTTPING_H_
#define MQTTPING_H_

int MQTTSerialize_pingreq(char* buf, int buflen, int dup, int qos, int retained);

int MQTTDeserialize_pingresp(int* dup, int* qos, int* retained, char* buf, int len);

#endif /* MQTTPING_H_ */
