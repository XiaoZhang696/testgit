#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>

#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include<sys/un.h>

#include <signal.h>

#include "cJSON.h"
#include "../Common/Log.h"
#include "../Common/NetClient.h"
#include "GlobalVar.h"


#include "queue.h"

#define DEV_NAME "/dev/ttymxc3"

#define EXPORT_PATH	"/sys/class/gpio/export"			//GPIO设备导出设备
#define VAL_PATH	"/sys/class/gpio/gpio134/value"		//GPIO输入输出电平值设备
#define DIR_PATH	"/sys/class/gpio/gpio134/direction"	//GPIO输入输出控制设备 

#define PORT 		5000         		// The port which is communicate with server
#define LENGTH 	256          			// Buffer length
#define SPLITER "####"

#define OUT		"out"		//输出
#define GPIO_PIN	"134"	//GPIO5_6
#define HIGH		"1"		//高电平
#define LOW		"0"		//低电平

void Soft_Led_Init(void)
{
	int fd_export, fd_dir;

	fd_export = open(EXPORT_PATH, O_WRONLY);	//打开GPIO导出设备
	if(fd_export < 0)
	{
		perror("open export:");
		return;
	}

	write(fd_export, GPIO_PIN, sizeof(GPIO_PIN));	//导出GPIO5_6
	close(fd_export);								//关闭设备

	fd_dir = open(DIR_PATH, O_RDWR);	//打开输入输出设备
	if(fd_dir < 0)
	{
		perror("open direction:");
		return;
	}

	/* ZDBG("Write OUT to %s.", DIR_PATH); */
	write(fd_dir, OUT, sizeof(OUT));	//写入输出
	fsync(fd_dir); 						//文件数据同步
	close(fd_dir);

}

void Led_On(void)
{
	int fd_val;

	fd_val = open(VAL_PATH, O_RDWR);	//打开电平值设备
	if(fd_val < 0)
	{
		perror("open value:");
		return;
	}
	/* ZDBG("Write HIGH to %s.", VAL_PATH); */
	write(fd_val, HIGH, sizeof(HIGH));	//高电平，LED点亮

	close(fd_val);
}

void Led_Off(void)
{
	int fd_val;
	
	fd_val = open(VAL_PATH, O_RDWR);	//打开电平值设备
	if(fd_val < 0)
	{
		perror("open value:");
		return;
	}

	/* ZDBG("Write LOW to %s.", VAL_PATH); */
	write(fd_val, LOW, sizeof(LOW));	//低电平,LED熄灭
	
	close(fd_val);
}

typedef enum _workpattern
{
	ROUTE = 0,
	QUERY
} WORKPATTERN;

cJSON* CreateCommandHead(char* pCmd)
{
	cJSON * root =	cJSON_CreateObject();
	cJSON_AddItemToObject(root, "command", cJSON_CreateString(pCmd));//根节点下添加
	cJSON_AddItemToObject(root, "time", cJSON_CreateNumber(time(NULL)));//根节点下添加
	return root;
}
cJSON* MarkServer(cJSON* root, int iServerIndex)
{
	cJSON_AddItemToObject(root, "IP", cJSON_CreateString(g_pSrvAddr[iServerIndex]));//根节点下添加
	cJSON_AddItemToObject(root, "PORT", cJSON_CreateNumber(g_SrvPorts[iServerIndex]));//根节点下添加
	return root;
}
cJSON* AddStringItem(cJSON* root,char* pKey,char* pVal)
{
	cJSON_AddItemToObject(root, pKey, cJSON_CreateString(pVal));
}
cJSON* AddIntItem(cJSON* root,char* pKey,long nVal)
{
	cJSON_AddItemToObject(root, pKey, cJSON_CreateNumber(nVal));
	return root;
}
cJSON* AddfloatItem(cJSON* root,char* pKey,double dbVal)
{
	cJSON_AddItemToObject(root, pKey, cJSON_CreateNumber(dbVal));
	return root;
}
char* GetStringVal(cJSON* root,char* pKey)
{
	char* val = ((cJSON *)(cJSON_GetObjectItem(root,pKey)))->valuestring;
	return val;
}
long GetIntVal(cJSON* root,char* pKey)
{
	return ((cJSON *)(cJSON_GetObjectItem(root,pKey)))->valueint;
}
int GetServerIndex(char* pIp, long port)
{
	int i = 0;
	for(i = 0; i < g_srvCount; i++)
	{
		//if(strstr(&(g_pSrvAddr[i][0]), pIp))
		if(strstr(g_pSrvAddr[i], pIp))
		{
			if(g_SrvPorts[i] == port)
			{
				return i;
			}
		}
	}
	return -1;
}

int ConnectLocalServer(const char* szLocalFileName){ 
	int connect_fd;
	int ret;
	int i;
	struct sockaddr_un srv_addr;
 
	// creat unix socket
	connect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(connect_fd < 0)
	{
		ZERR("cannot creat socket\n");
		return -1;
	}
	 
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, szLocalFileName);

	//connect server
	ret = connect(connect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if (ret < 0)
	{
		ZERR("cannot connect server\n");
		close(connect_fd);
		return -1;
	}
	return connect_fd;
}
void DisConnectLocalServer(int connect_fd)
{
	close(connect_fd);
}

int IsHandelReadable(int handle, int waitus)
{
	fd_set rd;
	FD_ZERO(&rd);
	FD_SET(handle, &rd);
	struct timeval tv;
	tv.tv_sec = waitus/1000000;
	tv.tv_usec = waitus%1000000;
	if(select(handle + 1, &rd, NULL, NULL, &tv) > 0){
		ZDBG("select succeed.\n");
		if(FD_ISSET(handle, &rd))
		{
			ZDBG("Handle %d is readable.\n", handle);
			return 1;
		}
	}
	ZDBG("Handle %d is NOT readable.\n", handle);
	return -1;
}

