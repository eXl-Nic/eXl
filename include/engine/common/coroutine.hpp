/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/common/animation.hpp>
#include <engine/common/world.hpp>
#include <engine/common/gamedata.hpp>
#include <engine/common/animation.cxx>

#ifdef Yield
#undef Yield
#endif


namespace eXl
{
  template <typename Coroutine>
  struct CoroutineState
  {
    float baseStep;
    uint32_t pauseCycle = 0;
    float yieldedFor = -1;
    ObjectTableHandle_Base timeHandle;
    Coroutine coroutine;
  };

  struct CoroutineAPI;
  DECLARE_ENGINE_TYPE(CoroutineAPI)

  struct CoroutineAPI
  {
    CoroutineAPI() = default;
    CoroutineAPI(CoroutineAPI const&) = delete;
    CoroutineAPI(CoroutineAPI&&) = delete;
    CoroutineAPI& operator=(CoroutineAPI const&) = delete;
    CoroutineAPI& operator=(CoroutineAPI&&) = delete;

    void Yield(float iTime)
    {
      yieldRequest = iTime;
    }
    void Stop()
    {
      stopRequest = true;
    }
    void Pause()
    {
      pauseRequest = true;
    }

    Optional<float> yieldRequest;
    bool pauseRequest = false;
    bool stopRequest = false;
  };

  template <typename Coroutine>
  struct CoroutineSchedule
  {
    ObjectHandle object;
    ObjectTableHandle<CoroutineState<Coroutine>> stateHandle;
    double prevTick;
    double nextTick;
    uint32_t pauseCycle;
    bool operator < (CoroutineSchedule const& iOther) const
    {
      return nextTick > iOther.nextTick;
    }
  };

  template <typename Coroutine>
  class T_CoroutineManager
  {
    using CoHandle = ObjectTableHandle<CoroutineState<Coroutine>>;
  public:

    Err AddCoroutine(World& iWorld, ObjectHandle iObject, Coroutine iState, float iStepTime = std::numeric_limits<float>::min(), bool iStartPaused = false);
    Err RemoveCoroutine(World& iWorld, ObjectHandle iObject);
    void Pause(World& iWorld, ObjectHandle iObject);
    void Resume(World& iWorld, ObjectHandle iObject);
    void Tick(World& iWorld);
    void Cleanup(World& iWorld);

  protected:
    
    Vector<CoroutineSchedule<Coroutine>> m_Time;
    
    ObjectTable<CoroutineState<Coroutine>> m_CoState;
    UnorderedMap<ObjectHandle, CoHandle> m_ObjectToCo;
    Vector<ObjectHandle> m_ToDelete;
  };

  template <typename Coroutine>
  Err T_CoroutineManager<Coroutine>::AddCoroutine(World& iWorld, ObjectHandle iObject, Coroutine iState, float iStepTime, bool iStartPaused)
  {
    if (!iWorld.IsObjectValid(iObject))
    {
      return Err::Failure;
    }

    auto iter = m_ObjectToCo.find(iObject);
    if (iter != m_ObjectToCo.end())
    {
      return Err::Failure;
    }

    CoHandle coHandle = m_CoState.Alloc();
    CoroutineState<Coroutine>& state = m_CoState.Get(coHandle);
    state.coroutine.~Coroutine();
    new(&state.coroutine) Coroutine(std::move(iState));
    state.baseStep = iStepTime;
    state.pauseCycle = iStartPaused ? 1 : 0;
    m_ObjectToCo.insert(std::make_pair(iObject, coHandle));

    state.coroutine.Start(iWorld, iObject);
    if (!iStartPaused && iStepTime > 0)
    {
      CoroutineSchedule<Coroutine> schedule;
      schedule.stateHandle = coHandle;
      schedule.object = iObject;
      schedule.prevTick = iWorld.GetGameTimeInSec();
      schedule.nextTick = schedule.prevTick + iStepTime;
      schedule.pauseCycle = state.pauseCycle;

      m_Time.push_back(schedule);
      std::push_heap(m_Time.begin(), m_Time.end());
    }

    return Err::Success;
  }

  template <typename Coroutine>
  Err T_CoroutineManager<Coroutine>::RemoveCoroutine(World& iWorld, ObjectHandle iObject)
  {
    auto iter = m_ObjectToCo.find(iObject);
    if (iter != m_ObjectToCo.end())
    {
      CoroutineState<Coroutine>& state = m_CoState.Get(iter->second);
      state.coroutine.Terminate(iWorld, iObject);
      m_CoState.Release(iter->second);
      m_ObjectToCo.erase(iter);

      return Err::Success;
    }
    return Err::Failure;
  }

