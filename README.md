# libmwnet
libmwnet is a multithreaded C++ network library based on
the reactor pattern.
It runs on Linux with kernel version >= 2.6.28.

## 版本控制
1.每个模块的版本管理,需要在编译前到对应模块文件夹下,修改Version.cc文件
2.查看模块版本号的方法:strings xxxx.a | grep lib_version