/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/script/eventsystem.hpp>
#include <engine/common/gamedata.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(EventSystem);

  struct EventSystem::Impl
  {
    Impl(EventsManifest const& iManifest, World& iWorld)
      : m_Objects(iWorld)
    {
      for (auto const& itf : iManifest.m_Interfaces)
      {
        for (auto const& function : itf.second)
        {
          Name functionKey(itf.first + "::" + function.first);
          m_FunctionsIndex.insert(std::make_pair(functionKey, &function.second));
        }
      }
    }
    struct ObjectEntry
    {
      UnorderedMap<Name, HandlerEntry> m_Handlers;
    };

    DenseGameDataStorage<ObjectEntry> m_Objects;
    UnorderedMap<Name, FunDesc const*> m_FunctionsIndex;
  };

  EventSystem::EventSystem(EventsManifest const& iManifest)
    : m_Manifest(iManifest)
  {
  }

  EventSystem::~EventSystem() = default;

  void EventSystem::Register(World& iWorld)
  {
    WorldSystem::Register(iWorld);
    m_Impl = std::make_unique<Impl>(m_Manifest, iWorld);
  }


  void EventSystem::GarbageCollect()
  {
    m_Impl->m_Objects.GarbageCollect();
  }

  FunDesc const* EventSystem::GetFunDesc(Name iFunction)
  {
    auto funIter = m_Impl->m_FunctionsIndex.find(iFunction);
    if (funIter == m_Impl->m_FunctionsIndex.end())
    {
      return nullptr;
    }

    return funIter->second;
  }

  void EventSystem::AddEventHandlerInternal(ObjectHandle iObject, Name iFunction, GenericHandler iFun, void* iPayload)
  {
    if (!GetWorld().IsObjectValid(iObject))
    {
      return;
    }

    eXl_ASSERT_REPAIR_RET(GetFunDesc(iFunction) != nullptr, void());

    Impl::ObjectEntry& entry = m_Impl->m_Objects.GetOrCreate(iObject);
    entry.m_Handlers.insert_or_assign(iFunction, HandlerEntry{iFun, iPayload});
  }

  EventSystem::HandlerEntry const* EventSystem::GetEventHandlerInternal(ObjectHandle iObject, Name iFunction)
  {
    if (!GetWorld().IsObjectValid(iObject))
    {
      return nullptr;
    }

    Impl::ObjectEntry const* entry = m_Impl->m_Objects.Get(iObject);
    if (entry == nullptr)
    {
      return nullptr;
    }

    auto iter = entry->m_Handlers.find(iFunction);
    if (iter == entry->m_Handlers.end())
    {
      return nullptr;
    }

    return &iter->second;
  }
}