#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "Log.h"
#include <sys/syscall.h>
#include <errno.h>
/*log输出到文件的名称*/
/*#define DEBUG_FILE_ "log.txt"*/
#define MAX_STRING_LEN      (1024*128 + 1)

//#define SIXTY_FOUR_MB (64*1024*1024)
#define SIXTY_FOUR_MB (16*1024*1024)


//Level类别
#define IC_NO_LOG_LEVEL         0
#define IC_DEBUG_LEVEL          1
#define IC_INFO_LEVEL           2
#define IC_WARNING_LEVEL        3
#define IC_ERROR_LEVEL          4

int  LogLevel[5] = { IC_NO_LOG_LEVEL, IC_DEBUG_LEVEL, IC_INFO_LEVEL, IC_WARNING_LEVEL, IC_ERROR_LEVEL };

//Level的名称
char ICLevelName[5][10] = { "NOLOG", "DEBUG", "INFO", "WARNING", "ERROR" };

char LogFileName[256] = {'\0'};
static char date[256] = {'\0'};

void InitialLog(char *szLogFile)
{
	snprintf(LogFileName, sizeof(LogFileName), "/media/mmcblk0p5/%s", szLogFile);
	printf("log file path: %s.\n", LogFileName);
}

static int C_Error_GetCurTime(char* strTime)
{
    //struct tm*      tmTime = NULL;
    size_t          timeLen = 0;
    //time_t          tTime = 0;

 /*   tTime = time(NULL);
    tmTime = localtime(&tTime);
    //timeLen = strftime(strTime, 33, "%Y(Y)%m(M)%d(D)%H(H)%M(M)%S(S)", tmTime);
    timeLen = strftime(strTime, 33, "%Y.%m.%d %H:%M:%S", tmTime);*/
	    time_t now;
		struct tm ptm;
	    time(&now);                   // Gets the system time
		localtime_r(&now, &ptm);
		struct timeval tv;
		struct timezone tz;
		struct tm       *p;
		gettimeofday(&tv, &tz);
		p = localtime(&tv.tv_sec);
        //strftime(date, sizeof (date), "%F %T", &ptm);
		strftime(date, sizeof (date), "%F", &ptm);
        sprintf(strTime, "[%s %02d:%02d:%02d.%06ld]:",date,p->tm_hour,p->tm_min,p->tm_sec, tv.tv_usec);

    return timeLen;
}

/*
static int C_Error_OpenFile(int* pf)
{
    char    fileName[1024];

    memset(fileName, 0, sizeof(fileName));
//#ifdef WIN32
    //sprintf(fileName, "e:\\eavil\\code\\vswork\\TestLOG\\%s", LJQ_DEBUG_FILE_);
	sprintf(fileName, "%s", DEBUG_FILE_);
//#else
//  sprintf(fileName, "%s/log/%s", getenv("HOME"), LJQ_DEBUG_FILE_);
//#endif

    *pf = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (*pf < 0)
    {
        return -1;
    }

    return 0;
}
*/

static int C_Error_OpenFile(int* pf, char* log_file_path)
{
    char    fileName[1024];

    memset(fileName, 0, sizeof(fileName));
//#ifdef WIN32
    //sprintf(fileName, "e:\\eavil\\code\\vswork\\TestLOG\\%s", LJQ_DEBUG_FILE_);
	//sprintf(fileName, "%s", DEBUG_FILE_);
	sprintf(fileName, "%s", log_file_path);
//#else
//  sprintf(fileName, "%s/log/%s", getenv("HOME"), LJQ_DEBUG_FILE_);
//#endif

    *pf = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (*pf < 0)
    {
    	printf("open %s failed.\n", fileName);
        return -1;
    }

    return 0;
}

static long get_file_size(char* file_name)
{
	long length = 0;
	FILE *fp = NULL;
 
	fp = fopen(file_name, "rb");
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
	}
	else
	{
		printf("get_file_size %s failed, error=%s.\n", file_name, strerror(errno));
		return -1;
	}
 
	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
 
	return length;
}


/*
static void C_Error_Core(const char *file, int line, int level, int status, const char *fmt, va_list args)
{
    char str[MAX_STRING_LEN];
    int  strLen = 0;
    char tmpStr[64];
    int  tmpStrLen = 0;
    int  pf = 0;

    //初始化
    memset(str, 0, MAX_STRING_LEN);
    memset(tmpStr, 0, 64);

    //加入LOG时间
    tmpStrLen = C_Error_GetCurTime(tmpStr);
    tmpStrLen = sprintf(str, "[%s] ", tmpStr);
    strLen = tmpStrLen;

    //加入LOG等级
    tmpStrLen = sprintf(str + strLen, "[%s] ", ICLevelName[level]);
    strLen += tmpStrLen;

	
    //加入LOG状态
    //if (status != 0)
    //{
    //    tmpStrLen = sprintf(str + strLen, "[ERRNO is %d] ", status);
    //}
    //else
    //{
    //    tmpStrLen = sprintf(str + strLen, "[SUCCESS] ");
    //}
    //strLen += tmpStrLen;
	
    //加入LOG信息
    //tmpStrLen = vsprintf(str + strLen, fmt, args);
    //strLen += tmpStrLen;
	

    //加入LOG发生文件
    tmpStrLen = sprintf(str + strLen, " @[%s]", file);
    strLen += tmpStrLen;

    //加入LOG发生行数
    tmpStrLen = sprintf(str + strLen, " [L%d] ", line);
    strLen += tmpStrLen;

	//加入LOG信息
    tmpStrLen = vsprintf(str + strLen, fmt, args);
    strLen += tmpStrLen;

	tmpStrLen = sprintf(str + strLen, "\n");
    strLen += tmpStrLen;

    //打开LOG文件
    if (C_Error_OpenFile(&pf))
    {
        return;
    }

    //写入LOG文件
    write(pf, str, strLen);
    //IC_Log_Error_WriteFile(str);
	//fflush(pf);
	fsync(pf);

    //关闭文件
    close(pf);

    return;
}
*/

