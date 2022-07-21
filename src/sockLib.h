#pragma once
#include <iostream>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")


#define ExitCode 0xffff
#define ExitStr "close"
#define BUF_SIZE 1024
#define DEBUG
#define TRACE(fmt,...) debugPrint(##fmt,__FILE__,__LINE__, ##__VA_ARGS__)

enum class Error {
	Success = 0,
	WSAStartup,
	SocketServer,
	SocketClient,
	Bind,
	Listen,
	Accept,
	Recv,
	Send,
	Malloc,
	Createprocess,
	Createpipe,
	Close,
	Relloc,
	Shell,
	Connect,
	SSlConnect,
	Accepts,
	SSLcert,
	SSLPrivateKey
};

void debugPrint(Error e, CONST CHAR* path, INT line);

void delay();



class _Socket
{
public:
	_Socket();
	~_Socket();

	BOOL _Recv(PCHAR &recvBuf,INT &size);

	BOOL Send(CONST CHAR *sendBuf);

	PCHAR Recv();

	VOID Print(PCHAR buf);


	PCHAR Mylloc(PCHAR szbuf, INT srcSize, INT dstSize);
private:
	typedef struct _Msg
	{
		DWORD nLen;  // msg总体长度
		PCHAR data;  // 从此处开始是消息的内容
	}Msg, * PMsg;

	PMsg _msg;
	WSADATA WSAData;
public:
	INT recvSize;
	SOCKET hSocket; // Server和client的socket不同所以在上层赋值
};



class Client:public _Socket
{
public:
	Client() {};
	Client(PCHAR ipaddr, WORD port);
	~Client();

	std::string Shell(PCHAR cmd);

private:
	sockaddr_in addrServer;
	PCHAR recvBuf = nullptr;
	std::string sendBuf;
};


class Server:private _Socket
{
public:
	Server(WORD port);
	~Server();

	Error Comminicate();

private:
	PCHAR recvBuf;
	PCHAR outBuf;
	SOCKET hServer;
	sockaddr_in addrServer;
	sockaddr_in addrClient;
};

