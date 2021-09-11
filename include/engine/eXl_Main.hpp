#pragma once

#ifdef EXL_SHARED_LIBRARY

#ifdef WIN32

#ifdef BUILD_MAIN_DLL
#define EXL_MAIN_API __declspec(dllexport)
#else
#define EXL_MAIN_API __declspec(dllimport)
#endif

#elif defined __GNUC__

#ifdef BUILD_MAIN_DLL
#define EXL_MAIN_API __attribute__ ((visibility("default")))
#else
#define EXL_MAIN_API
#endif

#else
#define EXL_MAIN_API
#endif

#else
#define EXL_MAIN_API
#endif

#include <engine/common/app.hpp>

class EXL_MAIN_API eXl_Main
{
public:

  eXl_Main();

  int Start(int argc, char* argv[]);
};

#define EXL_MAIN_WITH_SCENARIO(Scenario) \
using namespace eXl; \
int main(int argc, char* argv[]) \
{ \
  eXl_Main entryPoint; \
  Engine_Application& app = Engine_Application::GetAppl();\
  app.SetScenario(std::make_unique<Scenario>()); \
  return entryPoint.Start(argc, argv); \
}

#define EXL_MAIN_WITH_SCENARIO_AND_PROJECT(Scenario, ProjectPath) \
using namespace eXl; \
int main(int argc, char* argv[]) \
{ \
  eXl_Main entryPoint; \
  Engine_Application& app = Engine_Application::GetAppl();\
  app.SetScenario(std::make_unique<Scenario>()); \
  app.SetProjectPath(ProjectPath); \
  return entryPoint.Start(argc, argv); \
}
