#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>  
#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <ext/atomicity.h>
#include <signal.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"


char g_szIp[30] = {"192.169.3.245"};
char g_szSendMsg[1024] = {0};
int g_nPort = 5958;
int g_nMode = 2;
int g_total = 0;
int g_lasttotal = 0;
int g_maxdelay = 0;
int g_mindelay = 0;
int g_totaldelay = 0;

int GetTickCount (void)
{
    struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec*1000 + t.tv_usec/1000;
}

/*
void ThreadOnlyRecv(void*)
{	
	bool bOK = false;
	SOCKET sockClient;
	while(1)
	{
		if (!bOK)
		{
			printf("begin create socket...\n");
			sockClient=socket(AF_INET,SOCK_STREAM,0);//´´½¨һ¸�ket
			if(sockClient==INVALID_SOCKET) 
			{ 
				printf("create socket success\n");
				WSACleanup();
				printf("%d",int(sockClient));
				continue;
			}
			SOCKADDR_IN addrSrv;
			addrSrv.sin_addr.S_un.S_addr=inet_addr(g_szIp); //ת»»³ɺЊːϊ½µÉP(ؖ·�>unsigned long)
			addrSrv.sin_family=AF_INET;//Эөإ
			addrSrv.sin_port=htons(g_nPort);//¶˿ںÍ
			printf("Connectting to the server...\n");
			//Sleep(1000);
			if(connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR)))//l½э
			{
				//DWORD dwEr = GetLastError();
				closesocket(sockClient);
				
				printf("Connect error! continue connect...\n");
				sleep(1);
				continue;
			}
			else
			{
				printf("Connect successfully! socket = %d\n", sockClient);
			}
			bOK = true;
		}

		//Sleep(100000);
		char szBuf[20480] = {0};
		char szSendBuf[20480] = {0};
		memset(szBuf, 0, sizeof(szBuf));
		//int nBufLen = strlen(szBuf);
		//int nSendLen = sprintf(szSendBuf, szHttpHeader, g_szIp, nBufLen, szBuf);
		int nSendLen = sprintf(szSendBuf, "active test");
		if (SOCKET_ERROR != send(sockClient,szSendBuf,nSendLen,0))
		{
			memset(szSendBuf, 0, sizeof(szSendBuf));
			if (SOCKET_ERROR != recv(sockClient, szSendBuf, 20480, 0))
			{
				//printf("Recv msg from server:%s\n", szSendBuf);
				__gnu_cxx::__atomic_add((_Atomic_word*)(&g_total),1);
			}
			else
			{
				printf("Recv from server Error! Errcode=%d\n", WSAGetLastError());
				bOK = false;
				closesocket(sockClient);
			}
		}
		else
		{
			printf("Send to server Error! Errcode=%d\n", WSAGetLastError());
			bOK = false;
			closesocket(sockClient);
		}
		Sleep(3000);
	}
}
*/
#define myrandom(a,b) (rand()%(b-a+1)+a)

