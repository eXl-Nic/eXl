#pragma once

#include <core/coredef.hpp>

namespace eXl
{
  template <typename Type, uint32_t Position >
  struct PositionalArg
  {
    using ArgType = typename std::remove_reference<typename std::remove_const<Type>::type>::type;
    static constexpr uint32_t ArgPos = Position;
  };

  template <typename... Args>
  struct PositionalList 
  {};

  template <typename Dummy, uint32_t Step, uint32_t Max, typename... Args >
  struct MakePositionalList_Impl;

  template <typename Dummy, uint32_t Max, typename... Args >
  struct MakePositionalList_Impl<Dummy, Max, Max, Args...>
  {
    using type = PositionalList<Args...>;
  };

  template <uint32_t Step, uint32_t Max, typename NextArg, typename... Args >
  struct MakePositionalList_Impl <typename std::enable_if<Step != Max, bool>::type, Step, Max, NextArg, Args...>
    : MakePositionalList_Impl<bool, Step + 1, Max, Args..., PositionalArg<NextArg, Step>>
  {};

  template <typename... Args>
  struct MakePositionalList : MakePositionalList_Impl<bool, 0, sizeof...(Args), Args...>
  {};

  template<uint32_t iNum, typename... Args>
  struct Positional_Get;

  template<uint32_t iNum, typename... Args>
  struct Positional_Get<iNum, PositionalList<Args...>>
  {
    using type = typename Positional_Get<iNum, Args...>::type;
  };

  template<typename Arg, typename... Args>
  struct Positional_Get<0, Arg, Args...>
  {
    using type = Arg;
  };

  template<uint32_t iNum, typename Arg, typename... Args>
  struct Positional_Get<iNum, Arg, Args... >
  {
    using type = typename Positional_Get<iNum - 1, Args...>::type;
  };

  template<typename... Args>
  struct Positional_Size;

  template<typename... Args>
  struct Positional_Size<PositionalList<Args...>>
  {
    static constexpr size_t size = Positional_Size<Args...>::size;
  };

  template<>
  struct Positional_Size<>
  {
    static constexpr size_t size = 0;
  };

  template<typename Arg, typename... Args>
  struct Positional_Size<Arg, Args... >
  {
    static constexpr size_t size = Positional_Size<Args...>::size + 1;
  };
}