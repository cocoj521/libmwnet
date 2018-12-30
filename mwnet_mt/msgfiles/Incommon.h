#ifndef __IM_BUSINESS_INCOMMON_H__
#define __IM_BUSINESS_INCOMMON_H__

#include <stdio.h>
#include <endian.h>

#include <string>
#include <memory>

#include <arpa/inet.h>
#include <common/Monitor.h>
#include <mwnet_mt/util/MWLogger.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//#define PERFORMANCE_TEST_DEFINE		1
#define XIAOMI_SMSTEST_DEFINE			1
////////////////////////////////////////////////////////////////////////////////////////////////////

#if 1
#if 0
#define ACCESS_LOG_TRACE(fmt,args...) LOG_TRACE(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: TRACE " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_DEBUG(fmt,args...) LOG_DEBUG(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_INFO(fmt,args...) LOG_INFO(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_WARN(fmt,args...) LOG_WARN(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_ERROR(fmt,args...) LOG_ERROR(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_FATAL(fmt,args...) LOG_FATAL(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: FATAL " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define MGATE_LOG_TRACE(fmt,args...) LOG_TRACE(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: TRACE " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_DEBUG(fmt,args...) LOG_DEBUG(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_INFO(fmt,args...) LOG_INFO(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_WARN(fmt,args...) LOG_WARN(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_ERROR(fmt,args...) LOG_ERROR(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_FATAL(fmt,args...) LOG_FATAL(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: FATAL " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define MYSELF_LOG_TRACE(fmt,args...) LOG_TRACE(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: TRACE " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_DEBUG(fmt,args...) LOG_DEBUG(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_INFO(fmt,args...) LOG_INFO(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_WARN(fmt,args...) LOG_WARN(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_ERROR(fmt,args...) LOG_ERROR(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_FATAL(fmt,args...) LOG_FATAL(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: FATAL " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#else

#define ACCESS_LOG_TRACE(fmt,args...) LOG_TRACE(ACCESS_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: TRACE " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_DEBUG(fmt,args...) LOG_DEBUG(ACCESS_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_INFO(fmt,args...) LOG_INFO(ACCESS_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_WARN(fmt,args...) LOG_WARN(ACCESS_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_ERROR(fmt,args...) LOG_ERROR(ACCESS_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define ACCESS_LOG_FATAL(fmt,args...) LOG_FATAL(ACCESS_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: FATAL " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define MGATE_LOG_TRACE(fmt,args...) LOG_TRACE(MGATE_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: TRACE " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_DEBUG(fmt,args...) LOG_DEBUG(MGATE_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_INFO(fmt,args...) LOG_INFO(MGATE_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_WARN(fmt,args...) LOG_WARN(MGATE_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_ERROR(fmt,args...) LOG_ERROR(MGATE_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MGATE_LOG_FATAL(fmt,args...) LOG_FATAL(MGATE_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: FATAL " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define MYSELF_LOG_TRACE(fmt,args...) LOG_TRACE(MYSELF_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: TRACE " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_DEBUG(fmt,args...) LOG_DEBUG(MYSELF_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_INFO(fmt,args...) LOG_INFO(MYSELF_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_WARN(fmt,args...) LOG_WARN(MYSELF_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_ERROR(fmt,args...) LOG_ERROR(MYSELF_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define MYSELF_LOG_FATAL(fmt,args...) LOG_FATAL(MYSELF_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: FATAL " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#endif
#define SPEED_LOG_INFO(fmt,args...) LOG_INFO(SPEED_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define SPEED_LOG_ERROR(fmt,args...) LOG_ERROR(SPEED_MODULE_LOGS,MWLOGGER::LOCAL_FILE).Format(fmt, ## args);fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)


#else

#define ACCESS_LOG_TRACE(fmt,args...) LOG_TRACE(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define ACCESS_LOG_DEBUG(fmt,args...) LOG_DEBUG(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define ACCESS_LOG_INFO(fmt,args...) LOG_INFO(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define ACCESS_LOG_WARN(fmt,args...) LOG_WARN(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define ACCESS_LOG_ERROR(fmt,args...) LOG_ERROR(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define ACCESS_LOG_FATAL(fmt,args...) LOG_FATAL(ACCESS_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)