#define MAX_RETRYTIMES 3
int ErrorProcess(int iActive,int* iRetryTimes)
{
	int protocal = g_svrProtocol[iActive];
	int hTempSrv = 0;
	if(protocal == SOCK_DGRAM)
	{
		ZERR("this case need to retry[%d]...\n",(*iRetryTimes));
		if((*iRetryTimes)> MAX_RETRYTIMES)
		{
			ZERR("retry times expired.\n");
			return -1;
		}
		else
		{
			ZERR("ready to retry.\n");
			return 0;
		}
	}
	else
	{
	
		ZERR("this case need to reconnect[%d].\n", (*iRetryTimes));
		if((*iRetryTimes)> MAX_RETRYTIMES)
		{
			ZERR("retry times expired[%d].\n", (*iRetryTimes));
			return -1;
		}
		else
		{
INTERNAL_RETRY:
			//reconnect
			if(g_hServer[iActive] > 0)
			{
				close(g_hServer[iActive]);
				g_hServer[iActive] = -1;
			}
			g_bServerState[iActive] = INACTIVE;
			g_ServerWaitCount[iActive] = 0;
			gettimeofday(&g_ServerWaitTimeStart[iActive], NULL);
			hTempSrv = ConnectDataServer(g_pSrvAddr[iActive], g_SrvPorts[iActive], g_svrProtocol[iActive]);			
			if(hTempSrv > 0)
			{
				g_hServer[iActive] = hTempSrv;
				g_bServerState[iActive] = ACTIVE;
				ZERR("reconnect ok, ready to retry[%d].\n",*iRetryTimes);
				return 0;			
			}
			else
			{
				(*iRetryTimes)++;
				if(*iRetryTimes < MAX_RETRYTIMES)
				{
					ZERR("reconnect fail, ready to internal retry[%d].\n",*iRetryTimes);
					g_bServerState[iActive] = ACTIVE;
					goto INTERNAL_RETRY;
				}
				else
				{
					g_bServerState[iActive] = RETRY;
					ZERR("reconnect times expired[%d].\n", *iRetryTimes);
					return -1;
				}
			}
			
		}
		
	}
}


int SendDataServerResponseCommandToDBServer(int iActive, char* dataserver_response_command_buffer){
	int iSendBufferLen = 0;
	static char dbserver_command_buff[1024*4] = { '\0' };
	static char dataserver_response_buff[1024*4] = { '\0' };
	int dbserver_command_buflen = sizeof(dbserver_command_buff);
	
	if(!dataserver_response_command_buffer || (0>=strlen(dataserver_response_command_buffer))){
		ZERR("invalid data server response buffer.\n");
		return -1;
	}
	ZDBG("receive data server response: %s.\n", dataserver_response_command_buffer);
	cJSON* cjson = cJSON_Parse(dataserver_response_command_buffer);
	if(cjson == NULL){
		ZDBG("bad command.\n");
		return 0;
	}
	cJSON* pIsResult = cJSON_GetObjectItem(cjson, "dataResultWeb");
	cJSON* pDataServerResponse =  cJSON_CreateObject();
	if(pIsResult){
		ZDBG("The receive packet is the data server respose to local database server.\n");
		AddStringItem(pDataServerResponse, "command", "gpsdatarsp");
		cJSON_AddItemToObject(pDataServerResponse, "dataResultWeb", cJSON_Duplicate(pIsResult, 1));
	}else{
		ZERR("data server response command format invalid.\n");
		cJSON_Delete(cjson);
		cJSON_Delete(pDataServerResponse);
		return -2;
	}

	//标记服务器
	MarkServer(pDataServerResponse, iActive);
	char* pRecv = cJSON_Print(pDataServerResponse);
	memset(dbserver_command_buff, '\0', dbserver_command_buflen);
	iSendBufferLen = snprintf(dbserver_command_buff, dbserver_command_buflen, "%s%s", pRecv, SPLITER);
	free(pRecv);
	ZDBG("data server response command to local database server is(length=%d, bufflen=%d) %s.\n", 
		iSendBufferLen, dbserver_command_buflen, dbserver_command_buff);
	cJSON_Delete(cjson);
	cJSON_Delete(pDataServerResponse);
	
	int len = write(g_hLocalServer[0], dbserver_command_buff, iSendBufferLen);
	if(len > 0){
		ZDBG("Send data server response json to local database server length=%d.\n", len);

		// 修改机制，将云服务器上返回数据发送给DBServer，不需要等待DBServer返回 
		#if 0
		g_srvGPSBusinessPhase[iActive] = GPS_SENT_RESPONSE_TO_LOCALDB_SERVER;
		gettimeofday(&g_srvGPSBusinessPhaseTimeStart[iActive], NULL);
		#else
		g_srvGPSBusinessPhase[iActive] = GPS_FREE;
		#endif
		
		g_srvGPSBusinessPhaseWaitCount[iActive] = 0;
		ZDBG("g_srvGPSBusinessPhase[%d] = %d", iActive, g_srvGPSBusinessPhase[iActive]);
		return len;
	}else { //内部LocalSocket error
		ZERR("Send data server response json to local database server fail.\n");
		close(g_hLocalServer[0]);
		g_hLocalServer[0] = -1;
		g_bLocalServerState[0] = INACTIVE;
		g_srvGPSBusinessPhase[iActive] = GPS_INVALID;
		g_srvGPSBusinessPhaseWaitCount[iActive] = 0;
		return -1;
	}
	
	return 0;
}

