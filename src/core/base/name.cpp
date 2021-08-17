#include <core/name.hpp>
#include <boost/optional.hpp>

namespace eXl
{
#if !defined(__ANDROID__)
  boost::aligned_storage<sizeof(NameCoreHolder)> s_NameSingleton;
#endif

  void Name_Init()
  {
#if !defined(__ANDROID__)
    NameCore::reset_init();
    new(s_NameSingleton.address()) NameCoreHolder;
    NameCore::init();
#endif
  }

  void Name_Destroy()
  {
#if !defined(__ANDROID__)
    TNameCoreSingleton<NameCoreHolder>::get().~NameCoreHolder();
#endif
  }

#if !defined(__ANDROID__)
  NameCoreHolder& TNameCoreSingleton<NameCoreHolder>::get()
  {
    return *reinterpret_cast<NameCoreHolder*>(s_NameSingleton.address());
  }
#endif
}