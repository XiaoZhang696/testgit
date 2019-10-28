//client
 
#include<stdio.h>
 
#include<string.h>
 
#include<sys/types.h>
 
#include<sys/socket.h>
 
#include<sys/un.h>

//#define UNIX_DOMAIN "./UNIX.domain"
int ConnectLocalServer(const char* szLocalFileName){ 
	int connect_fd;	 
	int ret;	 
	int i;	 
	static struct sockaddr_un srv_addr;
	 
	// creat unix socket	 
	connect_fd = socket(PF_UNIX, SOCK_STREAM, 0);	 
	if(connect_fd < 0)
	{
		perror("cannot creat socket");
		return -1;
	 
	}
	 
	srv_addr.sun_family = AF_UNIX;	 
	strcpy(srv_addr.sun_path, szLocalFileName);
	 
	//connect server	 
	ret = connect(connect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if (ret < 0)
	{	 
		perror("cannot connect server");		 
		close(connect_fd);		 
		return -1;	 
	}
	return connect_fd;
}

void DisConnectLocalServer(int connect_fd)
{
	close(connect_fd);
}