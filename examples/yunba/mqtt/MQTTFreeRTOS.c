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

#include "MQTTFreeRTOS.h"
#include "stdlib.h"
//#include "esp_libc.h"
//#include "errno.h"
//#include "math.h"

#ifndef RAND_MAX
#define RAND_MAX 0x7FFF
#endif


int ICACHE_FLASH_ATTR ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
	int rc = 0;
	uint16_t usTaskStackSize = 280;//(configMINIMAL_STACK_SIZE * 5);
	portBASE_TYPE uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

	rc = xTaskCreate(fn,	/* The function that implements the task. */
		"MQTTTask",			/* Just a text name for the task to aid debugging. */
		usTaskStackSize,	/* The stack size is defined in FreeRTOSIPConfig.h. */
		arg,				/* The task parameter, not used in this case. */
		uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
		&thread->task);		/* The task handle is not used. */

	return rc;
}


void MutexInit(Mutex* mutex)
{
	mutex->sem = xSemaphoreCreateMutex();
}

int MutexLock(Mutex* mutex)
{
	int rc;
//	os_printf("mutex lock\n");
//	rc = xSemaphoreTake(mutex->sem, 3000 / portTICK_RATE_MS/*portMAX_DELAY*/);
	rc = xSemaphoreTake(mutex->sem, portMAX_DELAY);
//	os_printf("mutex lock:%d\n", rc);
	return rc;
}

int MutexUnlock(Mutex* mutex)
{
	int rc;
	rc = xSemaphoreGive(mutex->sem);
	//os_printf("mutex unlock\n");
	return rc;
}


void IRAM_ATTR
TimerCountdownMS(Timer* timer, uint32_t timeout_ms)
{
	timer->xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
	vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}


void TimerCountdown(Timer* timer, uint32_t timeout)
{
	TimerCountdownMS(timer, timeout * 1000);
}


uint32_t TimerLeftMS(Timer* timer)
{
	xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
	return (timer->xTicksToWait < 0) ? 0 : (timer->xTicksToWait * portTICK_RATE_MS);
}


portBASE_TYPE TimerIsExpired(Timer* timer)
{
	return xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait) == pdTRUE;
}


void TimerInit(Timer* timer)
{
	timer->xTicksToWait = 0;
	memset(&timer->xTimeOut, '\0', sizeof(timer->xTimeOut));
}


int FreeRTOS_read(Network* n, unsigned char* buffer, int len, uint32_t timeout_ms)
{
	portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
	xTimeOutType xTimeOut;
	int recvLen = 0;

	vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
	FreeRTOS_setsockopt(n->my_socket, SOL_SOCKET, FREERTOS_SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
	do
	{
		int rc = 0;

		rc = FreeRTOS_recv(n->my_socket, buffer + recvLen, len - recvLen, 0);
		if (rc > 0)
			recvLen += rc;
		else if (rc < 0)
		{
			recvLen = rc;
			break;
		}
	} while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

	return recvLen;
}


int FreeRTOS_write(Network* n, unsigned char* buffer, int len, uint32_t timeout_ms)
{
	portTickType xTicksToWait = timeout_ms / portTICK_RATE_MS; /* convert milliseconds to ticks */
	xTimeOutType xTimeOut;
	int sentLen = 0;

	vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */

	FreeRTOS_setsockopt(n->my_socket, SOL_SOCKET, FREERTOS_SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));

	do
	{
		int rc = 0;
		rc = FreeRTOS_send(n->my_socket, buffer + sentLen, len - sentLen, 0);
		if (rc > 0)
			sentLen += rc;
		else if (rc < 0)
		{
			sentLen = rc;
			break;
		}
	} while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

	return sentLen;
}


void FreeRTOS_disconnect(Network* n)
{
	FreeRTOS_closesocket(n->my_socket);
}


void ICACHE_FLASH_ATTR NetworkInit(Network* n)
{
	n->my_socket = 0;
	n->mqttread = FreeRTOS_read;
	n->mqttwrite = FreeRTOS_write;
	n->disconnect = FreeRTOS_disconnect;
}

int ICACHE_FLASH_ATTR getIpForHost(const char *host, struct sockaddr_in *ip) {
	struct hostent *he;
	struct in_addr **addr_list;
	he=gethostbyname(host);
	if (he==NULL) return 0;
	addr_list=(struct in_addr **)he->h_addr_list;
	if (addr_list[0]==NULL) return 0;
	ip->sin_family=AF_INET;
	memcpy(&ip->sin_addr, addr_list[0], sizeof(ip->sin_addr));
	return 1;
}