int ParseDataServerResponseCommand(int iActive)
{
	static char db_command_buff[1024*4] = { '\0' };
	static char db_command_tmp_buff[1024*4] = { '\0' };
	static int db_command_offset = 0;
	static char dataserver_response_json_buf[1024] = { '\0' };
	static char pReadIP[64];
	int fd = g_hServer[iActive];
	int protocal = g_svrProtocol[iActive];
	int readPort = 0;
	int iRecvLen = 0;
	int db_command_buflen = sizeof(db_command_buff);
	
	memset(pReadIP, 0, sizeof(pReadIP));
	sprintf(pReadIP, "%s", g_pSrvAddr[iActive]);
	readPort = g_SrvPorts[iActive];
	if( (g_bLocalServerState[0]==INACTIVE)|| (g_hLocalServer[0]< 0)){
		ZERR("local socket is invalid.\n");
		return -1;
	}
	
	iRecvLen = MyRead(protocal, fd, db_command_buff + db_command_offset, db_command_buflen - db_command_offset, pReadIP, &readPort);
	if(0 >= iRecvLen){
		close(g_hServer[iActive]);
		g_hServer[iActive] = -1;
		g_bServerState[iActive] = INACTIVE;
		ZERR("read the data server: %s:%d fail.\n", g_pSrvAddr[iActive], g_SrvPorts[iActive]);
		g_srvGPSBusinessPhase[iActive] = GPS_INVALID;
		g_srvGPSBusinessPhaseWaitCount[iActive] = 0;
		g_bServerState[iActive] = INACTIVE;
		g_ServerWaitCount[iActive] = 0;
		gettimeofday(&g_ServerWaitTimeStart[iActive], NULL);
	}else{
		ZDBG("read length is %d, can used buffer length is %d.\n", iRecvLen, db_command_buflen - db_command_offset);
		*(db_command_buff + db_command_offset + iRecvLen) = '\0';
		ZDBG("receive data from data server is (already in buffer data length:%d, read new data length:%d):\n%s\n", db_command_offset, iRecvLen, db_command_buff);
		char* pOneJson = db_command_buff;
		char* pNextJson = NULL;
		if(strlen(pOneJson) == 0){
			ZDBG("data length is 0, current loop split json over\n");
			memset(db_command_buff, '\0', db_command_buflen);
			db_command_offset = 0;
			return 0;
		}
		do{
			pNextJson = strstr(pOneJson, "}");
			if(pNextJson == NULL){
				ZERR("can not find json end mark.\n");
				memset(db_command_buff, '\0', db_command_buflen);
				db_command_offset = 0;
				return 0;
			}else{
				memset(dataserver_response_json_buf, '\0', sizeof(dataserver_response_json_buf));
				int iCurlen = pNextJson - pOneJson + 1;
				if(iCurlen > sizeof(dataserver_response_json_buf)){
					ZDBG("current json len %d is longer than json buffer length %d, current one json:%s\n",
						iCurlen, sizeof(dataserver_response_json_buf), pOneJson);
					iCurlen = sizeof(dataserver_response_json_buf);
				}
				memcpy(dataserver_response_json_buf, pOneJson, iCurlen);
				ZDBG("data server response json buf(length=%d): %s\n", strlen(dataserver_response_json_buf), dataserver_response_json_buf);
				SendDataServerResponseCommandToDBServer(iActive, dataserver_response_json_buf);					
				pOneJson = pNextJson + strlen("}");
			}
		}while(1);
	
		return 1;
	}
	ZERR("Parse and execute data server response command fail.\n");
	return -2;
}



