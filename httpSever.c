#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <string.h>
#include <process.h>

#pragma warning (disable : 4996)
#pragma comment (lib, "ws2_32.lib")

#define MSG_BUFF	1024
#define BUFF_SIZE	2048
#define BUFF_SMALL	100

void StatesCodeInit(int* nRes, int* errorCode);
unsigned WINAPI RequestHandler(void* arg);
char* ContentType(char* file);
void SendData(SOCKET sock, char* ct, char* filename);
void SendErrorMSG(SOCKET sock);

int main(int agrc, char* agrv[])
{
	char*			serverPort		 =	 "8080";
	char*			serverIP		 =	 "127.0.0.1";
	WORD			word			 =	 MAKEWORD(2, 2);
	char			rxBuff[MSG_BUFF] =	 { 0 };
	char			txBuff[MSG_BUFF] =	 { 0 };
	int				nRes			 =	 0;
	int				errorCode		 =	 0;
	WSADATA			WSADATA;
	SOCKET			serverSocket, clientSocket;
	SOCKADDR_IN		serverAddr, clientAddr;
	HANDLE			hThread;
	DWORD			hThreadID;
	int				clientSize;

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

	while (1)
	{
		clientSize = sizeof(clientAddr);
		clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientSize);
		printf("Connection Request : %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		hThread = (HANDLE)_beginthreadex(NULL, 0, RequestHandler, (void*)clientSocket, 0, (unsigned*)&hThreadID);

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

	recv(clientSock, buf, BUFF_SIZE, 0);
	if (strstr(buf, "HTTP/") == NULL)
	{
		SendErrorMSG(clientSock);
		closesocket(clientSock);
		return 1;
	}
	strcpy(method, strtok(buf, " /"));
	if (strcmp(method, "GET"))
	{
		SendErrorMSG(clientSock);
	}

	strcpy(filename, strtok(NULL, " /"));
	strcpy(ct, ContentType(filename));
	SendData(clientSock, ct, filename);
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
	long size = 0;
	char	protocol[]		=	"HTTP/1.0 200 OK\r\n";
	char	serverName[]	=	"Server:simple web server\r\n";
	char	cntLen[]		=	"Content-length:20480\r\n";
	char	teChunk[]		=	"Transfer-Encoding: chunked\r\n";
	char	endSign[]		=	"\r\n\r\n";
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
	closesocket(sock);
}

void SendErrorMSG(SOCKET sock)
{
	char	protocol[]		=	"HTTP/1.0 200 OK\r\n";
	char	serverName[]	=	"Server:simple web server\r\n";
	char	teChunk[]		=	"Transfer-Encoding: chunked\r\n";
	char	cntType[]		=	"Content-type:text/html\r\n\r\n";
	char	content[]		=	"<html><head><title>NETWORK</title></head>"
								"<body><font size=+5><br> 发生错误！查看请求文件和请求方式!"
								"</font></body></html>";
	char	endSign[]		=	"\r\n\r\n";

	send(sock, protocol, strlen(protocol), 0);
	send(sock, serverName, strlen(serverName), 0);
	send(sock, teChunk, strlen(teChunk), 0);
	send(sock, cntType, strlen(cntType), 0);
	send(sock, content, strlen(content), 0);
	send(sock, endSign, strlen(endSign), 0);

	closesocket(sock);
}