#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <string.h>
#include <process.h>
#include <time.h>
#pragma warning (disable : 4996)
#pragma comment (lib, "ws2_32.lib")

#define		MSG_BUFF					1024
#define		BUFF_SIZE					2048
#define		BUFF_SMALL					100
#define		DEVICE_NUM					100
#define		MAX_THREAD_NUM				100
#define		DEVICE_ATTRIBUTE_LENGTH		10

void StatesCodeInit(int* nRes, int* errorCode);
unsigned WINAPI RequestHandler(void* arg);
char* ContentType(char* file);
void SendData(SOCKET sock, char* ct, char* filename);
void SendDeviceData(SOCKET sock);
void SendErrorMSG(SOCKET sock);
void DeviceMsgProcess(char* msg);
void ThreadMonitor(void* arg);

typedef struct
{
	int		deviceNum;
	_Bool	states;
	_Bool	isWrite;
	float	temperature;
	float	humidity;
	float	PH;
}DEVICE;

typedef struct
{
	HANDLE hThread;
	DWORD hThreadID;
	_Bool threadSates;
}THREAD;

DEVICE	device[DEVICE_NUM];

int main(int agrc, char* agrv[])
{
	char*			serverPort		 =	 "8080";
	char*			serverIP		 =	 "127.0.0.1";
	WORD			word			 =	 MAKEWORD(2, 2);
	char			rxBuff[MSG_BUFF] =	 { 0 };
	char			txBuff[MSG_BUFF] =	 { 0 };
	int				nRes			 =	 0;
	int				errorCode		 =	 0;
	int				THREAD_NUM		 =	 1;
	WSADATA			WSADATA;
	SOCKET			serverSocket, clientSocket;
	SOCKADDR_IN		serverAddr, clientAddr;
	THREAD			threadPool[MAX_THREAD_NUM];
	int				clientSize;

	memset(device, 0, sizeof(device));
	memset(threadPool, 0, sizeof(threadPool));

	device[1].isWrite = 1;
	device[1].deviceNum = 2;
	device[1].temperature = 22.3;
	device[1].humidity = 19;
	device[1].PH = 7;
	device[1].states = 1;

	nRes = WSAStartup(word, &WSADATA);
	if (nRes != 0)
	{
		errorCode = WSAGetLastError();
		printf("WSAStartup() error! error code: %d\n", errorCode);
		return 0;
	}
	printf("WSAStartup() state code: %d\n", nRes);
	StatesCodeInit(&nRes, &errorCode);

	serverSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		errorCode = WSAGetLastError();
		printf("socket() error! error code: %d\n", errorCode);
		WSACleanup();
		return 0;
	}
	printf("socket over\n");
	StatesCodeInit(&nRes, &errorCode);

	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr(serverIP);
	serverAddr.sin_port = htons(atoi(serverPort));

	nRes = bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (nRes == SOCKET_ERROR)
	{
		errorCode = WSAGetLastError();
		printf("bind() error! error code: %d\n", errorCode);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("bind() state code: %d\n", nRes);
	StatesCodeInit(&nRes, &errorCode);

	nRes = listen(serverSocket, SOMAXCONN);
	if (nRes == SOCKET_ERROR)
	{
		errorCode = WSAGetLastError();
		printf("listen() error error! code: %d", errorCode);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("listen() state code: %d\n", nRes);
	StatesCodeInit(&nRes, &errorCode);

	threadPool[0].hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadMonitor, (void*)threadPool, 0, (unsigned*)&threadPool[0].hThreadID);
	
	time_t now_time;
	
	while (1)
	{
		clock_t start;
		clock_t finsh;
		double  time1;
		clientSize = sizeof(clientAddr);
		printf("׼������\n");
		start = clock();
		clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientSize);
		finsh = clock();
		time1 = (double)(finsh - start) / CLOCKS_PER_SEC;
		time(&now_time);
		printf("������� ��ʱ��%f\n ��ǰʱ��:%s\n", time1, ctime(&now_time));
		printf("Connection Request : %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		if (threadPool[THREAD_NUM].threadSates == 0)
		{
			threadPool[THREAD_NUM].hThread = (HANDLE)_beginthreadex(NULL, 0, RequestHandler, (void*)clientSocket, 0, (unsigned*)&threadPool[THREAD_NUM].hThreadID);
			if (threadPool[THREAD_NUM].hThread != NULL)
			{
				threadPool[THREAD_NUM].threadSates = 1;
				THREAD_NUM++;
				if (THREAD_NUM == (MAX_THREAD_NUM - 1))
					THREAD_NUM = 1;
			}
		}
	}
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}