int ICACHE_FLASH_ATTR NetworkConnect(Network* n, char* addr, int port)
{
	struct sockaddr_in sAddr;
	int retVal = -1;

	bzero(&sAddr, sizeof(struct sockaddr_in));
	if (!getIpForHost(addr, &sAddr)) {
		printf("get ip by hostname fail\n");
		return -1;
	}

	sAddr.sin_port = FreeRTOS_htons(port);

	if ((n->my_socket = FreeRTOS_socket(PF_INET, FREERTOS_SOCK_STREAM, 0)) < 0)
		goto exit;

//	printf("Connecting to server %s...\n", ipaddr_ntoa((const ip_addr_t*)&sAddr.sin_addr.s_addr));

	if ((retVal = FreeRTOS_connect(n->my_socket, (struct sockaddr *)(&sAddr), sizeof(struct sockaddr))) < 0)
	{
		FreeRTOS_closesocket(n->my_socket);
	    goto exit;
	}

exit:
//	printf("------------>connect init: %d, sock:%d \n", retVal, n->my_socket);
	return retVal;

#if 0
	struct freertos_sockaddr sAddr;
	int retVal = -1;
	uint32_t ipAddress;
	struct sockaddr_in *name_in;

	struct ip_addr *ad;
	struct hostent *pHost;
	if ((pHost = FreeRTOS_gethostbyname(addr)) == 0)
		goto exit;

	ad = (struct ip_addr *)&pHost->h_addr_list[0];
	ipAddress = ad->addr;

	sAddr.sa_family = AF_INET;
	name_in = (struct sockaddr_in *)(void*)(&sAddr);
	name_in->sin_port = FreeRTOS_htons(port);
	name_in->sin_addr.s_addr = ipAddress;
	printf("----->get ip: %08x, %08x, %08x\n", ipAddress, port, FreeRTOS_htons(port));

	if ((n->my_socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP)) < 0)
		goto exit;

	printf("++++++++++>get ip: %s\n", addr);

	if ((retVal = FreeRTOS_connect(n->my_socket, &sAddr, sizeof(sAddr))) < 0)
	{
		FreeRTOS_closesocket(n->my_socket);
	    goto exit;
	}

exit:
	printf("------------>connect init fail \n");
	return retVal;
#endif

#if 0
    int type = SOCK_STREAM;
    struct sockaddr_in address;
    int retVal = -1;
    sa_family_t family = AF_INET;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

    if ((retVal = getaddrinfo(addr, NULL, &hints, &result)) == 0)
    {
    	printf("------------------->1\n");
            struct addrinfo* res = result;
            /* prefer ip4 addresses */
            while (res)
            {
                    if (res->ai_family == AF_INET)
                    {
                            result = res;
                            break;
                    }
                    res = res->ai_next;
            }
            if (result->ai_family == AF_INET)
            {
				address.sin_port = htons(port);
				address.sin_family = family = AF_INET;
				address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
				printf("------------------->2, %08x, %08x\n", address.sin_port, address.sin_addr);
            }
            else
            	retVal = -1;

            freeaddrinfo(result);
    }

    if (retVal == 0) {
		printf("------------------->3\n");
		n->my_socket = socket(family, type, 0);
		if (n->my_socket != -1) {
			printf("Connecting to server %s...\n", ipaddr_ntoa((const ip_addr_t*)&address.sin_addr.s_addr));
			printf("------------------->4, socket:%d\n", n->my_socket);
			while (1) {
				retVal = FreeRTOS_connect(n->my_socket, (struct sockaddr*)&address, sizeof(struct sockaddr));
				if (retVal == 0) break;
			}
			printf("------------------->5, retrun:%d\n", retVal);
		}
}

exit:
    return retVal;
#endif
}



//unsigned long long int randm(int n) {
//#include <espressif/esp_libc.h>
//        double x;
//        unsigned long long int y;
//        srand(xTaskGetTickCount());
//        x = rand() / (double)RAND_MAX;
//        y = (unsigned long long int) (x * (unsigned long long int)pow_l((float)10.0, (float)n*1.0));
//        return y;
//}

uint64_t generate_uuid() {
        uint64_t system_time = (uint64_t)system_get_time();
//        uint64_t id = utc << (64 - 41);
//        id |= (uint64_t)(randm(16) % (unsigned long long int)(pow(2, (64 - 41))));
        return system_time;
}



#if 0
int NetworkConnectTLS(Network *n, char* addr, int port, SlSockSecureFiles_t* certificates, unsigned char sec_method, unsigned int cipher, char server_verify)
{
	SlSockAddrIn_t sAddr;
	int addrSize;
	int retVal;
	unsigned long ipAddress;

	retVal = sl_NetAppDnsGetHostByName(addr, strlen(addr), &ipAddress, AF_INET);
	if (retVal < 0) {
		return -1;
	}

	sAddr.sin_family = AF_INET;
	sAddr.sin_port = sl_Htons((unsigned short)port);
	sAddr.sin_addr.s_addr = sl_Htonl(ipAddress);

	addrSize = sizeof(SlSockAddrIn_t);

	n->my_socket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);
	if (n->my_socket < 0) {
		return -1;
	}

	SlSockSecureMethod method;
	method.secureMethod = sec_method;
	retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECMETHOD, &method, sizeof(method));
	if (retVal < 0) {
		return retVal;
	}

	SlSockSecureMask mask;
	mask.secureMask = cipher;
	retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &mask, sizeof(mask));
	if (retVal < 0) {
		return retVal;
	}

	if (certificates != NULL) {
		retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_FILES, certificates->secureFiles, sizeof(SlSockSecureFiles_t));
		if (retVal < 0)
		{
			return retVal;
		}
	}

	retVal = sl_Connect(n->my_socket, (SlSockAddr_t *)&sAddr, addrSize);
	if (retVal < 0) {
		if (server_verify || retVal != -453) {
			sl_Close(n->my_socket);
			return retVal;
		}
	}

	SysTickIntRegister(SysTickIntHandler);
	SysTickPeriodSet(80000);
	SysTickEnable();

	return retVal;
}
#endif
