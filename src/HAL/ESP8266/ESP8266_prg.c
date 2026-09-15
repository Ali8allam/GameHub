#include "../../LIB/STD_TYPES.h"
#include "../../LIB/BIT_MATH.h"

#include "../../MCAL/SYSTICK/SYSTICK_int.h"

#include "ESP8266_int.h"

static  u16  StringLength(const char* str)
{
	u16 len=0;

	while(str[len]!='\0')
	{
		len++;
	}
	return len;
}
static void Convert_to_String(u16 val , char* outstr)
{
	char temp[6];
	s8 i=0;
	s8 j=0;
	if(val ==0)
	{
		outstr[0] ='0';
		outstr[0] ='\0';
		return;
	}
	//12
	while(val>0)
	{
		temp[i++]=(val%10) +'0' ;
		val/=10;
	}
	for(j=0 ;j<i ; j++)
	{
		outstr[j] = temp[i-1-j];
	}

	//outstr = temp;
}
void HESP_vInit()
{
	MSYSTICK_Config_t STK_CFG={
			.InterruptEnable =INT_DISABLE,
			.CLK_SRC = CLK_SRC_AHB_8
	};
	MSYSTICK_vInit(&STK_CFG);
	MUSART_vInit();

	// disable echo
	MUSART_vSendString("ATE0\r\n");
	MSYSTICK_vSetDelay_ms(1000);

	// station mode
	MUSART_vSendString("AT+CWMODE=1\r\n");
	MSYSTICK_vSetDelay_ms(1000);

}
void HESP_vConnectAccessPoint(char* A_s8SSID , char* A_s8Password)
{
	// AT+CWJAP= "SSID","A_s8Password"
	MUSART_vSendString("AT+CWJAP= \"");
	MUSART_vSendString(A_s8SSID);
	MUSART_vSendString("\",\"");
	MUSART_vSendString(A_s8Password);
	MUSART_vSendString("\"\r\n");

	MSYSTICK_vSetDelay_ms(500);
}

// top
void HESP_vOpenServerTCPConnection(char* IP , char* SocketNo)
{
	MUSART_vSendString("AT+CIPSTART= \"TCP\",\"");
	MUSART_vSendString(IP);
	MUSART_vSendString("\",\"");
	MUSART_vSendString(SocketNo);
	MUSART_vSendString("\r\n");

	MSYSTICK_vSetDelay_ms(5000);
}


void HESP_vSendHttpRequest(char* URL)
{
	u16 URLLength = StringLength(URL) +6;
	u8 numString[6];

	Convert_to_String( URLLength, numString);

	// AT_CIPSEND = #
	//GET URL
	MUSART_vSendString("AT_CIPSEND =");
	MUSART_vSendString(numString);
	MUSART_vSendString("\r\n");

	MSYSTICK_vSetDelay_ms(1000);

	// HTTP REQ
	MUSART_vSendString("GET ");
	MUSART_vSendString(URL);
	MUSART_vSendString("\r\n");
	MSYSTICK_vSetDelay_ms(2000);


}

