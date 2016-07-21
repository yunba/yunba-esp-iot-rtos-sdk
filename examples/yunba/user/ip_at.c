#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MQTTClient.h"
#include "util.h"
#include "at.h"
#include "user_config.h"
#include "driver/uart.h"
#include "ip_at.h"

#define RECV_TIMEOUT 1200


typedef struct {
	int socket;
	bool connect;
} IP_MGR_t;


IP_MGR_t ip_mgr = {-1, false};


LOCAL void printResp(int8_t *resp) {
	uart0_sendStr(resp);
}

LOCAL bool close_conn(IP_MGR_t *mgr) {
	APP_TRACE_INFO(("close socket: %d\n", mgr->socket));
	if (mgr->socket > 0)
		close(mgr->socket);
	mgr->connect = false;
	mgr->socket = -1;
	return true;
}

LOCAL int8_t close_conn_resp(bool rc) {
	char buf[12];
	snprintf(buf, 12, "%s\r\n", rc ? "OK": "ERROR");
	printResp(buf);
	return 0;
}

LOCAL bool send_payload(IP_MGR_t *mgr, uint8_t *payload, uint16_t len) {
	int8_t sent = 0;
	bool ret = false;

	sent = send(mgr->socket, payload, len, 0);
	APP_TRACE_INFO(("ip send: %d, %d\n", len, sent));
	if (len == sent)
		ret = true;
	return ret;
}

LOCAL int send_payload_resp(bool rc) {
	char buf[56];
	sprintf(buf, "%s\r\n", rc ? "SEND OK" : "ERROR");
	printResp(buf);
	return 0;
}

LOCAL bool get_ip_from_hostname(const char *host, struct sockaddr_in *ip) {
	struct hostent *he;
	struct in_addr **addr_list;

	he = gethostbyname(host);
	if (he == NULL) return false;
	addr_list = (struct in_addr **)he->h_addr_list;
	if (addr_list[0] == NULL) return false;
	ip->sin_family = AF_INET;
	memcpy(&ip->sin_addr, addr_list[0], sizeof(ip->sin_addr));
	return true;
}


LOCAL bool setup_connecting_server(IP_MGR_t *mgr, int8_t *addr, int16_t port) {
	struct sockaddr_in sAddr;
	bool ret = false;
	int timeout_ms = RECV_TIMEOUT;
	int tmp = 0;

	uint8_t status = wifi_station_get_connect_status();
	if (status == STATION_GOT_IP) {
		bzero(&sAddr, sizeof(struct sockaddr_in));

		if (!get_ip_from_hostname(addr, &sAddr)) {
			APP_TRACE_INFO(("get ip by hostname fail\n"));
			return false;
		}
		sAddr.sin_port = htons(port);
		mgr->socket = socket(PF_INET,SOCK_STREAM,0);
		if (mgr->socket == -1) {
			APP_TRACE_INFO(("ip socket fail\n"));
			close(mgr->socket);
			return false;
		}

		tmp = setsockopt(mgr->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_ms, sizeof(int));
		APP_TRACE_INFO(("setsocketopt: recv timeout, ret:%d\r\n", tmp));
		if(0 != connect(mgr->socket, (struct sockaddr *)(&sAddr), sizeof(struct sockaddr))) {
			APP_TRACE_INFO(("ip connect fail\n"));
			close(mgr->socket);
			return false;
		}
		APP_TRACE_INFO(("ip connect successfully\n"));
		mgr->connect = true;
		ret = true;
	}
	return ret;
}

LOCAL int connecting_server_resp(bool rc) {
	char buf[16];
	sprintf(buf, "%s\r\n", rc ? "OK" : "ERROR");
	printResp(buf);
	return 0;
}

LOCAL bool recv_msg_from_server(IP_MGR_t *mgr) {
	if (mgr->socket != -1 && mgr->connect) {
		uint8_t buf[56];
		int32_t ret;
		memset(buf, 0, 56);
		ret = recv(mgr->socket, buf, 56, 0);
		if (ret > 0) {
			uint8_t resp[64];
			memset(resp, 0, 64);
			APP_TRACE_INFO(("recv: %d, buf:%s\r\n", ret, buf));
			snprintf(resp, 64, "+IPD:%d,%s\r\n", ret, buf);
			printResp(resp);
		}
		return true;
	}
	return false;
}

LOCAL void ip_client_task(void *pvParameters) {
	bool rc = false;
	IP_AT_t msg;
	APP_TRACE_INFO(("---->%s\n", __func__));
	for (;;) {
		if (xQueueReceive(xQueueIP, &msg, 100/portTICK_RATE_MS ) == pdPASS) {
			switch (msg.cmd) {
			case IP_CONNECT_SERVER:
				APP_TRACE_INFO(("ip connect server: %s, %d\r\n", msg.ip_addr, msg.port));
				rc = setup_connecting_server(&ip_mgr, msg.ip_addr, msg.port);
				connecting_server_resp(rc);
				break;
			case IP_SEND_PAYLOD:
				APP_TRACE_INFO(("ip send payload: %s\r\n", msg.payload));
				rc = send_payload(&ip_mgr, msg.payload, msg.len);
				send_payload_resp(rc);
				break;
			case IP_CLOSE:
				APP_TRACE_INFO(("ip close\r\n"));
				rc = close_conn(&ip_mgr);
				close_conn_resp(rc);
				break;
			default:
				break;
			}
		}
		recv_msg_from_server(&ip_mgr);
	}
	vQueueDelete(xQueueIP);
	vTaskDelete(NULL);
}

void ip_at_init(void) {
	xQueueIP = xQueueCreate(32, sizeof(IP_AT_t));

	xTaskCreate(ip_client_task, "ip_client_task", 384, NULL, 2, NULL);
}
