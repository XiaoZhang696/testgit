#pragma once

#ifndef _C_LOG_H_
#define _C_LOG_H_

#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>

/*
#define ZDBG(fmt, args...) do { \
	printf(fmt, ##args);	\
	C_LOG(LogFileName, __FILE__,__FUNCTION__, __LINE__, LogLevel[2], 0, fmt, ##args); \
	} while(0)
#define ZERR(fmt, args...) do { \
	printf(fmt, ##args);	\
	C_LOG(LogFileName, __FILE__,__FUNCTION__, __LINE__, LogLevel[4], 0, fmt, ##args); \
	} while(0)	
	*/
#define ZDBG(fmt, args...) do { \
		C_LOG(LogFileName, __FILE__,__FUNCTION__, __LINE__, LogLevel[2], 0, fmt, ##args); \
		} while(0)
#define ZERR(fmt, args...) do { \
		C_LOG(LogFileName, __FILE__,__FUNCTION__, __LINE__, LogLevel[4], 0, fmt, ##args); \
		} while(0)	

#ifdef  __cplusplus
extern "C" {
#endif


    /*
    #define IC_NO_LOG_LEVEL         0
    #define IC_DEBUG_LEVEL          1
    #define IC_INFO_LEVEL           2
    #define IC_WARNING_LEVEL        3
    #define IC_ERROR_LEVEL          4;
    */

    /************************************************************************/
    /*
    const char *file���ļ�����
    int line���ļ��к�
    int level�����󼶱�
    0 -- û����־
    1 -- debug����
    2 -- info����
    3 -- warning����
    4 -- err����
    int status��������
    const char *fmt���ɱ����
    */
    /************************************************************************/
    //ʵ��ʹ�õ�Level
    extern int  LogLevel[5];
	extern char LogFileName[256];
    //void C_LOG(const char *file, int line, int level, int status, const char *fmt, ...);
	void C_LOG(char* log_file_path, const char *file,const char* function, int line, int level, int status, const char *fmt, ...);
	void InitialLog(char *szLogFile);

#ifdef __cplusplus_
}
#endif

#endif
