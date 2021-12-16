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
 *******************************************************************************/

#if !defined(MQTT_lpc1549_H)
#define MQTT_lpc1549_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct Timer 
{
	uint32_t TicksToWait;
	uint32_t Start;
} Timer;


typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
    char ssid[32];
    char password[32];
};

void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);



int lpc1549_read(Network*, unsigned char*, int, int);
int lpc1549_write(Network*, unsigned char*, int, int);
void lpc1549_disconnect(Network*);

void NetworkInit(Network *n, const char *ssid, const char *password);
int NetworkConnect(Network *n, char *address, int port);
void NetworkDisconnect(Network *n);

#ifdef __cplusplus
}
#endif


#endif
