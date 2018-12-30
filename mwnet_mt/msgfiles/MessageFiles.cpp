#include "MessageFilesImpl.h"

namespace MSGFILES
{

MessageFiles* MessageFilesFactory::New()
{
	return new MessageFilesImpl();
}

void MessageFilesFactory::Destroy(MessageFiles* files)
{
	if (files)
	{
		delete files;
		files = 0;
	}
}

} // end namespace MSGFILES
