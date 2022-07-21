#pragma once
#include <openssl/ssl.h>
#include "sockLib.h"

#define cert_path "cacert.pem"
#define privkey_path "privkey.pem"

class HttpsClient:public Client
{
public:
	HttpsClient(PCHAR ipaddr, WORD port);
	VOID Main();

	PCHAR Recv();

	BOOL Send(CONST CHAR* sendBuf);

	~HttpsClient();

private:
	sockaddr_in addrServer;
	PCHAR recvBuf;
	std::string sendBuf;
	SSL_CTX* sslctx;
	SSL* cSSL;
	INT nRet;
};


class HttpsServer:public _Socket
{
public:
	HttpsServer(WORD port);
	~HttpsServer();

	Error Comminicate();

	PCHAR Recv();

	BOOL Send(CONST CHAR* sendBuf);

	VOID ConfigCtx(SSL_CTX* ctx);
private:
	PCHAR recvBuf;
	SOCKET hServer;
	sockaddr_in addrServer;
	sockaddr_in addrClient;
	SSL_CTX* sslctx;
	SSL* cSSL;
	INT nRet;
};

