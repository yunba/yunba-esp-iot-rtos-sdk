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

#if !defined(MQTTFreeRTOS_H)
#define MQTTFreeRTOS_H

//#include "FreeRTOS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#include "FreeRTOS_Sockets.h"
//#include "FreeRTOS_IP.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define FreeRTOS_setsockopt(a, b, c, d, e)   setsockopt(a, b, c, d, e)
#define FreeRTOS_recv(a, b, c, d) recv(a, b, c, d)
#define FreeRTOS_send(a, b, c, d) send(a, b, c, d)
#define FreeRTOS_closesocket(a) closesocket(a)
#define FreeRTOS_gethostbyname(a) lwip_gethostbyname(a)
#define FreeRTOS_htons(a) htons(a)
#define FreeRTOS_socket(a, b, c) socket(a, b, c)
#define FreeRTOS_connect(a, b, c) connect(a, b, c)

#define freertos_sockaddr sockaddr


#define FREERTOS_SO_RCVTIMEO SO_RCVTIMEO

#define FREERTOS_AF_INET AF_INET
#define FREERTOS_SOCK_STREAM SOCK_STREAM
#define FREERTOS_IPPROTO_TCP IPPROTO_TCP


typedef struct Timer 
{
//	TickType_t xTicksToWait;
	portTickType xTicksToWait;
//	TimeOut_t xTimeOut;
	xTimeOutType xTimeOut;
} Timer;

typedef struct Network Network;

struct Network
{
//	xSocket_t my_socket;
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, uint32_t);
	int (*mqttwrite) (Network*, unsigned char*, int, uint32_t);
	void (*disconnect) (Network*);
};

void TimerInit(Timer*);
portBASE_TYPE TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, uint32_t);
void TimerCountdown(Timer*, uint32_t);
uint32_t TimerLeftMS(Timer*);

typedef struct Mutex
{
//	SemaphoreHandle_t sem;
	xSemaphoreHandle sem;
} Mutex;

void MutexInit(Mutex*);
int MutexLock(Mutex*);
int MutexUnlock(Mutex*);

typedef struct Thread
{
	xTaskHandle task;
} Thread;

int ThreadStart(Thread*, void (*fn)(void*), void* arg);

int FreeRTOS_read(Network*, unsigned char*, int, uint32_t);
int FreeRTOS_write(Network*, unsigned char*, int, uint32_t);
void FreeRTOS_disconnect(Network*);

void NetworkInit(Network*);
int NetworkConnect(Network*, char*, int);
/*int NetworkConnectTLS(Network*, char*, int, SlSockSecureFiles_t*, unsigned char, unsigned int, char);*/
uint64_t generate_uuid();

#endif
