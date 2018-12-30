#include "../MWTimestamp.h"
#include <iostream>

using namespace MWTIMESTAMP;

int main()
{
	Timestamp t1 = Timestamp::Now();
	Timestamp t2;
	t2.Swap(t1);
	std::cout << "now-->" << t1.ToFormattedString() << std::endl;
	std::cout << "t1 cmp t2-->" << (t2 == t1) << std::endl;

	return 0;
}