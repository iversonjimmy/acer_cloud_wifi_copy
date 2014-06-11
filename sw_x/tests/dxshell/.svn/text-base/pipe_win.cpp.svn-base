#include <stdio.h>
#include <Winsock2.h>

#include "log.h"

int read(int fd, char *buf, int len)
{
	return recv(fd, buf, len, 0);
}

 int write(int fd, char *buf, int len)
{
	return send(fd, buf, len, 0);
}

void close(int fd)
{
	closesocket(fd);
}

int pipe(int fildes[2])
{
	int tcp1, tcp2;
	sockaddr_in name;

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		LOG_ERROR("FAIL: WSAStartup failed with error: %d", err);
		goto clean;
	}

	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	int namelen = sizeof(name);
	tcp1 = tcp2 = -1;

	int tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp == -1){
		int err = WSAGetLastError();
		LOG_ERROR("FAIL: Fail to create socket: %d\n", err);
		goto clean;
	}
	if (bind(tcp, (sockaddr*)&name, namelen) == -1){
		goto clean;
	}
	if (listen(tcp, 5) == -1){
		goto clean;
	}
	if (getsockname(tcp, (sockaddr*)&name, &namelen) == -1){
		goto clean;
	}
	tcp1 = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp1 == -1){
		goto clean;
	}
	if (-1 == connect(tcp1, (sockaddr*)&name, namelen)){
		goto clean;
	}

	tcp2 = accept(tcp, (sockaddr*)&name, &namelen);
	if (tcp2 == -1){
		goto clean;
	}
	if (closesocket(tcp) == -1){
		goto clean;
	}

	fildes[0] = tcp1;
	fildes[1] = tcp2;

	return 0;
clean:
	if (tcp != -1){
		closesocket(tcp);
	}
	if (tcp2 != -1){
		closesocket(tcp2);
	}
	if (tcp1 != -1){
		closesocket(tcp1);
	}
	return -1;
}