void StatesCodeInit(int* nRes, int* errorCode)
{
	*nRes = 0;
	*errorCode = 0;
}

unsigned WINAPI RequestHandler(void* arg)
{
	SOCKET	clientSock = (SOCKET)arg;
	char	buf[BUFF_SIZE];
	char	method[BUFF_SMALL];
	char	ct[BUFF_SMALL];
	char	filename[BUFF_SMALL];
	char	deviceMsg[BUFF_SMALL];

	time_t now_time;

	recv(clientSock, buf, BUFF_SIZE, 0);
	if (strstr(buf, "HTTP/") == NULL)
	{
		SendErrorMSG(clientSock);
		closesocket(clientSock);
		return 1;
	}

	time(&now_time);
	printf("���Ľ������ʱ��:%s\n", ctime(&now_time));

	strcpy(method, strtok(buf, " /"));
	if (!(strcmp(method, "GET")))
	{
		strcpy(filename, strtok(NULL, " /"));
		if (!(strcmp(filename, "GET_EDVICE_DATA")))
		{
			SendDeviceData(clientSock);
			time(&now_time);
			printf("���������ʱ��:%s\n", ctime(&now_time));
			return 0;
		}
		else
		{
			strcpy(ct, ContentType(filename));
			SendData(clientSock, ct, filename);
			time(&now_time);
			printf("���������ʱ��:%s\n", ctime(&now_time));
			return 0;
		}
	}

	else if (!(strcmp(method, "POST")))
	{
		strcpy(filename, strtok(NULL, " /"));
		if (!(strcmp(filename, "GET_EDVICE_DATA")))
		{
			SendDeviceData(clientSock);
			return 0;
		}
		else
		{
			strcpy(ct, ContentType(filename));
			SendData(clientSock, ct, filename);
			return 0;
		}
	}

	else if (!(strcmp(method, "DEVICEMSG")))
	{
		strcpy(deviceMsg, strtok(NULL, " /"));
		DeviceMsgProcess(deviceMsg);
		return 0;
	}
	else
	{
		SendErrorMSG(clientSock);
	}
	return 0;
}

char* ContentType(char* file)
{
	char extension[BUFF_SMALL];
	char filename[BUFF_SMALL];
	strcpy(filename, file);
	strtok(filename, ".");
	strcpy(extension, strtok(NULL, "."));
	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
	{
		return "text/html";
	}
	else
	{
		return "text/plain";
	}
}

void SendData(SOCKET sock, char* ct, char* filename)
{
	char	protocol[]		=	"HTTP/1.0 200 OK\r\n";
	char	serverName[]	=	"Server:simple web server\r\n";
	char	teChunk[]		=	"Transfer-Encoding: chunked\r\n";
	char	endSign[]		=	"0\r\n\r\n";
	char	cntType[BUFF_SMALL];
	char	buf[BUFF_SIZE];
	FILE*	sendFile;

	sprintf(cntType, "Content-type:%s; charset=GBK\r\n\r\n", ct);
	if ((sendFile = fopen(filename,"r")) == NULL)
	{
		SendErrorMSG(sock);
		return;
	}

	send(sock, protocol, strlen(protocol), 0);
	send(sock, serverName, strlen(serverName), 0);
	send(sock, teChunk, strlen(teChunk), 0);
	send(sock, cntType, strlen(cntType), 0);

	while (fgets(buf, BUFF_SIZE, sendFile) != NULL)
	{
		send(sock, buf, strlen(buf), 0);
	}
	send(sock, endSign, strlen(endSign), 0);
	fclose(sendFile);
	closesocket(sock);
}