int ExecuteLocalDatabaseResponseCommand(char* gps_response_cmd_buffer)
{	
	static char gps_data_buffer[DATA_BUFFER_LEN] = { '\0' };
	int gps_data_buflen = 0;
	int i = 0;
	int send_result = 1;  // 1-send success, 0-send fail.
	
	if(!gps_response_cmd_buffer || (0>=strlen(gps_response_cmd_buffer))){
		ZERR("invalid command buffer.\n");
		return -1;
	}
	
	do{
		//ZDBG("gps response command buffer(length=%d): %s\n", strlen(gps_response_cmd_buffer), gps_response_cmd_buffer);
		cJSON* pJson = cJSON_Parse(gps_response_cmd_buffer);
		if(pJson){
			ZDBG("parse json command success.\n");
			//JSON解析成功，继续处理
			char* pIp = GetStringVal(pJson, "IP");

			if(!pIp){
				ZDBG("Distract IP Address fail, json buffer: %s.", gps_response_cmd_buffer);
				continue;
			}
			
			long iPort = GetIntVal(pJson, "PORT");
			ZDBG("received IP=%s, Port=%d.\n", pIp, iPort);
			long iServerIndex = GetServerIndex(pIp, iPort);
			ZDBG("received Server index is %d.", iServerIndex);
			if(iServerIndex < 0){
				ZERR("Data Server Index error, drop this Json.\n");
				cJSON_Delete(pJson);
				continue;						
			}

			ZDBG("Use ServerIndex to get data server, IP=%s, Port=%d, State=%d, Handle=%d.\n",
				g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex], g_bServerState[iServerIndex], g_hServer[iServerIndex]);
			if(g_bServerState[iServerIndex]!=ACTIVE){
				ZERR("this server is not active, drop this Json.\n");
				cJSON_Delete(pJson);
				continue;
			}

			cJSON* pJsonData = cJSON_GetObjectItem(pJson, "data");
			//char* pData = cJSON_Print(pJsonData);
			char* pData = cJSON_PrintUnformatted(pJsonData);
			ZDBG("Extract data:%s.\n", pData);
				
			cJSON* cmd = cJSON_GetObjectItem(pJsonData, "command");
			if(cmd){
				ZDBG("local database server response command=%s.\n", cmd->valuestring);
			}
			
			if(cmd && !strcmp(cmd->valuestring, "gpsdatarsp")){
				if(g_srvGPSBusinessPhase[iServerIndex] != GPS_SENT_RESPONSE_TO_LOCALDB_SERVER){
					ZERR("g_srvGPSBusinessPhase[%d]=%d, g_srvGPSBusinessPhase[%d] != GPS_SENT_RESPONSE_TO_LOCALDB_SERVER, this server phase is invalid, drop this Json.\n", 
						iServerIndex, g_srvGPSBusinessPhase[iServerIndex], iServerIndex);
					g_srvGPSBusinessPhase[iServerIndex] = GPS_FREE;
					g_srvGPSBusinessPhaseWaitCount[iServerIndex] = 0;
				}else{
					cJSON* pSuccess = cJSON_GetObjectItem(pJsonData, "success");
					if(1==pSuccess->valueint){
						ZDBG("gpsdatarsp success.\n");
						// 此处置标志位表明gpsdatarsp成功
						g_srvGPSBusinessPhase[iServerIndex] = GPS_FREE;
						g_srvGPSBusinessPhaseWaitCount[iServerIndex] = 0;
					}else{
						ZERR("gpsdatarsp fail.\n");
						// 此处置标志位表明gpsdatarsp失败
						g_srvGPSBusinessPhase[iServerIndex] = GPS_FREE;
						g_srvGPSBusinessPhaseWaitCount[iServerIndex] = 0;
					}
				}
			}else{
				// 修改机制，DBServer主动上报数据，故去掉查询动作
				bzero(gps_data_buffer, DATA_BUFFER_LEN);
				gps_data_buflen = sprintf(gps_data_buffer, "%s\r\n", pData);
				ZDBG("gps data to send:%s.\n", gps_data_buffer);
				for(i = 0; i < gps_data_buflen/MAX_SIZE_PER_SEND_FRAME; i++){
					if(MAX_SIZE_PER_SEND_FRAME!=MyWrite(g_svrProtocol[iServerIndex], g_hServer[iServerIndex], 
						gps_data_buffer + i * MAX_SIZE_PER_SEND_FRAME, 
						MAX_SIZE_PER_SEND_FRAME, 
						g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex])){
						send_result = 0;
						break;
					}
				}
				if( (1==send_result) && (gps_data_buflen%MAX_SIZE_PER_SEND_FRAME > 0) ){
					if( (gps_data_buflen%MAX_SIZE_PER_SEND_FRAME)!=MyWrite(g_svrProtocol[iServerIndex], g_hServer[iServerIndex], 
						gps_data_buffer + gps_data_buflen -(gps_data_buflen%MAX_SIZE_PER_SEND_FRAME), 
						gps_data_buflen%MAX_SIZE_PER_SEND_FRAME, 
						g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex]) ){
						send_result = 0;
					}
				}
				
				//if(MyWrite(g_svrProtocol[iServerIndex], g_hServer[iServerIndex], pData, strlen(pData), g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex]) > 0){
				//if(MyWrite(g_svrProtocol[iServerIndex], g_hServer[iServerIndex], gps_data_buffer, strlen(gps_data_buffer), g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex]) > 0){
				//if(MyWrite(g_svrProtocol[iServerIndex], g_hServer[iServerIndex], gps_data_buffer, gps_data_buflen, g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex]) > 0){
				if(1==send_result){
					free(pData);
					g_srvGPSBusinessPhase[iServerIndex] = GPS_SENT_DATA_TO_DATA_SERVER;
					gettimeofday(&g_srvGPSBusinessPhaseTimeStart[iServerIndex], NULL);
					g_srvGPSBusinessPhaseWaitCount[iServerIndex] = 0;
					ZDBG("===send data to No:%d handle %d, %s:%d end===\n", 
						iServerIndex, g_hServer[iServerIndex], g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex]);
					ZDBG("Send json packet to Data Server: %s:%d success.\n", g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex]);

					#if 0
					/*不需要收取云服务器回执*/
					if(g_srvNeedRespose[iServerIndex] < DATA_SERVER_RESPONSE_CUT_POINT){
						g_srvGPSBusinessPhase[iServerIndex] = GPS_FREE;
						g_srvGPSBusinessPhaseWaitCount[iServerIndex] = 0;
					}
					#endif
					/* 修改实现机制，改成发送数据后不需要等待回执的模式*/
					g_srvGPSBusinessPhase[iServerIndex] = GPS_FREE;
					g_srvGPSBusinessPhaseWaitCount[iServerIndex] = 0;
					
					ZDBG("g_srvGPSBusinessPhase[%d]=%d\n", iServerIndex, g_srvGPSBusinessPhase[iServerIndex]);						
				}else{
					free(pData);
					ZERR("write to %s:%d failed.\n", g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex]);
					close(g_hServer[iServerIndex]);
					g_hServer[iServerIndex] = -1;
					g_bServerState[iServerIndex] = INACTIVE;
					g_srvGPSBusinessPhase[iServerIndex] = GPS_INVALID;
					g_srvGPSBusinessPhaseWaitCount[iServerIndex] = 0;
					g_ServerWaitCount[iServerIndex] = 0;
					gettimeofday(&g_ServerWaitTimeStart[iServerIndex], NULL);
				}				
			}
			cJSON_Delete(pJson);			
		}else{
			ZERR("parse jaon command fail.\n");
		}
	}while(0/*has_more_cmd*/);

	return 1;
}


