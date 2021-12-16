/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
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
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *    Ian Craggs - convert to FreeRTOS
 *******************************************************************************/

#include <string.h>
#include "MQTT_lpc1549.h"

#include "systick.h"
#include "esp8266_socket.h"




void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
	timer->TicksToWait = timeout_ms;
	timer->Start = get_ticks();
}


void TimerCountdown(Timer* timer, unsigned int timeout) 
{
	TimerCountdownMS(timer, timeout * 1000);
}


int TimerLeftMS(Timer* timer) 
{
	int32_t left = (int32_t)timer->TicksToWait - (int32_t)(get_ticks() - timer->Start);
    return left < 0 ? 0 : left;
}


char TimerIsExpired(Timer* timer)
{
	return (get_ticks() - timer->Start) > timer->TicksToWait;
}


void TimerInit(Timer* timer)
{
	timer->TicksToWait = 0;
	timer->Start = 0;
}


int lpc1549_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	int recvLen = 0;
	Timer timer;

	TimerCountdownMS(&timer, timeout_ms);

	do
	{
		int rc = 0;

		rc = esp_read(n->my_socket, buffer + recvLen, len - recvLen);
		if (rc > 0)
			recvLen += rc;
		else if (rc < 0)
		{
			recvLen = rc;
			break;
		}
	} while (recvLen < len && !TimerIsExpired(&timer));

    return recvLen;
}


int lpc1549_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	int sentLen = 0;
	Timer timer;

	TimerCountdownMS(&timer, timeout_ms);


    do
	{
		int rc = 0;

		rc = esp_write(n->my_socket, buffer + sentLen, len - sentLen);
		if (rc > 0)
			sentLen += rc;
		else if (rc < 0)
		{
			sentLen = rc;
			break;
		}
	} while (sentLen < len && !TimerIsExpired(&timer));

return sentLen;
}


void lpc1549_disconnect(Network* n)
{
    esp_close(n->my_socket);
}


void NetworkInit(Network *n, const char *ssid, const char *password)
{
	n->mqttread = lpc1549_read;
	n->mqttwrite = lpc1549_write;
	n->disconnect = lpc1549_disconnect;

    strncpy(n->ssid, ssid, 32);
    strncpy(n->password, password, 32);

    n->my_socket = esp_socket(n->ssid, n->password);


}


int NetworkConnect(Network* n, char* address, int port)
{
    return esp_connect(n->my_socket, address, port);
}

void NetworkDisconnect(Network *n)
{
	n->disconnect(n);
}