void SendErrorMSG(SOCKET sock)
{
	char	protocol[]		=	"HTTP/1.1 200 OK\r\n";
	char	serverName[]	=	"Server:simple web server\r\n";
	char	teChunk[]		=	"Transfer-Encoding: chunked\r\n";
	char	cntType[]		=	"Content-type:text/html\r\n\r\n";
	char	content[]		=	"<html><head><title>NETWORK</title></head>"
								"<body><font size=+5><br> �������󣡲鿴�����ļ�������ʽ!"
								"</font></body></html>";
	char	endSign[]		=	"\r\n\r\n";

	send(sock, protocol,	strlen(protocol),	0);
	send(sock, serverName,	strlen(serverName), 0);
	send(sock, teChunk,		strlen(teChunk),	0);
	send(sock, cntType,		strlen(cntType),	0);
	send(sock, content,		strlen(content),	0);
	send(sock, endSign,		strlen(endSign),	0);

	closesocket(sock);
}

void SendDeviceData(SOCKET sock)
{
	char	protocol[] = "HTTP/1.1 200 OK\r\n";
	char	serverName[] = "Server:simple web server\r\n";
	char	teChunk[] = "Transfer-Encoding: chunked\r\n";
	char    allowMethods[] = "Access-Control-Allow-Methods:GET, POST\r\n";
	char	allowOrigin[] = "Access-Control-Allow-Origin:*\r\n";
	char	allowHeaders[] = "Access-Control-Allow-Headers: Content-Type,Transfer-Encoding\r\n";
	char	allowCredentials[] = "Access-Control-Allow-Credentials: true\r\n";
	char	cntType[] = "Content-type:text/plain; charset=utf-8\r\n\r\n";
	char	endSign[] = "0\r\n"
						"\r\n";
	char	buf[BUFF_SIZE];
	char	buflength[BUFF_SMALL];

	send(sock, protocol, strlen(protocol), 0);
	send(sock, serverName, strlen(serverName), 0);
	send(sock, teChunk, strlen(teChunk), 0);
	send(sock, allowOrigin, strlen(allowOrigin), 0);
	send(sock, allowMethods, strlen(allowMethods), 0);
	send(sock, allowHeaders, strlen(allowHeaders), 0);
	send(sock, allowCredentials, strlen(allowCredentials), 0);
	send(sock, cntType, strlen(cntType), 0);

	for (int i = 0; i < DEVICE_NUM; i++)
	{
		if (device[i].isWrite == TRUE)
		{
			sprintf(buf, "%d&%d&%f&%f&%f\r\n", device[i].deviceNum, device[i].states, device[i].temperature, device[i].humidity, device[i].PH);
			sprintf(buflength, "%x\r\n", (strlen(buf)-2));
			send(sock, buflength, strlen(buflength), 0);
			send(sock, buf, strlen(buf), 0);
		}
		else
		{
			continue;
		}
	}
	send(sock, endSign, strlen(endSign), 0);
	closesocket(sock);
}

void DeviceMsgProcess(char* msg)
{
	int		num;
	char	deviceNum[DEVICE_ATTRIBUTE_LENGTH];
	char	states[DEVICE_ATTRIBUTE_LENGTH];
	char	temperature[DEVICE_ATTRIBUTE_LENGTH];
	char	humidity[DEVICE_ATTRIBUTE_LENGTH];
	char	PH[DEVICE_ATTRIBUTE_LENGTH];

	strcpy(deviceNum, strtok(msg, "&"));
	strcpy(states, strtok(NULL, "&"));
	strcpy(temperature, strtok(NULL, "&"));
	strcpy(humidity, strtok(NULL, "&"));
	strcpy(PH, strtok(NULL, "&"));

	num = atoi(deviceNum);
	device[num].deviceNum = num;
	device[num].states = (_Bool)atoi(states);
	device[num].temperature = atof(temperature);
	device[num].humidity = atof(humidity);
	device[num].PH = atof(PH);
	device[num].isWrite = 1;
}

void ThreadMonitor(void* arg)
{
	THREAD* threadPool = (THREAD*)arg;
	threadPool[0].threadSates = 1;
	while (1)
	{
		for (int i = 0; i < MAX_THREAD_NUM; i++)
		{
			if ((WaitForSingleObject(threadPool[i].hThread, 100) == WAIT_OBJECT_0)&&(threadPool[i].threadSates == 1))
			{
				threadPool[i].threadSates = 0;
			}
		}
	}
}