int ParseLocalDatabaseResponseCommand()
{
	static char gps_response_json_buf[DATA_BUFFER_LEN] = { '\0' };
	static char gps_response_cmd_buff[1024*128] = { '\0' };
	static char gps_response_cmd_tmp_buffer[1024*128] = { '\0' };
	static int gps_response_cmd_offset = 0;
	int gps_response_cmd_buflen = sizeof(gps_response_cmd_buff);
	/*memset(gps_response_cmd_buff, 0, gps_response_cmd_buflen); */
	bzero(gps_response_cmd_buff, gps_response_cmd_buflen);
	int iTotalLen = 0;
	int i = 0;
	
	do{
		ZDBG("buffer offset is %d, continue to read data.\n", gps_response_cmd_offset);
		if(gps_response_cmd_offset){
			ZDBG("data already in buffer:%s\n", gps_response_cmd_buff);
		}
		if(gps_response_cmd_offset >= gps_response_cmd_buflen){
			ZERR("dbserver buff is full.\n%s\n", gps_response_cmd_buff);
			gps_response_cmd_offset = 0;
			memset(gps_response_cmd_buff, 0, gps_response_cmd_buflen);
			break;
		}
		
		iTotalLen = read(g_hLocalServer[0], gps_response_cmd_buff + gps_response_cmd_offset, gps_response_cmd_buflen - gps_response_cmd_offset);
		ZDBG("recv data length=%d.\n", iTotalLen);
		if(iTotalLen > 0){
			ZDBG("read length is %d, can used buffer length is %d.\n", iTotalLen, gps_response_cmd_buflen - gps_response_cmd_offset);
			*(gps_response_cmd_buff + gps_response_cmd_offset + iTotalLen) = '\0';
			ZDBG("receive data from local database server is (already in buffer data length:%d, read new data length:%d):\n%s\n", 
				gps_response_cmd_offset, iTotalLen, gps_response_cmd_buff);
			char* pOneJson = gps_response_cmd_buff;
			char* pNextJson = NULL;
			if(strlen(pOneJson) == 0){
				ZDBG("data length is 0, current loop split json over.\n");
				memset(gps_response_cmd_buff, 0, gps_response_cmd_buflen);
				gps_response_cmd_offset = 0;
				break;
			}
			
			do{
				pNextJson = strstr(pOneJson, SPLITER);
				if(pNextJson == NULL){
					ZERR("can not find json end mark.\n");
					bzero(gps_response_cmd_tmp_buffer, sizeof(gps_response_cmd_tmp_buffer));
					sprintf(gps_response_cmd_tmp_buffer, "%s", pOneJson);
					bzero(gps_response_cmd_buff, gps_response_cmd_buflen);
					sprintf(gps_response_cmd_buff, "%s", gps_response_cmd_tmp_buffer);
					gps_response_cmd_offset = strlen(gps_response_cmd_buff);

					ZERR("the left json buffer(length=%d): %s.\n", gps_response_cmd_offset, gps_response_cmd_buff);
					
					break;
				}else{
					memset(gps_response_json_buf, 0, sizeof(gps_response_json_buf));
					int iCurlen = pNextJson - pOneJson;
					if(iCurlen > sizeof(gps_response_json_buf) + 1){
						ZDBG("current json len %d is longer than json buffer length %d, current one json:%s\n", 
							iCurlen, sizeof(gps_response_json_buf), pOneJson);
						iCurlen = sizeof(gps_response_json_buf) -1;
					}
					memcpy(gps_response_json_buf, pOneJson, iCurlen);
					ExecuteLocalDatabaseResponseCommand(gps_response_json_buf);
					pOneJson = pNextJson + strlen(SPLITER);
				}
			}while(1);
		}
	}while(0);

	return 0;
}



int ParseLocalDatabaseServerCommand()
{
	return ParseLocalDatabaseResponseCommand();
}


int QueryGPSFromDatabase(int iServerIndex)
{
	static char getgps_command_buff[1024*8];
	int getgps_command_buflen = sizeof(getgps_command_buff);
	memset(getgps_command_buff, 0, getgps_command_buflen);
	int iTotalLen = 0;
	int i = 0;
	ZDBG("The server %s:%d try to read database, interval is %d.\n", g_pSrvAddr[iServerIndex], g_SrvPorts[iServerIndex], g_srvNeedRespose[iServerIndex]);
	cJSON * root = CreateCommandHead("getgps");
	MarkServer(root, iServerIndex);
	AddIntItem(root, "needRsp", g_srvNeedRespose[iServerIndex]);
	char *pprint = cJSON_Print(root);
	int iLen = 0;
	if(pprint){
		iLen = snprintf(getgps_command_buff, getgps_command_buflen, "%s%s", pprint, SPLITER);
		free(pprint);
		pprint = NULL;
	}
	cJSON_Delete(root);
	ZDBG("send json command to dbserver(length:%d):\n%s.\n", iLen, getgps_command_buff);
	if(iLen > 0 && iLen <= getgps_command_buflen){
		iLen = write(g_hLocalServer[0], getgps_command_buff, iLen);
		if(0 >= iLen){
			ZERR("send internal json command failed.\n");
			close(g_hLocalServer[0]);
			g_hLocalServer[0] = -1;
			g_bLocalServerState[0] = INACTIVE;
			return -1;	
		}
	}else{
		ZERR("Make process json command failed.\n");
		return -1;
	}
	ZDBG("send command length is %d.\n",iLen);
	return iLen;
}

