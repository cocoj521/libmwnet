#include <mwnet_mt/base/Exception.h>
#include <stdio.h>

class Bar
{
 public:
  void test()
  {
    throw mwnet_mt::Exception("oops");
  }
};

void foo()
{
  Bar b;
  b.test();
}

int main()
{
  try
  {
    foo();
  }
  catch (const mwnet_mt::Exception& ex)
  {
    printf("reason: %s\n", ex.what());
    printf("stack trace: %s\n", ex.stackTrace());
  }
}
