#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/Logging.h>

std::unique_ptr<mwnet_mt::LogFile> g_logFile;

void outputFunc(const char* msg, int len)
{
  g_logFile->append(msg, len);
}

void flushFunc()
{
  g_logFile->flush();
}

int main(int argc, char* argv[])
{
  char name[256];
  strncpy(name, argv[0], 256);
  g_logFile.reset(new mwnet_mt::LogFile(::basename(name), 200*1000));
  mwnet_mt::Logger::setOutput(outputFunc);
  mwnet_mt::Logger::setFlush(flushFunc);

  mwnet_mt::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (int i = 0; i < 10000; ++i)
  {
    LOG_INFO << line << i;

    usleep(1000);
  }
}