#define MGATE_LOG_TRACE(fmt,args...) LOG_TRACE(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MGATE_LOG_DEBUG(fmt,args...) LOG_DEBUG(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MGATE_LOG_INFO(fmt,args...) LOG_INFO(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MGATE_LOG_WARN(fmt,args...) LOG_WARN(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MGATE_LOG_ERROR(fmt,args...) LOG_ERROR(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MGATE_LOG_FATAL(fmt,args...) LOG_FATAL(MGATE_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)

#define MYSELF_LOG_TRACE(fmt,args...) LOG_TRACE(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MYSELF_LOG_DEBUG(fmt,args...) LOG_DEBUG(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MYSELF_LOG_INFO(fmt,args...) LOG_INFO(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MYSELF_LOG_WARN(fmt,args...) LOG_WARN(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MYSELF_LOG_ERROR(fmt,args...) LOG_ERROR(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)
#define MYSELF_LOG_FATAL(fmt,args...) LOG_FATAL(MYSELF_MODULE_LOGS,MWLOGGER::PRINT_STDOUT).Format(fmt, ## args)

#endif

#if 0

#define PDEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PINFO(fmt, args...)			INFO(fmt, ## args)

#define PWARN(fmt, args...)			WARN(fmt, ## args)

#define PERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#else

#define PDEBUG(fmt, args...)		fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PINFO(fmt, args...)

#define PWARN(fmt, args...)

#define PERROR(fmt, args...)		fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)


#endif

#if 0

#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PINFO(fmt, args...)		fprintf(stderr, "%s :: %s() %d: INFO " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PWARN(fmt, args...)		fprintf(stderr, "%s :: %s() %d: WARN " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#endif

#define MAX_INT_NUM							2147483648

#define MESSAGE_HEAD_BUF_LEN				4
#define HEART_BEAT_STR_LEN					10
//#define MGATE_HEART_BEAT_REQ				0x0002
//#define MGATE_HEART_BEAT_RSP				0x8002
////////////////////////////////////////////////////////////////////error code
//////inside error code
#define IMB_REQUEST_PARAMETER_ERROR			-1
#define IMB_SERVER_MALLOC_ERROR				-2
#define IMB_SERVER_REDIS_ERROR				-3

#define MGATE_NO_ONLINE_ERROR				-4
#define SDK_NO_ONLINE_ERROR					-5
#define ACCESS_SERVER_NO_ONLINE				-8

#define ACCESS_SOCK_WRITE_FULL				-6
#define ACCESS_SOCK_NEEWA_ADD				-7
//////output error code
#define REQUEST_PARAMTER_ERROR				4400
#define REDIS_OPERATION_ERROR				4401
#define MEMORY_MALLOC_ERROR					4402

#define SDK_OFFLINE_RESULT					4403
#define MGATE_OFFLINE_RESULT				4404

#define IM_PARAMTER_DEFECT					4405
///////////////////////////////////////////////////////////////////////////////////////////////
#define IM_RVSDTYPE_BTC						1
#define IM_RVSDTYPE_CTB						2

#define MSG_RECEIPT_TYPE					2102
#define MSG_READ_RECEIPT_TYPR				2103

#define MSG_RECEIPT_RSP_TYPE				1102
#define MSG_READ_RECEIPT_RSP_TYPE			1103

#define REPORT_DEVINFO_TYPE					3001
#define REPORT_SMSINFO_TYPE					3002

#define NOTIFY_ANANYSIS_TYPE				5001
#define NOTIFY_RECEIPT_TYPE					5002

#define SDK_ONLINE_FLAGNUM					1

#define DEFAULT_IM_TIMELINE_SECOND			3

#define MGATE_NEED_READRECEIPT_FLAG			3

#define IM_DEFAULT_EXPITM_HOUR				72


#define IM_SMMSGTYPE_OTTA					11
#define IM_SMMSGTYPE_SMSC					13
#define IM_SMMSGTYPE_SMSD					14
#define IM_SMMSGTYPE_OTTB					15
#define IM_SMMSGTYPE_OTTC					16
#define IM_SMMSGTYPE_SYSTEM					999

#define IM_REDIS_INIT_STATE					1
#define IM_REDIS_NOTIFY_STATE				2
#define IM_REDIS_RECV_STATE					4
#define IM_REDIS_PULL_STATE					8
#define IM_REDIS_READ_STATE					16
#define IM_REDIS_EXPI_STATE					32
#define IM_REDIS_SEND_FAILED				64


