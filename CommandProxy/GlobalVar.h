#ifndef GLOBALVAR_H
#define GLOBALVAR_H

#include <sys/time.h>
#include <stdint.h>

/*重新尝试连接的等待时间*/
#define DATA_SERVER_MAX_WAIT_TIME_INSECONDS 3

#define GPS_BUSINESS_PHASE_MAX_WAIT_TIME_IN_SECONDS 20
#define MAX_SERVER 8

#define MAX_SIZE_PER_SEND_FRAME 6510

/* 此值表示是否需要从云服务器获取反馈包 */
#define DATA_SERVER_RESPONSE_CUT_POINT 1000
#define NUMBER_LOCALSERVER 1

typedef enum{
	INACTIVE = 0,
	ACTIVE,
	RETRY
} WORKTYPE;

#define GPS_INVALID -1
#define GPS_FREE 0
#define GPS_SENT_QUERY_CMD_TO_LOCALDB_SERVER 1
#define GPS_SENT_DATA_TO_DATA_SERVER 2
#define GPS_SENT_RESPONSE_TO_LOCALDB_SERVER 3

#define DBSERVER_NO_COMM_IN_SECOND 300

extern int g_hServer[MAX_SERVER];
extern int g_bServerState[MAX_SERVER];
extern struct timeval g_ServerWaitTimeStart[MAX_SERVER];
extern int g_ServerWaitCount[MAX_SERVER];

extern int g_hLocalServer[MAX_SERVER];
extern int g_bLocalServerState[MAX_SERVER];

extern char g_pSrvAddr[MAX_SERVER][32];
extern uint16_t g_SrvPorts[MAX_SERVER];
extern int g_svrProtocol[MAX_SERVER];
extern int g_srvNeedRespose[MAX_SERVER];
extern int g_srvGPSBusinessPhase[MAX_SERVER];
extern int g_srvGPSBusinessPhaseWaitCount[MAX_SERVER];
extern struct timeval g_srvGPSBusinessPhaseTimeStart[MAX_SERVER];
extern int g_srvCount;	
extern struct timeval g_DBserverLatestCommnicationTime;
#endif