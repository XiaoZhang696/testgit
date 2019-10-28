#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/tcp.h>
#include <linux/socket.h>
#include <errno.h>
#include <Log.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

#include "NetClient.h"


#define CONNECT_TIMEOUT_COMPLICATED

enum {
	TCP_ESTABLISHED = 1,
	TCP_SYN_SENT,
	TCP_SYN_RECV,
	TCP_FIN_WAIT1,
	TCP_FIN_WAIT2,
	TCP_TIME_WAIT,
	TCP_CLOSE,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_LISTEN,
	TCP_CLOSING,   /* Now a valid state */
	TCP_MAX_STATES /* Leave at the end! */
};



int ConnectDataServer(char *pServerIP, uint16_t iPort, int protocal)
{
	static int udp_sock = -1;
    	int conn_sock = -1;
    	struct sockaddr_in server_addr;
	if(protocal == SOCK_DGRAM){
		ZDBG("UDP socket create....\n");
		if(udp_sock<0){
			udp_sock =  socket(AF_INET, protocal, 0);
			if(udp_sock <0){
				ZERR("UDP socket create fail.\n");
				return udp_sock;
			}else{
				ZDBG("UDP socket create success.\n");
				return udp_sock;
			}
		}else{
			ZDBG("UDP socket has been created, directly return the old.\n");
			return udp_sock;
		}
	}
	
	ZDBG("TCP socket create...\n");
    	conn_sock = socket(AF_INET, protocal, 0);
   	if (conn_sock < 0) {
        		ZERR("socket create error.\n");
        		goto create_err;
	}
	ZDBG("TCP socket create success.\n");
	FILL_SOCKADDR_IN(server_addr, pServerIP, iPort);	
	ZDBG("begin connect server %s:%d\n", pServerIP, iPort);
#if 0           // 阻塞模式
    	if (connect(conn_sock,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
	{
		ZERR("socket connect error,%s\n",strerror(errno));
        	goto create_err;
    	}
	
	ZDBG("Connect server %s:%d succeed\n", pServerIP, iPort);
	return conn_sock;
#endif
#ifdef CONNECT_TIMEOUT_COMPLICATED			// 非阻塞模式
	int error = -1, len;
  	len = sizeof(int);
  	struct timeval tm;
  	fd_set set;
  	unsigned long ul = 1;
  	ioctl(conn_sock, FIONBIO, &ul); //设置为非阻塞模式
	if( connect(conn_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		tm.tv_sec = 0;
		tm.tv_usec = 600000;
  		FD_ZERO(&set);
  		FD_SET(conn_sock, &set);
  		if( select(conn_sock + 1, NULL, &set, NULL, &tm) > 0){
    			getsockopt(conn_sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
    			if(error == 0) {
				KeepLiveSocket(conn_sock);

				int snd_size = 0;   /* 发送缓冲区大小 */ 
				socklen_t optlen;    /* 选项值长度 */ 
				/* 设置发送缓冲区大小 */ 
    				snd_size = 4*1024;    /* 发送缓冲区大小为4K */ 
    				optlen = sizeof(snd_size); 
   				int ret = setsockopt(conn_sock, SOL_SOCKET, SO_SNDBUF, &snd_size, optlen); 
    				if(ret < 0){ 
        					ZERR("set server:%s:%d, socket:%d send buffer size failed.\n", pServerIP, iPort, conn_sock); 
    				} 

				struct timeval timeout={0, 500000}; //500ms
    				ret = setsockopt(conn_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
				if(ret < 0){ 
        					ZERR("set server:%s:%d, socket:%d send timeout failed.\n", pServerIP, iPort, conn_sock); 
    				} 
			
				ul = 0;
				ioctl(conn_sock, FIONBIO, &ul); //设置为阻塞模式
				ZDBG("Connect server %s:%d succeed.\n", pServerIP, iPort);
				return conn_sock;
			} else{
				close(conn_sock);
				ZERR("Connect server %s:%d failed.\n", pServerIP, iPort);
    				return -1000;
			}
		}else{
			close(conn_sock);
			ZERR("Connect server %s:%d failed\n", pServerIP, iPort);
    			return -2000;
		}
	} else{
		KeepLiveSocket(conn_sock);

		int snd_size = 0;   /* 发送缓冲区大小 */ 
		socklen_t optlen;    /* 选项值长度 */ 
		/* 设置发送缓冲区大小 */ 
    		snd_size = 4*1024;    /* 发送缓冲区大小为16K */ 
    		optlen = sizeof(snd_size); 
   		int ret = setsockopt(conn_sock, SOL_SOCKET, SO_SNDBUF, &snd_size, optlen); 
    		if(ret < 0){ 
        			ZERR("set server:%s:%d, socket:%d send buffer size failed.\n", pServerIP, iPort, conn_sock); 
    		} 

		struct timeval timeout={0, 500000}; //500ms
    		ret = setsockopt(conn_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
		if(ret < 0){ 
        			ZERR("set server:%s:%d socket:%d send timeout failed.\n", pServerIP, iPort, conn_sock); 
    		} 
	
		ul = 0;
		ioctl(conn_sock, FIONBIO, &ul); //设置为阻塞模式
		ZDBG("Connect server %s:%d succeed.\n", pServerIP, iPort);
		return conn_sock;
	}
#endif

 err:
    close(conn_sock);
 create_err:
    ZERR("client error: %s\n", strerror(errno));
    return -3000;
}

int ClientConnectToServerWithTimeout(char *pServerIP, uint16_t iPort,int protocal,int tms)
{
	static int udp_sock = -1;
    	int conn_sock;
    	struct sockaddr_in server_addr;
	if(protocal == SOCK_DGRAM){
		ZDBG("UDP socket create....\n");
		if(udp_sock<0){
			udp_sock =  socket(AF_INET, protocal, 0);
			if(udp_sock <0){
				ZERR("UDP socket create fail\n");
				return udp_sock;
			}else{
				ZDBG("UDP socket create success\n");
				return udp_sock;
			}
		}else{
			ZDBG("UDP socket has been created direct return the old\n");
			return udp_sock;
		}
	}
	ZDBG("TCP socket create...\n");
    	conn_sock = socket(AF_INET, protocal, 0);
    	if (conn_sock < 0) {
        		ZERR("socket create error\n");
		goto create_err;
    	}
	ZDBG("TCP socket create success\n");
	FILL_SOCKADDR_IN(server_addr,pServerIP,iPort);	
	ZDBG("begin connect server %s:%d\n", pServerIP, iPort);
	int error = -1;
	int len = 0;
	struct timeval tm;
	fd_set wr;
	unsigned long ul = 1;
	int flags = fcntl(conn_sock,F_GETFL,0);
	fcntl(conn_sock,F_SETFL,flags|O_NONBLOCK);
    	if (connect(conn_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        		ZDBG("socket connect io noblock....\n");
		tm.tv_sec=tms;
		tm.tv_usec = 0;
		FD_ZERO(&wr);
		FD_SET(conn_sock,&wr);
		if(select(conn_sock+1,NULL,&wr,NULL,&tm) > 0){
			if(getsockopt(conn_sock,SOL_SOCKET,SO_ERROR,&error,(socklen_t*)&len) < 0){
				goto create_err;
			}
			
			if(error == 0){
				ZDBG("Connect server %s:%d succeed\n", pServerIP, iPort);
				fcntl(conn_sock,F_SETFL,flags);
				return conn_sock;
			}else{
				ZDBG("Connect server %s:%d failed\n", pServerIP, iPort);
				goto create_err;
			}
		}
		goto create_err;
	}
	ZDBG("Connect server %s:%d succeed\n", pServerIP, iPort);
	fcntl(conn_sock,F_SETFL,flags);
	return conn_sock;

 create_err:
	ZERR("client error,%s\n", strerror(errno));
	fcntl(conn_sock, F_SETFL, flags);
    	close(conn_sock);
    	return -1000;
}

int UDPSetSendPort(int sockfd,int iPort)
{
	struct sockaddr_in send_addr = {0};
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(iPort);
	send_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	int res = bind(sockfd,(struct sockaddr*)(&send_addr),sizeof(send_addr));
	if(0==res){
		ZDBG("UDP socket 绑定端口%d成功！！\n", iPort);
	}else{
		ZERR("UDP socket 绑定端口%d失败:%s！！\n", iPort, strerror(errno));
	}
	return res;
}

int MyWrite(int protocalType, int sockfd,char *pdata, int iLen, char* serverIp, int iPort)
{
	if(protocalType == SOCK_DGRAM){
		ZDBG("Send to %s:%d with UDP protocal\n", serverIp, iPort);
		struct sockaddr_in	server_addr;
		FILL_SOCKADDR_IN(server_addr,serverIp,iPort);
		return sendto(sockfd,pdata,iLen, 0, (void *)&server_addr, sizeof(server_addr));
	}else{
		ZDBG("Send to %s:%d with TCP protocal\n", serverIp, iPort);
		return send(sockfd, pdata, iLen, 0);
	}
}

int MyRead(int protocalType, int sockfd, char *pdata, int iLen, char* serverIp, int* iPort)
{
	if(protocalType == SOCK_DGRAM){
		if(serverIp == NULL){
			return -1;
		}
		ZDBG("Reveive with UDP protocal\n");
		struct sockaddr_in stRemoteAddr = {0};
		socklen_t iRemoteAddrLen = sizeof(stRemoteAddr);
		int iRecvLen = recvfrom(sockfd, pdata,iLen, 0, (void *)&stRemoteAddr, &iRemoteAddrLen);
		*iPort = ntohs(stRemoteAddr.sin_port);
		strcpy(serverIp,(char*)inet_ntoa(stRemoteAddr.sin_addr));//.s_addr
		ZDBG("Server IP %s,Port %d\n",serverIp, *iPort);
		return iRecvLen;
	}else{
		ZDBG("Receive from %s:%d with TCP protocal\n", serverIp, *iPort);
		return recv(sockfd, pdata, iLen, 0);
	}
}

int KeepLiveSocket(int sock){
	/*
	
	int keepalive = 1; // 开启keepalive属性
	int keepidle = 60; // 如该连接在60秒内没有任何数据往来,则进行探测
	int keepinterval = 5; // 探测时发包的时间间隔为5 秒
	int keepcount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	*/
	int keepalive = 1;
	int keepidle = 1;
	int keepinterval = 1;
	int keepcount = 1;
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	setsockopt(sock, IPPROTO_TCP/*SOL_TCP*/, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle ));
	setsockopt(sock, IPPROTO_TCP/*SOL_TCP*/, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
	setsockopt(sock, IPPROTO_TCP/*SOL_TCP*/, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

	return 0;
}

int SocketConnected(int sock){
	return 0;
	
	struct tcp_info info;
	int len = sizeof(info);

	if(sock <= 0){
		ZERR("Invalid socket: %d.\n", sock);
		return -1;
	}
	getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
	if((info.tcpi_state==TCP_ESTABLISHED)){
		return 0;
	}else{
		ZERR("Socket %d disconnected.\n", sock);
		return -2;
	}
}

