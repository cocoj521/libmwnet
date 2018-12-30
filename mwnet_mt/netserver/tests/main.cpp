#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

int main(int argc, char* argv[])
{
	dlopen("./gen_so.so",RTLD_NOW);
	printf("err:%s\n",dlerror());
	return 0;
}