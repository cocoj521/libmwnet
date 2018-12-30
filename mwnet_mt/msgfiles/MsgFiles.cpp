#include "MsgFilesImpl.h"

namespace MSGFILES
{

CMsgFiles* CMsgFilesFactory::New()
{
	return new CMsgFilesImpl();
}

void CMsgFilesFactory::Destroy(CMsgFiles* files)
{
	if (files)
	{
		delete files;
		files = 0;
	}
}

} // end namespace MSGFILES