  template <typename Coroutine>
  void T_CoroutineManager<Coroutine>::Tick(World& iWorld)
  {
    double elapsedGameTime = iWorld.GetGameTimeInSec();
    while (!m_Time.empty() && m_Time.front().nextTick <= elapsedGameTime)
    {
      CoroutineSchedule<Coroutine> curTimer = m_Time.front();
      CoroutineState<Coroutine>* state = m_CoState.TryGet(curTimer.stateHandle);
      std::pop_heap(m_Time.begin(), m_Time.end());
      m_Time.pop_back();
      if (state == nullptr || state->pauseCycle != curTimer.pauseCycle)
      {
        continue;
      }

      double nextTime = Mathd::Max(elapsedGameTime + state->baseStep, std::nextafter(elapsedGameTime, Mathd::MaxReal()));

      CoroutineAPI api;
      if (state->yieldedFor > 0)
      {
        state->yieldedFor = -1;
      }
      state->coroutine.Step(api, iWorld, curTimer.object, elapsedGameTime - curTimer.prevTick);
      if (api.stopRequest)
      {
        m_ToDelete.push_back(curTimer.object);
        nextTime = -1;
      }
      else if (api.pauseRequest)
      {
        state->pauseCycle++;
        state->coroutine.Paused(iWorld, curTimer.object);
        nextTime = -1;
      }
      else if (api.yieldRequest &&
        *api.yieldRequest > 0)
      {
        state->yieldedFor = *api.yieldRequest;
        nextTime = Mathd::Max(elapsedGameTime + state->yieldedFor, std::nextafter(elapsedGameTime, Mathd::MaxReal()));
      }

      // Account for in-callback deletion
      if (nextTime > 0)
      {
        curTimer.prevTick = elapsedGameTime;
        curTimer.nextTick = nextTime;

        m_Time.push_back(curTimer);
        std::push_heap(m_Time.begin(), m_Time.end());
      }
    }
  }

  template <typename Coroutine>
  void T_CoroutineManager<Coroutine>::Cleanup(World& iWorld)
  {
    for (auto obj : m_ToDelete)
    {
      RemoveCoroutine(iWorld, obj);
    }
    m_ToDelete.clear();
  }

  template <typename Coroutine>
  void T_CoroutineManager<Coroutine>::Pause(World& iWorld, ObjectHandle iObject)
  {
    auto iter = m_ObjectToCo.find(iObject);
    if (iter != m_ObjectToCo.end())
    {
      CoroutineState<Coroutine>& state = m_CoState.Get(iter->second);
      if (state.pauseCycle % 2 == 0)
      {
        state.coroutine.Paused(iWorld, iObject);
        state.pauseCycle++;
      }
    }
  }

  template <typename Coroutine>
  void T_CoroutineManager<Coroutine>::Resume(World& iWorld, ObjectHandle iObject)
  {
    auto iter = m_ObjectToCo.find(iObject);
    if (iter != m_ObjectToCo.end())
    {
      CoroutineState<Coroutine>& state = m_CoState.Get(iter->second);
      if (state.pauseCycle %2 == 1)
      {
        state.coroutine.Resume(iWorld, iObject);
        state.pauseCycle++;

        CoroutineSchedule<Coroutine> schedule;
        schedule.stateHandle = iter->second;
        schedule.object = iObject;
        schedule.prevTick = iWorld.GetGameTimeInSec();
        schedule.nextTick = schedule.prevTick + state.baseStep;
        schedule.pauseCycle = state.pauseCycle;

        m_Time.push_back(schedule);
        std::push_heap(m_Time.begin(), m_Time.end());
      }
    }
  }

  template<typename Coroutine>
  class T_ComponentCoroutineManager : public ComponentManager, public T_CoroutineManager<Coroutine>
  {
  public:

    void AddCoroutine(ObjectHandle iObject, Coroutine iState, float iStepTime = std::numeric_limits<float>::min(), bool iStartPaused = false)
    {
      T_CoroutineManager<Coroutine>::AddCoroutine(*m_World, iObject, std::move(iState), iStepTime, iStartPaused);
      ComponentManager::CreateComponent(iObject);
    }

    void DeleteComponent(ObjectHandle iObj) override
    {
      T_CoroutineManager<Coroutine>::RemoveCoroutine(*m_World, iObj);
      ComponentManager::DeleteComponent(iObj);
    }

    void Pause(ObjectHandle iObject)
    {
      T_CoroutineManager<Coroutine>::Pause(*m_World, iObject);
    }

    void Resume(ObjectHandle iObject)
    {
      T_CoroutineManager<Coroutine>::Resume(*m_World, iObject);
    }

    void Tick()
    {
      T_CoroutineManager<Coroutine>::Tick(*m_World);
      for (auto toDelete : this->m_ToDelete)
      {
        DeleteComponent(toDelete);
      }
      this->m_ToDelete.clear();
    }

  };
}