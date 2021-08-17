#pragma once

#include <core/string.hpp>
#include <core/coredef.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/no_tracking.hpp>
//#include <boost/unordered_map.hpp>

#define MAKE_NAME_DECL(NameType) \
struct NameType : eXl::Name \
{ \
  NameType() {} \
  NameType(NameType const& iOther) : Name(iOther) {} \
  NameType(eXl::String const& iStr) : Name(iStr.c_str()) {} \
  NameType(eXl::Char const* iStr) : Name(iStr) {} \
  explicit NameType(eXl::Name const& iOtherName) : Name(iOtherName) {} \
  NameType& operator=(eXl::Char const* iStr) {*static_cast<eXl::Name*>(this) = iStr ; return *this; } \
  bool operator == (NameType const& iOther) const { return *static_cast<eXl::Name const*>(this) == static_cast<eXl::Name const&>(iOther); } \
  bool operator != (NameType const& iOther) const { return !(*this == iOther); } \
  bool operator < (NameType const& iOther) const { return *static_cast<eXl::Name const*>(this) < static_cast<eXl::Name const&>(iOther); } \
  Err Stream(eXl::Streamer& iStreamer) const { return iStreamer.Write(static_cast<eXl::Name const*>(this));}; \
  Err Unstream(eXl::Unstreamer& iUnstreamer) { return iUnstreamer.Read(static_cast<eXl::Name*>(this)); }; \
}

#define MAKE_NAME_TYPE(NameType) \
REDIRECT_TYPE_EX(NameType, eXl::Name); \
inline size_t hash_value(NameType const& iName) \
{ \
  return hash_value(static_cast<eXl::Name const&>(iName)); \
}

#define MAKE_NAME(NameType) \
MAKE_NAME_DECL(NameType); \
MAKE_NAME_TYPE(NameType)

namespace eXl
{
  struct DefaultTag {};
  //template <typename T = DefaultTag>

#if defined(__ANDROID__)
  using Name = boost::flyweight<String, boost::flyweights::tag<DefaultTag>, boost::flyweights::no_tracking>;
#else

  template<typename C>
  struct TNameCoreSingleton;

  struct eXlIntermoduleHolder : boost::flyweights::holder_marker
  {
    template<typename C>
    struct apply
    {
      typedef TNameCoreSingleton<C> type;
    };
  };

  using Name = boost::flyweight<String, boost::flyweights::tag<DefaultTag>, eXlIntermoduleHolder, boost::flyweights::no_tracking>;

  using NameCore = boost::flyweights::detail::flyweight_core<
    boost::flyweights::detail::default_value_policy<String>, DefaultTag, boost::flyweights::no_tracking,
    boost::flyweights::hashed_factory<boost::mpl::na, boost::mpl::na, boost::mpl::na, 0>, boost::flyweights::simple_locking, eXl::eXlIntermoduleHolder>;

  using NameCoreHolder = NameCore::holder_struct;


  template<>
  struct TNameCoreSingleton<NameCoreHolder>
  {
    EXL_CORE_API static NameCoreHolder& get();
  };

#endif
}

#include <core/type/typetraits.hpp>
#include <core/vlog.hpp>
namespace eXl
{
  inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM, const Name& tolog)
  {
    LM.write(tolog.get());
    return LM;
  }
}