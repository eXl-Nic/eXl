#include <core/corelib.hpp>
#include <core/log.hpp>
#include "qtapplication.hpp"


#ifdef WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <cstring>

int main(int argc, char *argv[])
{
#ifdef WIN32
    CHAR exePath[512];
    DWORD oSize = GetModuleFileName(NULL,exePath,512-1);

    char * posBack = strrchr(exePath,'\\');
    std::string path(exePath,posBack);
    chdir(path.c_str());
  
#else
    char exePath[512];
    ssize_t len = ::readlink("/proc/self/exe", exePath, sizeof(exePath)-1);
    if (len != -1) 
    {
      exePath[len] = '\0';
      char * posBack = strrchr(exePath,'/');
      std::string path(exePath,posBack);
      //path = path + "/bin";
      chdir(path.c_str());
    }
#endif
  
  eXl::QTApplication s_Appl;

  eXl::InitFileLog("eXl_Editor.log");
  s_Appl.SetAppPath(exePath);
  s_Appl.SetArgs(argc,argv);
  s_Appl.Start();
  
  s_Appl.DefaultLoop();
  
  return 0;
}
