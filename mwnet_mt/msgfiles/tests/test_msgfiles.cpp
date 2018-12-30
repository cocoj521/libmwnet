#include "../MsgFiles.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <memory>

using namespace MSGFILES;

std::shared_ptr<CMsgFiles> msgfilesptr;

void* SwitchMsgFiles(void* p)
{
	while(1)
	{
		if (msgfilesptr)
		{
			msgfilesptr->SwitchMsgFiles();
		}
		
		sleep(1);
	}
	return NULL;
}



int main(int argc, char* argv[])
{
	std::string strPtCode = "ptcode";
	int ptid = 100;
	int gateno = 10;

	pthread_t ntid = 0;

	pthread_create(&ntid, NULL, SwitchMsgFiles, NULL);

	msgfilesptr.reset(CMsgFilesFactory::New());
	
	msgfilesptr->InitParams(strPtCode,ptid,gateno);
	
	msgfilesptr->InitMsgFiles(strPtCode,ptid,gateno,"filetype1",20,10);
	
	//msgfilesptr->InitMsgFiles(strPtCode,ptid,gateno,"filetype2",20,10);

	std::string strContent = "your content.....\n";
	
	msgfilesptr->WriteFile("filetype1", strContent);
	
	//msgfilesptr->WriteFile("filetype2", strContent);

	while(1)
	{
		usleep(100);
		
		msgfilesptr->WriteFile("filetype1", strContent);
	
		//msgfilesptr->WriteFile("filetype2", strContent);
	}
}