int SelfIntroductionToLocalServer(int localSock)
{
	static char sefl_intro_buff[1024] = { '\0' };
	int buflen = sizeof(sefl_intro_buff);
	memset(sefl_intro_buff, '\0', buflen);
	int iTotalLen = 0;
	int i = 0;
	do{
		cJSON * root =  cJSON_CreateObject();
		cJSON_AddItemToObject(root, "command", cJSON_CreateString("mynameiscommandproxy")); //根节点下添加
		cJSON_AddItemToObject(root, "time", cJSON_CreateNumber(time(NULL))); //根节点下添加

		cJSON * jIPArray =  cJSON_CreateArray(); 
		cJSON * jPortArray =  cJSON_CreateArray(); 
		cJSON * jNeedResponseArray =  cJSON_CreateArray(); 
		for(i = 0; i < g_srvCount; i++){
			cJSON_AddItemToArray(jIPArray, cJSON_CreateString(g_pSrvAddr[i]));
			cJSON_AddItemToArray(jPortArray, cJSON_CreateNumber(g_SrvPorts[i]));
			cJSON_AddItemToArray(jNeedResponseArray, cJSON_CreateNumber(g_srvNeedRespose[i]));
		}
		
		if(jIPArray){
			cJSON_AddItemToObject(root, "IPS", jIPArray);
			cJSON_AddItemToObject(root, "PORTS", jPortArray);
			cJSON_AddItemToObject(root, "NEEDRESPONSES", jNeedResponseArray);
		}
		char *pprint = cJSON_Print(root);
		int iLen = 0;
		if(pprint){
			iLen = snprintf(sefl_intro_buff, buflen, "%s%s", pprint, SPLITER);
			free(pprint);
		}
		cJSON_Delete(root);
		ZDBG("send json command(length:%d)=%s\n", iLen, sefl_intro_buff);

		if(iLen > 0){
			iLen = write(localSock, sefl_intro_buff, iLen);
			if(iLen <= 0){
				ZDBG("send internal json command failed.\n");
				return -1;				
			}
		}else{
			ZERR("send internal json command failed.\n");
			return -1;
		}
		
		if(IsHandelReadable(localSock, 3000000) < 0){
			ZDBG("local socket is not readable.\n");
			break;
		}
		
		iTotalLen = read(localSock, sefl_intro_buff, buflen);
		ZDBG("recv data length=%d\n", iTotalLen);
		if(iTotalLen >= 0){
			if(iTotalLen == 0){
				ZDBG("Data receive complete, recv data len=%d\n", iTotalLen);
				break;
			}else{
				ZDBG("receive data is (length:%d)\n%s\n", iTotalLen, sefl_intro_buff);
				return 1;//收到即OK
			}
		}else{
			ZERR("command receive from server error...\n");
			break;
		}
	}while(0);
	return -1;
}
//const char* LOCAL_SERVER[]={"/opt/bin/UNIX.domain"};
const char* LOCAL_SERVER[]={"/opt/bin/CommandProxy.domain"};
//#define UNIX_DOMAIN2 "/opt/bin/CommandProxy.domain"
void usage() {
	printf("CommandProxy format: \n");
	printf("CommandProxy 192.168.1.121 8888 TCP|UDP 8000 192.168.1.221 9999 TCP|UDP 8000\n");
	printf("	192.168.1.121: Server IP Address;\n");
	printf("	8888: Server IP Port\n");
	printf("	TCP|UDP: TCP or UDP protocal\n");
	printf("	8000: recv time out interval\n");
	printf("	192.168.1.221: Server IP Address;\n");
	printf("	9999: Server IP Port,\n");
	printf("	TCP|UDP: TCP or UDP protocal\n");
	printf("	8000: recv time out interval\n");
	printf("	etc.\n");
	return;
}
int ConnectToDBServerProcess()
{
	int i = 0;
	int hTempSrv = -1;
	int iActiveLocalServer = 0;
	g_hLocalServer[0] = -1;
	g_bLocalServerState[0] = INACTIVE;
	for(i = 0; i < NUMBER_LOCALSERVER; i++)
	{
		hTempSrv = ConnectLocalServer(LOCAL_SERVER[i]);
		if(hTempSrv <= 0){
			ZDBG("Can not connect to local server: %s.\n", LOCAL_SERVER[i]);
			usleep(100000);
		}else{
			gettimeofday(&g_DBserverLatestCommnicationTime, NULL);
			if(SelfIntroductionToLocalServer(hTempSrv) > 0){
				g_hLocalServer[iActiveLocalServer] = hTempSrv;
				g_bLocalServerState[iActiveLocalServer] = ACTIVE;
				ZDBG("Local server:%s, handle=%d.\n", LOCAL_SERVER[i], g_hLocalServer[iActiveLocalServer]);
				iActiveLocalServer++;
				ZDBG("Local server handle=%d.\n", g_hLocalServer[i]);
			}else{
				ZDBG("Self intruduction to local database server fail, close the socket and reconnect.\n");
				close(hTempSrv);
				usleep(100000);
			}
		}
	}
	return iActiveLocalServer;
}

long get_time_span(struct timeval time_start, struct timeval time_end){
	long time_span = (time_end.tv_sec - time_start.tv_sec)*1000000 + (time_end.tv_usec - time_start.tv_usec);
	return (time_span / 1000000);
}

