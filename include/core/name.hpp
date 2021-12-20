/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/string.hpp>
#include <core/coredef.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/no_tracking.hpp>
#include <boost/flyweight/intermodule_holder.hpp>

#define MAKE_NAME_DECL(NameType) \
struct NameType : eXl::Name \
{ \
  NameType() {} \
  NameType(NameType const& iOther) : Name(static_cast<eXl::Name const&>(iOther)) {} \
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

#if !defined(EXL_SHARED_LIBRARY)
  using Name = boost::flyweight<String, boost::flyweights::tag<DefaultTag>, boost::flyweights::no_tracking>;
#endif

#if defined(EXL_SHARED_LIBRARY)
#if defined(EXL_NAME_EXPLICIT_INIT)

#if !defined(BOOST_FLYWEIGHT_EXPLICIT_INIT_PATCH)
#error("Need boost patch")
#endif

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
#else
  using Name = boost::flyweight<String, boost::flyweights::tag<DefaultTag>, boost::flyweights::intermodule_holder, boost::flyweights::no_tracking>;
#endif
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