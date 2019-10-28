#include "GlobalVar.h"

int g_hServer[MAX_SERVER] ={0,0,0,0,0,0,0,0};
int g_bServerState[MAX_SERVER] ={0,0,0,0,0,0,0,0};
struct timeval g_ServerWaitTimeStart[MAX_SERVER];
int g_ServerWaitCount[MAX_SERVER] ={0,0,0,0,0,0,0,0};


int g_hLocalServer[MAX_SERVER] ={0,0,0,0,0,0,0,0};
int g_bLocalServerState[MAX_SERVER] ={0,0,0,0,0,0,0,0};

char g_pSrvAddr[MAX_SERVER][32]={"", "", "", "", "", "", "", ""};
uint16_t g_SrvPorts[MAX_SERVER] ={0,0,0,0,0,0,0,0};
int g_svrProtocol[MAX_SERVER] = {0, 0, 0, 0, 0, 0, 0, 0};
int g_srvNeedRespose[MAX_SERVER] = {0, 0, 0, 0, 0, 0, 0, 0};


int g_srvGPSBusinessPhase[MAX_SERVER] = {GPS_FREE, GPS_FREE, GPS_FREE, GPS_FREE, GPS_FREE, GPS_FREE, GPS_FREE, GPS_FREE};
int g_srvGPSBusinessPhaseWaitCount[MAX_SERVER] = {0, 0, 0, 0, 0, 0, 0, 0};
struct timeval g_srvGPSBusinessPhaseTimeStart[MAX_SERVER];

int g_srvCount = 0;
struct timeval g_DBserverLatestCommnicationTime;