int main(int argc, char *argv[])
{ 
	InitialLog("CommandProxy.txt");
	signal(SIGPIPE, SIG_IGN);								//忽略SIGPIPE信号
	const char* build_time = __TIME__ " "__DATE__;	//编译日期
	ZDBG("CommandProxy Build@%s\n", build_time);

	if(argc < 3){
		usage();
		return 1;
	}
	if(argc % 3 != 1){
		usage();
		return 1;
	}
	
	int i = 0;
	int hTempSrv = -1;
	int maxFd = -1;
	int select_result = -1;
	
	for(i = 0; i < MAX_SERVER; i++){
		g_hServer[i] = -1;
		gettimeofday(&g_srvGPSBusinessPhaseTimeStart[i], NULL);
		gettimeofday(&g_ServerWaitTimeStart[i], NULL);
	}
	memset(g_hLocalServer, 0, sizeof(g_hLocalServer));
	
	for(i = 0; i < MAX_SERVER; i++){
		memset(g_pSrvAddr[i], '\0', 32);
		g_SrvPorts[i] = 0;
	}
	
	g_srvCount = 0;
	for(i = 1; i < argc; i++) {
		sprintf(g_pSrvAddr[g_srvCount], "%s", argv[i++]);
		g_SrvPorts[g_srvCount] = atoi(argv[i++]);
		if(!strcmp(argv[i++], "UDP")){
			g_svrProtocol[g_srvCount] = SOCK_DGRAM;
		}else{
			g_svrProtocol[g_srvCount] = SOCK_STREAM;
		}
		g_srvNeedRespose[g_srvCount] = atoi(argv[i]);
		ZDBG("SOCKET %d: %s:%d protocal %s(%d) interval is %d.\n", 
			g_srvCount, g_pSrvAddr[g_srvCount], g_SrvPorts[g_srvCount], argv[i], g_svrProtocol[g_srvCount], g_srvNeedRespose[g_srvCount]);
		g_srvCount++;
	}

	ZDBG("server count config value is %d.\n", g_srvCount);
	if(g_srvCount > MAX_SERVER){
		ZDBG("Too many server count config value is %d > %d.\n", g_srvCount, MAX_SERVER);
		return -1;
	}
	
	for(i = 0; i < g_srvCount; i++){
		ZDBG("try to connect the %dth server: %s:%d.\n", i, g_pSrvAddr[i], g_SrvPorts[i]);
		hTempSrv = ConnectDataServer(g_pSrvAddr[i], g_SrvPorts[i], g_svrProtocol[i]);
		if(hTempSrv <= 0){
			ZDBG("Can not connect server: %s:%d,%s\n", g_pSrvAddr[i], g_SrvPorts[i], strerror(errno));
			g_hServer[i] = -1;
			g_bServerState[i] = INACTIVE;
			g_srvGPSBusinessPhase[i] = GPS_INVALID;
			g_srvGPSBusinessPhaseWaitCount[i] = 0;
			gettimeofday(&g_ServerWaitTimeStart[i], NULL);
			g_ServerWaitCount[i] = 0;
		}else{
			ZDBG("Server socket handle is %d at %s:%d\n", hTempSrv, g_pSrvAddr[i], g_SrvPorts[i]);
			g_hServer[i] = hTempSrv;
			g_srvGPSBusinessPhase[i] = GPS_FREE;
			g_srvGPSBusinessPhaseWaitCount[i] = 0;
			g_bServerState[i] = ACTIVE;
		}		
	}
	//多个进程间通讯，暂时先写死使用一个
	int iActiveLocalServer = 0;
	iActiveLocalServer = ConnectToDBServerProcess();

	fd_set rd;
	struct timeval tv_dbserver;
	tv_dbserver.tv_sec = 0;
	tv_dbserver.tv_usec = 20000;
	struct timeval tv_data_server;
	tv_data_server.tv_sec = 0;
	tv_data_server.tv_usec = 20000;
	int iNeedReturn = 0;
	int bUdpIsSet = 0;
	int iActiveSockNum = 0;

	long time_use = 0;
	struct timeval cycle_start;
	struct timeval cycle_end;
	struct timeval time_current;

	Soft_Led_Init();
	
	while(1){
		Led_Off();
		gettimeofday(&cycle_start, NULL);

		gettimeofday(&time_current, NULL);
		
		if(g_hLocalServer[0] > 0){		
			if(g_bLocalServerState[0]!=ACTIVE){
				close(g_hLocalServer[0]);
				g_hLocalServer[0] = -1;
				ZDBG("reconnect local server.\n");
				ConnectToDBServerProcess();
			}				
		}else{
			ZERR("DBServer need re-connect.\n");
			ConnectToDBServerProcess();
		}
		
		if( (g_hLocalServer[0] <= 0) || (g_bLocalServerState[0] != ACTIVE)){
			usleep(500000);
			ZDBG("DBServer process socket error.\n");
			continue;
		}

		if( (g_hLocalServer[0] > 0) && (g_bLocalServerState[0] == ACTIVE) 
			&& (get_time_span(g_DBserverLatestCommnicationTime, time_current) > DBSERVER_NO_COMM_IN_SECOND) ){
			close(g_hLocalServer[0]);
			g_hLocalServer[0] = -1;
			g_bLocalServerState[0] = INACTIVE;
			usleep(500000);
			ZDBG("DBServer process communicate timeout error.\n");
			gettimeofday(&g_DBserverLatestCommnicationTime, NULL);
			continue;
		}
		
		#if 0
		iActiveSockNum = 0;
		bUdpIsSet = 0;
		#endif
		
		FD_ZERO(&rd);
		gettimeofday(&time_current, NULL);
		// 每次循环
		for(i = 0; i < g_srvCount; i++) {
			if( (g_bServerState[i] != ACTIVE) || (g_hServer[i] < 0)){
				//if(g_ServerWaitCount[i] >= DATA_SERVER_MAX_WAIT_COUNT){
				//if(g_ServerWaitCount[i] >= DATA_SERVER_MAX_WAIT_TIME_INSECONDS){
				if(get_time_span(g_ServerWaitTimeStart[i], time_current) > DATA_SERVER_MAX_WAIT_TIME_INSECONDS){
					g_ServerWaitCount[i] = 0;
					gettimeofday(&g_ServerWaitTimeStart[i], NULL);
					hTempSrv = ConnectDataServer(g_pSrvAddr[i], g_SrvPorts[i], g_svrProtocol[i]);
					if(hTempSrv <= 0) {
						ZDBG("Can not connect server: %s:%d, %s\n", g_pSrvAddr[i], g_SrvPorts[i], strerror(errno));
						g_hServer[i] = -1;
						g_srvGPSBusinessPhase[i] = GPS_INVALID;
						g_srvGPSBusinessPhaseWaitCount[i] = 0;
						g_bServerState[i] = INACTIVE;
					}else{
						ZDBG("Server socket handle is %d at %s:%d\n", hTempSrv, g_pSrvAddr[i], g_SrvPorts[i]);
						g_hServer[i] = hTempSrv;	
						g_srvGPSBusinessPhase[i] = GPS_FREE;
						g_srvGPSBusinessPhaseWaitCount[i] = 0;
						g_bServerState[i] = ACTIVE;
					}
				}else{
					g_ServerWaitCount[i]++;
				}				
			}
		}
		
		for(i = 0; i < g_srvCount; i++) {
			if( (g_bServerState[i] == ACTIVE) && (0!=SocketConnected(g_hServer[i]))){
				ZDBG("socket %d disconnected unexpectively, %s:%d.\n", g_hServer[i], g_pSrvAddr[i], g_SrvPorts[i]);
				close(g_hServer[i]);				
				g_hServer[i] = -1;
				g_srvGPSBusinessPhase[i] = GPS_INVALID;
				g_srvGPSBusinessPhaseWaitCount[i] = 0;
				g_bServerState[i] = INACTIVE;
				g_ServerWaitCount[i] = 0;
				gettimeofday(&g_ServerWaitTimeStart[i], NULL);
			}	
		}
		
		maxFd = 0;
		if( (g_bLocalServerState[0] > 0)&& (g_bLocalServerState[0] == ACTIVE)){
			FD_ZERO(&rd);
			FD_SET(g_hLocalServer[0], &rd);
			if(maxFd < g_hLocalServer[0]){
				maxFd = g_hLocalServer[0];
			}

			ZDBG("select dbserver process localsocket.\n");
			select_result = select(maxFd + 1, &rd, NULL, NULL, &tv_dbserver);
			if( select_result > 0){
				// 收到DBServer的数据，发送给数据服务器
				if( (g_hLocalServer[0] > 0) && (g_bLocalServerState[0] == ACTIVE) && (FD_ISSET(g_hLocalServer[0], &rd))) {
					gettimeofday(&g_DBserverLatestCommnicationTime, NULL);
					ZDBG("Receive and parse data from Local Database Server.\n");		//收到包
					ParseLocalDatabaseServerCommand();
				}
			}else if(select_result==0){
				ZDBG("select local DBServer timeout.\n");
			}else{
				//对TCP通讯异常的Socket进行重连,需要验证是否在这里处理
				ZERR("select fail, error: %s.\n", strerror(errno));
			}
		}
		
		maxFd = 0;
		iActiveSockNum = 0;
		bUdpIsSet = 0;
		gettimeofday(&time_current, NULL);
		FD_ZERO(&rd);
		for(i = 0; i < g_srvCount; ++i){
			if((g_hServer[i] > 0)&&(g_bServerState[i]==ACTIVE)){
				#if 0
				if(g_srvGPSBusinessPhase[i] > GPS_FREE) {
					g_srvGPSBusinessPhaseWaitCount[i]++;
				}
				
				if(g_srvGPSBusinessPhaseWaitCount[i] > GPS_BUSINESS_PHASE_MAX_WAIT_COUNT){
					ZDBG("g_srvGPSBusinessPhase[%d]=%d, timeout, reset phase.", g_srvGPSBusinessPhase[i], i);
					g_srvGPSBusinessPhase[i] = GPS_FREE;
					g_srvGPSBusinessPhaseWaitCount[i] = 0;
				}
				#endif
				if( (g_srvGPSBusinessPhase[i] > GPS_FREE) 
					&& (get_time_span(g_srvGPSBusinessPhaseTimeStart[i], time_current) > GPS_BUSINESS_PHASE_MAX_WAIT_TIME_IN_SECONDS) ){
					ZDBG("g_srvGPSBusinessPhase[%d]=%d, timeout, reset phase.", g_srvGPSBusinessPhase[i], i);
					g_srvGPSBusinessPhase[i] = GPS_FREE;
					g_srvGPSBusinessPhaseWaitCount[i] = 0;
				}

				
				if(maxFd < g_hServer[i]){
					maxFd = g_hServer[i];
				}
				
				if(g_svrProtocol[i] == SOCK_STREAM){
					FD_SET(g_hServer[i], &rd);
					++iActiveSockNum;
				}else{
					if(g_svrProtocol[i] == SOCK_DGRAM && bUdpIsSet == 0){
						FD_SET(g_hServer[i], &rd);
						bUdpIsSet = 1;
						++iActiveSockNum;
					}
				}
			}
		}		
		ZDBG("select data server count is %d.\n", iActiveSockNum);
		if(iActiveSockNum > 0){
			select_result = select(maxFd + 1, &rd, NULL, NULL, &tv_data_server);
			if( select_result > 0){
				// 收数据服务器返回的指令，解析后转发给DBServer
				ZDBG("recv data server command Start->:\n");
				for(i = 0; i < g_srvCount; i++){
					if((g_hServer[i] > 0) && (g_bServerState[i]==ACTIVE) && (FD_ISSET(g_hServer[i], &rd))){
						Led_On();						
						ParseDataServerResponseCommand(i);
					}				
				}
				ZDBG("recv data server command End.\n");
			}else if(select_result==0){
				ZDBG("select data server timeout.\n");
			}else{
				//对TCP通讯异常的Socket进行重连,需要验证是否在这里处理
				ZERR("select fail, error:%s.\n", strerror(errno));
			}
		}

		// 修改数据发送机制，改为由DBServer进程主动发送数据，
		// 故此处不需要发送查询命令
		#if 0                         
		// 组数据申请包，发送给DBServer
		for(i = 0; i < g_srvCount; ++i){
			if((g_hServer[i] > 0) && (g_bServerState[i]==ACTIVE) && (g_srvGPSBusinessPhase[i]==GPS_FREE)){
				ZDBG("Data Server %s:%d, handle: %d, state=%d, GPSBusinessPhase=%d:\n",
					g_pSrvAddr[i], g_SrvPorts[i], g_hServer[i], g_bServerState[i], g_srvGPSBusinessPhase[i]);
				if(QueryGPSFromDatabase(i) > 0){						
					ZDBG("send getgps command to DBServer process success.\n");
					g_srvGPSBusinessPhase[i] = GPS_SENT_QUERY_CMD_TO_LOCALDB_SERVER;
					gettimeofday(&g_srvGPSBusinessPhaseTimeStart[i], NULL);
					g_srvGPSBusinessPhaseWaitCount[i] = 0;
					ZDBG("g_srvGPSBusinessPhase[%d]=%d.\n", i, g_srvGPSBusinessPhase[i]);
				}else{
					ZDBG("send getgps command to DBServer process fail.\n");
				}
			}
		}
		#endif
		
		gettimeofday(&cycle_end, NULL);
		time_use = (cycle_end.tv_sec - cycle_start.tv_sec)*1000000 + (cycle_end.tv_usec - cycle_start.tv_usec); //微秒
		ZDBG("time_use = %07ld\n", time_use);
		if(time_use < 300000){
			usleep(300000 - time_use);
		}
	}
}
