/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <core/type/fundesc.hpp>
#include <engine/common/world.hpp>

namespace eXl
{
  struct EventsManifest
  {
    using FunctionsMap = UnorderedMap<String, FunDesc>;
    UnorderedMap<String, FunctionsMap> m_Interfaces;
  };

  class EventSystem : public WorldSystem
  {
    DECLARE_RTTI(EventSystem, WorldSystem);

    struct Impl;
  public:

    EventSystem(EventsManifest const& iManifest);
    ~EventSystem();

    void Register(World& iWorld) override;

    void GarbageCollect();

    template<typename Ret, typename... Args>
    void AddEventHandler(ObjectHandle iObject, Name iFunction, std::function<Ret(Args...)> iFun)
    {
      if (FunDesc const* desc = GetFunDesc(iInterface, iFunction))
      {
        if (desc->ValidateSignature<Ret, Args...>())
        {
          auto cb = [this, fun = std::move(iFun)](Name, ObjectHandle, ConstDynObject const& iArgsBuffer, DynObject& oOutput)
          {
            Invoker_RetWrapper<Ret, Args...>::Execute([this, &fun](Args... iArgs)
              {
                eXl_ASSERT_REPAIR_RET(!(!fun), RetType());
                return iFun(std::forward<Args>(iArgs)...);
              }, iArgsBuffer, oOutput);
          };
          AddEventHandlerInternal(iObject, iInterface, iFunction, std::move(cb));
        }
      }
    }

    template <typename Ret, typename... Args, typename std::enable_if<std::is_same<Ret, void>::value, bool>::type = true>
    Err Dispatch(ObjectHandle iHandle, Name iFunction, Args&&... iArgs)
    {
      GenericHandler const* handler = GetEventHandlerInternal(iHandle, iFunction);
      if (handler == nullptr)
      {
        return Err::Failure;
      }
      FunDesc const* desc = GetFunDesc(iFunction);
      bool validSignature = desc->ValidateSignature<void, Args...>();
      eXl_ASSERT_REPAIR_RET(validSignature, Err::Error);
      
      ArgsBuffer buffType(desc->arguments);
      uint8_t argsStore[ComputeArgListStorage<Args...>::size];
      DynObject argsObj(&buffType, argsStore);
      BufferPopulator<0, Args...>::Populate(buffType, argsObj, std::forward<Args>(iArgs)...);
      DynObject output;
      (*handler)(iHandle, iFunction, argsObj, output);

      return Err::Success;
    }

    template <typename Ret, typename... Args, typename std::enable_if<!std::is_same<Ret, void>::value, bool>::type = true>
    Optional<Ret> Dispatch(ObjectHandle iHandle, Name iFunction, Args&&... iArgs)
    {
      GenericHandler const* handler = GetEventHandlerInternal(iHandle, iFunction);
      if (handler == nullptr)
      {
        return {};
      }
      FunDesc const* desc = GetFunDesc(iFunction);
      bool validSignature = desc->ValidateSignature<Ret, Args...>();
      eXl_ASSERT_REPAIR_RET(validSignature, Err::Error);

      ArgsBuffer buffType(desc->arguments);
      uint8_t argsStore[ComputeArgListStorage<Args...>::size];
      DynObject argsObj(&buffType, argsStore);
      BufferPopulator<0, Args...>::Populate(buffType, argsObj, std::forward<Args>(iArgs)...);
      Ret outputStore;
      DynObject output(TypeManager::GetType<Ret>(), &outputStore);
      (*handler)(iHandle, iFunction, argsObj, output);

      return outputStore;
    }

    FunDesc const* GetFunDesc(Name iFunction);
    EventsManifest const& GetManifest() { return m_Manifest; }

    using GenericHandler = std::function<void(ObjectHandle iObject, Name iFunction, ConstDynObject const& iArgsBuffer, DynObject& oOutput)>;

    void AddEventHandlerInternal(ObjectHandle iObject, Name iFunction, GenericHandler iFun);

    GenericHandler const* GetEventHandlerInternal(ObjectHandle iObject, Name iFunction);

  private:
    EventsManifest const& m_Manifest;
    UniquePtr<Impl> m_Impl;
  };
}