static void C_Error_Core(char* log_file_path, const char *file,const char* function,int line, int level, int status, const char *fmt, va_list args)
{
    static char str[MAX_STRING_LEN] = { '\0' };
    int  strLen = 0;
    static char tmpStr[64] = { '\0' };
    int  tmpStrLen = 0;
    int  pf = 0;

    //初始化
    bzero(str, MAX_STRING_LEN);
    bzero(tmpStr, 64);

    //加入LOG时间
    tmpStrLen = C_Error_GetCurTime(tmpStr);
    tmpStrLen = sprintf(str, "%s", tmpStr);
    strLen = tmpStrLen;

    //加入LOG等级
    tmpStrLen = sprintf(str + strLen, "[%s]", ICLevelName[level]);
    strLen += tmpStrLen;
    tmpStrLen = sprintf(str + strLen, "[pid=%d tid=%ld]", getpid(), syscall(SYS_gettid));
    strLen += tmpStrLen;

    //加入LOG状态
    //if (status != 0)
    //{
    //    tmpStrLen = sprintf(str + strLen, "[ERRNO is %d] ", status);
    //}
    //else
    //{
    //    tmpStrLen = sprintf(str + strLen, "[SUCCESS] ");
    //}
    //strLen += tmpStrLen;

    //加入LOG信息
    //tmpStrLen = vsprintf(str + strLen, fmt, args);
    //strLen += tmpStrLen;
	

    //加入LOG发生文件
    tmpStrLen = sprintf(str + strLen, "[%s,%s,Line:%d]", file, function, line);
    strLen += tmpStrLen;

 /*   //加入LOG发生函数
    tmpStrLen = sprintf(str + strLen, "[%s]", function);
    strLen += tmpStrLen;
	
    //加入LOG发生行数
    tmpStrLen = sprintf(str + strLen, "[L%d]", line);
    strLen += tmpStrLen;*/

	//加入LOG信息
    tmpStrLen = vsprintf(str + strLen, fmt, args);
    strLen += tmpStrLen;

    tmpStrLen = sprintf(str + strLen, "\n");
    strLen += tmpStrLen;

    //打开LOG文件
    if (C_Error_OpenFile(&pf, log_file_path))
    {
        return;
    }

    //printf(str);
    //写入LOG文件
    write(pf, str, strLen);
    //IC_Log_Error_WriteFile(str);
    //fflush(pf);

#ifdef _FAT32_
	fsync(pf);
#endif

    //关闭文件
    close(pf);

    return;
}

/*
void C_LOG(const char *file, int line, int level, int status, const char *fmt, ...)
{
    va_list args;
	long length = 0;
	char    fileName[1024];

    memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s", DEBUG_FILE_);

    //判断是否需要写LOG
    //  if(level!=IC_DEBUG_LEVEL && level!=IC_INFO_LEVEL && level!=IC_WARNING_LEVEL && level!=IC_ERROR_LEVEL)
    if (level == IC_NO_LOG_LEVEL)
    {
        return;
    }

	length = get_file_size(fileName);
 
	if (length > SIXTY_FOUR_MB)
	{
		unlink(fileName); // 删除文件
	}
	

    //调用核心的写LOG函数
    va_start(args, fmt);
    C_Error_Core(file, line, level, status, fmt, args);
    va_end(args);

    return;
}
*/

void C_LOG(char* log_file_path, const char *file,const char* function, int line, int level, int status, const char *fmt, ...)
{	
	/*return; */
	va_list args;
	long length = 0;
	char    fileName[1024] = { '\0' };

	memset(fileName, 0, sizeof(fileName));
	sprintf(fileName, "%s", log_file_path);

    	//判断是否需要写LOG
    	//  if(level!=IC_DEBUG_LEVEL && level!=IC_INFO_LEVEL && level!=IC_WARNING_LEVEL && level!=IC_ERROR_LEVEL)
    	if (level == IC_NO_LOG_LEVEL)
    	{
        	return;
    	}

	length = get_file_size(fileName);
 
	if (length > SIXTY_FOUR_MB)
	{
		unlink(fileName); // 删除文件
	}
	

    	//调用核心的写LOG函数
    	va_start(args, fmt);
    	C_Error_Core(fileName, file, function, line, level, status, fmt, args);
    	va_end(args);

    	return;
}
