#include <mwnet_mt/base/CurrentThread.h>
#include <mwnet_mt/base/TimeZone.h>
#include <mwnet_mt/base/LogStream.h>
#include <mwnet_mt/base/Timestamp.h>
#include <mwnet_mt/base/StringPiece.h>
#include <mwnet_mt/base/Types.h>
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/AsyncLogging.h>
#include <assert.h>
#include <string.h> // memcpy
#include <string>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>


#include "MWTimestamp.h"

using namespace mwnet_mt;
using namespace MWTIMESTAMP;

typedef std::shared_ptr<mwnet_mt::Timestamp> TimestampPtr;

namespace MWTIMESTAMP
{
Timestamp::Timestamp()
{
	TimestampPtr pTimestamp(new mwnet_mt::Timestamp(0));
	m_any = pTimestamp;
}

Timestamp::Timestamp(int64_t microSecondsSinceEpochArg)
{
	TimestampPtr pTimestamp(new mwnet_mt::Timestamp(microSecondsSinceEpochArg));
	m_any = pTimestamp;
}

Timestamp::~Timestamp()
{

}
std::string Timestamp::ToString() const
{
	TimestampPtr pTimestamp(boost::any_cast<TimestampPtr>(m_any));

	if (pTimestamp)
	{
		return pTimestamp->toString();
	}

	return  "";
}

std::string Timestamp::ToFormattedString(bool showMicroseconds) const
{
	TimestampPtr pTimestamp(boost::any_cast<TimestampPtr>(m_any));

	if (pTimestamp)
	{
		return pTimestamp->toFormattedString(showMicroseconds);
	}

	return std::string("");
}

bool Timestamp::Valid() const
{ 
	bool bRet = false;
	TimestampPtr pTimestamp(boost::any_cast<TimestampPtr>(m_any));

	if (pTimestamp)
	{
		bRet = pTimestamp->valid();
	}

	return bRet;
}

int64_t Timestamp::MicroSecondsSinceEpoch() const
{
	int64_t nTmp = 0;

	TimestampPtr pTimestamp(boost::any_cast<TimestampPtr>(m_any));

	if (pTimestamp)
	{
		nTmp = pTimestamp->microSecondsSinceEpoch();
	}

	return nTmp;
}

time_t Timestamp::SecondsSinceEpoch() const
{
	time_t tTime = 0;

	TimestampPtr pTimestamp(boost::any_cast<TimestampPtr>(m_any));

	if (pTimestamp)
	{
		tTime = pTimestamp->secondsSinceEpoch();
	}

	return tTime;
}

Timestamp Timestamp::Now()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	int64_t seconds = tv.tv_sec;

	return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

Timestamp Timestamp::Invalid()
{
	return Timestamp();
}

Timestamp Timestamp::FromUnixTime(time_t t, int microseconds)
{
	return Timestamp(static_cast<int64_t>(t)* kMicroSecondsPerSecond + microseconds);
}

Timestamp Timestamp::FromUnixTime(time_t t)
{
	return FromUnixTime(t, 0);
}

}