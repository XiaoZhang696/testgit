#ifndef NETCLIENT_H
#define NETCLIENT_H

#define FILL_SOCKADDR_IN(server_addr,szIP,iPort)  memset(&server_addr, 0, sizeof(server_addr)); \
	server_addr.sin_family		= AF_INET; \
	server_addr.sin_port		= htons(iPort); \
	server_addr.sin_addr.s_addr	= inet_addr(szIP)
		
int ConnectDataServer(char *pServerIP, uint16_t iPort,int protocal);
int UDPSetSendPort(int sockfd,int iPort);
int MyWrite(int protocalType, int sockfd, char *pdata, int iLen, char* serverIp, int iPort);
int MyRead(int protocalType, int sockfd, char *pdata, int iLen, char* serverIp, int* iPort);
int KeepLiveSocket(int sock);
int SocketConnected(int sock);
#endif