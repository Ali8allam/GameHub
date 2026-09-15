/*
 * ESP8266_int.h
 *
 *  Created on: Sep 8, 2026
 *      Author: ALI & ADHM
 */

#ifndef HAL_ESP8266_ESP8266_INT_H_
#define HAL_ESP8266_ESP8266_INT_H_



void HESP_vInit();
// connect to wifi network
void HESP_vConnectAccessPoint(char* A_s8SSID , char* A_s8Password);
// top
void HESP_vOpenServerTCPConnection(char* IP , char* SocketNo);

void HESP_vSendHttpRequest(char* URL);


#endif /* HAL_ESP8266_ESP8266_INT_H_ */