#define IM_REPORT_RECV_STATE				1
#define IM_REPORT_READ_STATE				2

#define SDK_RECEIPT_TARGET_ERROR			1
#define SDK_RECEIPT_TARGET_MISS				2
#define SDK_RECEIPT_SHOW_EXPIRE				3
#define SDK_RECEIPT_JSON_ERROR				4

///////////////////////////////////////////////////////////////////////////////////////////////
//#define ACCESS_LOGIN_REQ					0x2001
//#define ACCESS_LOGIN_RSP					0x2002

#define ACCESS_HEART_BEAT_REQ				0x1000
#define ACCESS_HEART_BEAT_RSP				0x2000

#define ACCESS_DROP_CMD_REQ					0x3000

#define ACCESS_LOGOUT_REQ					0x2003
#define ACCESS_LOGOUT_RSP					0x2004

///////////////////////////////////////////////////////////////////////////////////////////////
#define SDK_PUSH_MSG_REQ					0x1001
#define SDK_PUSH_NOTIFY_REQ					0x1003
#define SDK_PULL_MSG_REQ					0x1004
#define SDK_SEND_MSG_REQ					0x1006
#define SDK_SEND_RECEIPT_REQ				0x1008
#define SDK_REPORT_INFO_REQ					0x100a
#define SDK_ACTIVE_REQ						0x0005
#define SDK_QUERY_INFO_REQ					0x100c// 异步返回，序号对不上，已经处理

#define ACCESS_PUSH_ERR_RSP					0xe002

#define SDK_PUSH_MSG_RSP					0x1002
#define SDK_PULL_MSG_RSP					0x1005
#define SDK_SEND_MSG_RSP					0x1007
#define SDK_SEND_RECEIPT_RSP				0x1009
#define SDK_REPORT_INFO_RSP					0x100b
#define SDK_ACTIVE_RSP						0x0006
#define SDK_QUERY_INFO_RSP					0x100d

///////////////////////////////////////////////////////////////////////////////////////////////////
#define MGATE_LOGIN_REQ						0x0001
#define MGATE_HEART_BEAT_REQ				0x0002
#define MGATE_IM_REQ						0x0003
#define MGATE_REPORT_REQ					0x0004
#define MGATE_MO_REQ						0x0005


#define MGATE_LOGIN_RSP						0x8001
#define MGATE_HEART_BEAT_RSP				0x8002
#define MGATE_IM_RSP						0x8003
#define MGATE_REPORT_RSP					0x8004
#define MGATE_MO_RSP						0x8005
////////////////////////////////////////////////////////////////////////////////////////////////////


enum MODULE_LOGS
{
	ACCESS_MODULE_LOGS,
	MGATE_MODULE_LOGS,
	MYSELF_MODULE_LOGS,
	SPEED_MODULE_LOGS,
	MODULE_LOGS_NUM,
};

//extern const char* g_module_logs[];

void sigMyself();

int daemonize(int nochdir, int noclose);

inline uint16_t hostToNetwork16(uint32_t host16)
{
  return htobe16(host16);
}

inline uint16_t networkToHost16(uint32_t net16)
{
  return be16toh(net16);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
  return htobe32(host32);
}

inline uint32_t networkToHost32(uint32_t net32)
{
  return be32toh(net32);
}

inline uint64_t hostToNetwork64(uint64_t host64)
{
  return htobe64(host64);
}

inline uint64_t networkToHost64(uint64_t net64)
{
  return be64toh(net64);
}

inline const char* utilEndConstchar(const char* str)
{
    const char *p = str;
    if(str == NULL)
        return NULL;
    while(*p != 0)
    {
        p++;
    }
    p--;
    return p;
}

inline uint64_t strToBigNum(const char *p)
{
    int kk = 0, i = 0;
    uint64_t result = 0, tmp = 0;
    if(p == NULL)
        return result;
    const char *pEnd = utilEndConstchar(p);
    while(pEnd >= p)
    {
        if(*pEnd >= 48 && *pEnd <= 57)
        {
			tmp = *pEnd - 48;
			for(i = 0; i < kk; i++)
			{
				tmp = tmp * 10;
			}
            result += tmp;
        }else{
            break;
        }
		
        kk++;
        pEnd--;
        if(kk > 20)
            break;
    }
    return result;
}


#endif
