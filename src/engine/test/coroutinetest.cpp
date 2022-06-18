#include <gtest/gtest.h>

#include <core/corelib.hpp>
#include <core/clock.hpp>
#include <engine/common/world.hpp>
#include <engine/common/coroutine.hpp>
#include <engine/common/gamedatabase.hpp>
#include <engine/game/archetype.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <engine/script/luacoroutine.hpp>

#include <core/utils/capturestack.hpp>

namespace eXl
{
  PropertySheetName GetTestProp()
  {
    static const PropertySheetName s_PropName("TestCounter");
    return s_PropName;
  }

  struct TestStruct
  {
    EXL_REFLECT

    uint32_t m_Counter;
  };

  Type const* TestStruct::GetType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<TestStruct>("eXl::TestStruct")
        .AddField("counter", &TestStruct::m_Counter).EndRegistration();
    }();

    return s_Type;
  }

  static const uint32_t s_PauseMarker = 0xbbbbbbbb;
  static const uint32_t s_DeleteMarker = 0xcccccccc;

  struct DummyState
  {
    DummyState() = default;
    DummyState(bool iYield, bool iStop, bool iMarkPause)
      : m_Yield(iYield)
      , m_Stop(iStop)
      , m_MarkPause(iMarkPause)
    {

    }
    bool m_Yield = false;
    bool m_Stop = false;
    bool m_MarkPause = false;
    uint32_t nativeCounter = -1;

    void Start(World& iWorld, ObjectHandle iObj)
    {
      nativeCounter = 0;
      (*iWorld.GetSystem<GameDatabase>()->ModifyData(iObj, GetTestProp()).CastBuffer<TestStruct>()).m_Counter = nativeCounter;
      Stack st = CaptureStack();
      LOG_INFO << DumpStackInformation(st);
    }
    void Step(CoroutineAPI& iApi, World& iWorld, ObjectHandle iObj, float iTime)
    {
      nativeCounter++;
      (*iWorld.GetSystem<GameDatabase>()->ModifyData(iObj, GetTestProp()).CastBuffer<TestStruct>()).m_Counter = nativeCounter;
      if (m_Yield && nativeCounter > 4)
      {
        iApi.Yield(0.1);
      }
      if (m_Stop && nativeCounter == 4)
      {
        iApi.Stop();
      }
    }
    void Terminate(World& iWorld, ObjectHandle iObj)
    {
      (*iWorld.GetSystem<GameDatabase>()->ModifyData(iObj, GetTestProp()).CastBuffer<TestStruct>()).m_Counter = s_DeleteMarker;
    }
    void Paused(World& iWorld, ObjectHandle iObj)
    {
      if (m_MarkPause)
      {
        (*iWorld.GetSystem<GameDatabase>()->ModifyData(iObj, GetTestProp()).CastBuffer<TestStruct>()).m_Counter = s_PauseMarker;
      }
    }
    void Resume(World& iWorld, ObjectHandle iObj)
    {
      if (m_MarkPause)
      {
        (*iWorld.GetSystem<GameDatabase>()->ModifyData(iObj, GetTestProp()).CastBuffer<TestStruct>()).m_Counter = nativeCounter;
      }
    }
  };

  class DummyCoroutineManager : public T_ComponentCoroutineManager<DummyState>
  {
    DECLARE_RTTI(DummyCoroutineManager, ComponentManager);
  };

  IMPLEMENT_RTTI(DummyCoroutineManager);

  TEST(Coroutine, BaseTest)
  {
    ComponentManifest dummyManifest;
    World world(dummyManifest);

    world.AddSystem(std::make_unique<DummyCoroutineManager>());
    DummyCoroutineManager& mgr = *world.GetSystem<DummyCoroutineManager>();

    PropertiesManifest props;
    props.RegisterPropertySheet(GetTestProp(), TypeManager::GetType<TestStruct>());
    world.AddSystem(std::make_unique<GameDatabase>(props));
    GameDatabase& db = *world.GetSystem<GameDatabase>();

    Archetype* dummyArch = Archetype::Create("", "dummy");
    uint32_t counter = -1;
    ConstDynObject obj(TypeManager::GetType<uint32_t>(), &counter);
    dummyArch->SetProperty(GetTestProp(), obj, true);

    world.Tick();

    ObjectHandle obj1 = world.CreateObject();
    ObjectHandle obj2 = world.CreateObject();
    ObjectHandle obj3 = world.CreateObject();

    db.InstantiateArchetype(obj1, dummyArch, nullptr);
    db.InstantiateArchetype(obj2, dummyArch, nullptr);
    db.InstantiateArchetype(obj3, dummyArch, nullptr);

    volatile uint32_t const* obj1Val = &db.GetData(obj1, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;
    volatile uint32_t const* obj2Val = &db.GetData(obj2, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;
    volatile uint32_t const* obj3Val = &db.GetData(obj3, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;

    ASSERT_EQ(*(obj1Val), -1);
    ASSERT_EQ(*(obj2Val), -1);
    ASSERT_EQ(*(obj3Val), -1);

    mgr.AddCoroutine(obj1, DummyState(false, true, false));
    mgr.AddCoroutine(obj2, DummyState(true, false, false));
    mgr.AddCoroutine(obj3, DummyState(false, false, true), 0.05);

    obj1Val = &db.GetData(obj1, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;
    obj2Val = &db.GetData(obj2, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;
    obj3Val = &db.GetData(obj3, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;

    ASSERT_EQ(*(obj1Val), 0);
    ASSERT_EQ(*(obj2Val), 0);
    ASSERT_EQ(*(obj3Val), 0);

    world.Tick();
    mgr.Tick();

    ASSERT_EQ(*(obj1Val), 1);
    ASSERT_EQ(*(obj2Val), 1);

    world.Tick();
    mgr.Tick();

    ASSERT_EQ(*(obj1Val), 2);
    ASSERT_EQ(*(obj2Val), 2);
    ASSERT_EQ(*(obj3Val), 0);

    mgr.Pause(obj2);
    mgr.Pause(obj3);

    ASSERT_EQ(*(obj3Val), s_PauseMarker);

    world.Tick();
    mgr.Tick();

    ASSERT_EQ(*(obj1Val), 3);
    ASSERT_EQ(*(obj2Val), 2);
    ASSERT_EQ(*(obj3Val), s_PauseMarker);

    mgr.Resume(obj2);
    mgr.Resume(obj3);
    ASSERT_EQ(*(obj3Val), 0);

    world.Tick();
    mgr.Tick();

    ASSERT_EQ(*(obj1Val), s_DeleteMarker);
    ASSERT_EQ(*(obj2Val), 3);
    ASSERT_EQ(*(obj3Val), 0);

    double gameTimeStart = world.GetGameTimeInSec();
    double curGameTime = gameTimeStart;
    while (curGameTime - gameTimeStart < 0.32)
    {
      world.Tick();
      mgr.Tick();

      curGameTime = world.GetGameTimeInSec();
    }

    ASSERT_EQ(*(obj1Val), s_DeleteMarker);
    ASSERT_EQ(*(obj2Val), 8);
    ASSERT_EQ(*(obj3Val), 6);

    mgr.DeleteComponent(obj2);

    ASSERT_EQ(*(obj1Val), s_DeleteMarker);
    ASSERT_EQ(*(obj2Val), s_DeleteMarker);
    ASSERT_EQ(*(obj3Val), 6);
  }

  LUA_REG_FUN(BindTest)
  {
    TestStruct::GetType()->RegisterLua(iState);

    return 0;
  };

  TEST(Coroutine, LuaTest)
  {
    ComponentManifest dummyManifest;
    World world(dummyManifest);

    PropertiesManifest props;
    props.RegisterPropertySheet(GetTestProp(), TypeManager::GetType<TestStruct>());
    world.AddSystem(std::make_unique<GameDatabase>(props));
    GameDatabase& db = *world.GetSystem<GameDatabase>();

    LuaManager::AddRegFun(&BindTest);

    world.AddSystem(std::make_unique<LuaScriptSystem>());
    LuaScriptSystem& scripts = *world.GetSystem<LuaScriptSystem>();

    Archetype* dummyArch = Archetype::Create("", "dummy");
    uint32_t counter = -1;
    ConstDynObject obj(TypeManager::GetType<uint32_t>(), &counter);
    dummyArch->SetProperty(GetTestProp(), obj, true);

    world.Tick();

    ObjectHandle obj1 = world.CreateObject();
    ObjectHandle obj2 = world.CreateObject();
    ObjectHandle obj3 = world.CreateObject();

    db.InstantiateArchetype(obj1, dummyArch, nullptr);
    db.InstantiateArchetype(obj2, dummyArch, nullptr);
    db.InstantiateArchetype(obj3, dummyArch, nullptr);

    LuaCoroutine* luaCo = LuaCoroutine::Create("", "TestLuaCoroutine");

    luaCo->m_Script =
R"(
local module Test_routine = {}

local propName = eXl.Name("TestCounter")

function Test_routine.Start(object)
  local self = {}
  self.data = eXl.AccessProperty(object, propName)
  self.data.counter = 0

  return self
end

function Test_routine.Terminate(self, object)

end

function Test_routine.Paused(self, object)

end

function Test_routine.Resume(self, object)

end

function Test_routine.Step(self, coroutine, object, elapsedTime)
  self.data.counter = self.data.counter + 1
  if self.data.counter > 4 then
    coroutine:Yield(0.1)
  end
end

return Test_routine
)";
    luaCo->m_DefaultTickRate = std::numeric_limits<float>::min();

    scripts.AddCoroutine(obj1, *luaCo);
    scripts.AddCoroutine(obj2, *luaCo);

    volatile uint32_t const* obj1Val = &db.GetData(obj1, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;
    volatile uint32_t const* obj2Val = &db.GetData(obj2, GetTestProp()).CastBuffer<TestStruct>()->m_Counter;


    ASSERT_EQ(*(obj1Val), 0);
    ASSERT_EQ(*(obj2Val), 0);

    world.Tick();
    scripts.Tick();

    ASSERT_EQ(*(obj1Val), 1);
    ASSERT_EQ(*(obj2Val), 1);

    world.Tick();
    scripts.Tick();

    ASSERT_EQ(*(obj1Val), 2);
    ASSERT_EQ(*(obj2Val), 2);
    scripts.PauseCoroutine(obj2);

    world.Tick();
    scripts.Tick();

    ASSERT_EQ(*(obj1Val), 3);
    ASSERT_EQ(*(obj2Val), 2);

    scripts.ResumeCoroutine(obj2);

    world.Tick();
    scripts.Tick();

    ASSERT_EQ(*(obj1Val), 4);
    ASSERT_EQ(*(obj2Val), 3);

    double gameTimeStart = world.GetGameTimeInSec();
    double curGameTime = gameTimeStart;
    while (curGameTime - gameTimeStart < 0.35)
    {
      world.Tick();
      scripts.Tick();

      curGameTime = world.GetGameTimeInSec();
    }

    ASSERT_EQ(*(obj1Val), 8);
    ASSERT_EQ(*(obj2Val), 8);
  }
}
