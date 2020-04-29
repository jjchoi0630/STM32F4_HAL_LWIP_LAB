#include "MQTTInterface.h"
#include "stm32f4xx_hal.h"

#include <string.h>
#include "lwip.h"
#include "lwip/api.h"

#define MQTT_PORT	1883
#define SERVER_IP1	192
#define SERVER_IP2	168
#define SERVER_IP3	1
#define SERVER_IP4	227

uint32_t MilliTimer;

char TimerIsExpired(Timer *timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0);
}

void TimerCountdownMS(Timer *timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}

void TimerCountdown(Timer *timer, unsigned int timeout) {
	timer->end_time = MilliTimer + (timeout * 1000);
}

int TimerLeftMS(Timer *timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}

void TimerInit(Timer *timer) {
	timer->end_time = 0;
}

void NewNetwork(Network *n) {
	n->conn = NULL;
	n->mqttread = net_read;
	n->mqttwrite = net_write;
	n->disconnect = net_disconnect;
}

int net_read(Network *n, unsigned char *buffer, int len, int timeout_ms) {
	struct netbuf *buf;
	void *data;

	uint16_t nLen = 0; //buffer length
	uint16_t nRead = 0; //read buffer index

	while (netconn_recv(n->conn, &buf) == ERR_OK) //receive the response
	{
		do {
			netbuf_data(buf, &data, &nLen); //receive data pointer & length
			memcpy(buffer + nRead, data, nLen);
			nRead += nLen;
		} while (netbuf_next(buf) >= 0); //check buffer empty
		netbuf_delete(buf); //clear buffer
	}

	return nRead;
}

int net_write(Network *n, unsigned char *buffer, int len, int timeout_ms) {
	uint16_t nLen = 0; //buffer length
	uint16_t nWritten = 0; //write buffer index

	do {
		if (netconn_write_partly(n->conn, //connection
				(const void*) (buffer + nWritten), //buffer pointer
				(len - nWritten), //buffer length
				NETCONN_NOFLAG, //no copy
				(size_t*) &nLen) != ERR_OK) //written len
				{
			return -1;
		} else {
			nWritten += nLen;
		}
	} while (nWritten < len); //send request

	return nWritten;
}

void net_disconnect(Network *n) {
	netconn_close(n->conn); //close session
	netconn_delete(n->conn); //free memory
}

int ConnectNetwork(Network *n, char *ip, int port) {
	err_t err;
	ip_addr_t server_ip;

	ipaddr_aton(ip, &server_ip);

	n->conn = netconn_new(NETCONN_TCP);
	if (n->conn != NULL) {
		err = netconn_connect(n->conn, &server_ip, port);

		if (err != ERR_OK) {
			netconn_delete(n->conn); //free memory
			return -1;
		}
	}

	return 0;
}