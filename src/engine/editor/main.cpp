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
  eXl::QTApplication s_Appl;

  eXl::InitFileLog("eXl_Editor.log");
  s_Appl.SetArgs(argc, argv);
  s_Appl.Start();
  
  s_Appl.DefaultLoop();
  
  return 0;
}