void* ThreadSendAndRecv(void*)
{	
	bool bOK = false;
	unsigned int sockClient;
	while(1)
	{
		if (!bOK)
		{
			printf("create socket...\n");
			sockClient=socket(AF_INET,SOCK_STREAM,0);//´´½¨һ¸�ket
			if(sockClient==-1) 
			{ 
				printf("create socket success.\n");
				//WSACleanup();
				printf("%d",int(sockClient));
				continue;
			}
			sockaddr_in addrSrv;
			addrSrv.sin_addr.s_addr=inet_addr(g_szIp); //ת»»³ɺЊːϊ½µÉP(ؖ·�>unsigned long)
			addrSrv.sin_family=AF_INET;//Эөإ
			addrSrv.sin_port=htons(g_nPort);//¶˿ںÍ
			
			printf("Connectting to the server...\n");
			//Sleep(1000);
			if(connect(sockClient,(sockaddr*)&addrSrv,sizeof(sockaddr)))//l½э
			
			{
				close(sockClient);
				
				printf("Connect error! continue...\n");
				sleep(1);
				continue;
			}
			else
			{
				printf("Connect successfully! socket = %d\n", sockClient);
			}
			bOK = true;
		}

		//Sleep(100000);
		char szRecvBuf[1024+1] = {0};
		//int nBufLen = strlen(szBuf);
		//int nSendLen = sprintf(szSendBuf, szHttpHeader, g_szIp, nBufLen, szBuf);
		int nSendLen = strlen(g_szSendMsg);
		int nStart = GetTickCount();
		int nRet = 0;
		if (-1 != (nRet=send(sockClient,g_szSendMsg,nSendLen,0)))
		{
			if (3 != g_nMode)
			{
				if (-1 != (nRet=recv(sockClient, szRecvBuf, 1024, 0)))
				{
					int nEnd = GetTickCount();
					int nTm = nEnd-nStart;
					if (nTm > g_maxdelay) g_maxdelay = nTm;
					if (nTm < g_mindelay) g_mindelay = nTm;
					
					g_totaldelay += nTm;
					
					//printf("Recv msg from server:%s\n", szSendBuf);
					__gnu_cxx::__atomic_add((_Atomic_word*)(&g_total),1);
				}
				else
				{
					printf("Recv from server Error! \n");
					bOK = false;
					close(sockClient);
				}
			}
			else
			{
				//usleep(1000*1);
				//printf("send ok....\n");
			}
		}
		else
		{
			printf("Send to server Error!\n");
			bOK = false;
			close(sockClient);
		}

		if (1 == g_nMode) 
		{
			srand(time(NULL));
			int n = myrandom(200,295);
			sleep(n);
		}
	}
	return NULL;
}

void signal_callback(int signo)
{

}
int main()
{ 
    signal(SIGPIPE, SIG_IGN);       //13 产生  SIGPIPE 信号时就不会中止程序，直接把这个信号忽略掉。
    signal(SIGPIPE, signal_callback);//13  SIGPIPE 13 A 管道破裂: 写一个没有读端口的管道
/*
	int i=0; 
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err; 
	wVersionRequested = MAKEWORD(2,1);
	err = WSAStartup(wVersionRequested,&wsaData);
	if(err)
	{   
		printf("WSAStartup error.\n");
		return; 
	}
	
	if(LOBYTE( wsaData.wVersion ) != 2||
		HIBYTE( wsaData.wVersion) != 1)
	{
		//WSACleanup();
		printf("error version\n");
		return;
	}
	printf("WSAStartup success\n");
*/
	printf("input IP:");
	scanf("%s", g_szIp);
	getchar();
	printf("input Port:");
	scanf("%d", &g_nPort);
	getchar();
	g_nMode = 2;

	int nConnNum = 1;
	printf("input thread num:");
	scanf("%d", &nConnNum);
	getchar();

	printf("input run mode(1:active test 2:loop send 3:only send):");
	scanf("%d", &g_nMode);
	getchar();
	
	printf("input test msg(<1024Bytes):");
	scanf("%s", g_szSendMsg);
	getchar();

	if (strlen(g_szSendMsg) <= 0)
	{
		strcpy(g_szSendMsg, "send test");
	}

	{
		for (int j = 0; j < nConnNum; ++j)
		{
			pthread_t ntid;
    		int 	err = pthread_create(&ntid, NULL, ThreadSendAndRecv, NULL);
    		if (err != 0)
        	printf("can't create thread: %s\n", strerror(err));
    		pthread_detach(ntid);
		}
	}

	int nTm = 1;
	while (1)
	{
		int ntmp = g_total;
		printf("total:%d,spd:%d,mindelay:%dms,maxdelay:%dms,avgdelay:%dms\r\n", ntmp, ntmp-g_lasttotal, g_mindelay, g_maxdelay, g_totaldelay/(nTm*1000));
		g_lasttotal = ntmp;
		
		sleep(1);

		++nTm;
	}
	return 0;
} 

