
#include <gtest/gtest.h>

#include <core/base/corelib.hpp>
#include <core/lua/luamanager.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/dynobject.hpp>
#include <core/base/log.hpp>

using namespace eXl;

static Type const* s_TypeToRegister = nullptr;
static Type const* s_CompoundType = nullptr;

LUA_REG_FUN(RegisterTest)
{
  if (s_TypeToRegister)
  {
    s_TypeToRegister->RegisterLua(iState);
    s_CompoundType->RegisterLua(iState);

    TypeManager::GetArrayType<int>()->RegisterLua(iState);
  }
  return 0;
}

bool InitCore()
{
  static bool s_Initialized = []
  {
    StartCoreLib(nullptr);
    InitConsoleLog();

    s_TypeToRegister = TypeManager::BeginTypeRegistration("TestType")
      .AddField("TestInt", TypeManager::GetType<int>())
      .AddField("TestString", TypeManager::GetType<AString>())
      .EndRegistration();

    s_CompoundType = TypeManager::BeginTypeRegistration("CompoundType")
      .AddField("TestOther", TypeManager::GetType<float>())
      .AddField("StructField", s_TypeToRegister)
      .AddField("TestOther2", TypeManager::GetType<float>())
      .AddField("TestArray", TypeManager::GetArrayType<int>())
      .EndRegistration();

    LuaManager::AddRegFun(&RegisterTest);

    return true;
  }();

  return s_Initialized;
}

TEST(eXl_Lua, TestCompound)
{
  InitCore();
  LuaManager::Reset();
  
  LuaWorld luaCtx = LuaManager::CreateWorld(nullptr);

  const char* luaScript = R"-(
local testVar = eXl.CompoundType()
--print(testVar)
testVar.StructField.TestInt = 1
testVar.StructField.TestString = eXl.String("Aye")
print(testVar.StructField.TestInt)
testVar.TestOther = 2 + 3
print(testVar.TestOther)
testVar.TestOther2 = 4 + 5
print(testVar.TestOther2)
return testVar
)-";

  luabind::object testObj;
  luaCtx.DoString(luaScript, testObj);

  DynObject objRef = LuaManager::GetObjectRef(testObj, s_CompoundType);
  DynObject fieldRef;

  if (objRef.IsValid())
  {
    LOG_INFO << *objRef.GetField<float>(0) << "\n";
    LOG_INFO << *objRef.GetField<float>(2) << "\n";
  
    objRef.GetField(1, fieldRef);
    if (fieldRef.IsValid())
    {
      LOG_INFO << *fieldRef.GetField<String>(1) << "\n";
    }
  }

  const char* luaScriptFunc = R"-(
local testFun = function(obj)
obj.StructField = eXl.TestType()
obj.StructField.TestInt = 4
obj.StructField.TestString = eXl.String("Oyyy")
end
return testFun
)-";

  luabind::object testFun;
  luaCtx.DoString(luaScriptFunc, testFun);

  auto callTestFN = [&luaCtx, &testFun, &testObj]
  {
    auto luaState = luaCtx.GetState();
    auto luaCallCtx = luaState.PrepareCall(testFun);
    luaCallCtx.Push(testObj);
    luaCallCtx.Call(0);
  };

  callTestFN();
  if (fieldRef.IsValid())
  {
    LOG_INFO << *fieldRef.GetField<String>(1) << "\n";
  }

  DynObject arrayRef;
  objRef.GetField(3, arrayRef);
  if (arrayRef.IsValid())
  {
    ArrayType const* arrayType = ArrayType::DynamicCast(arrayRef.GetType());
    arrayType->SetArraySize(arrayRef.GetBuffer(), 10);
    for (uint32_t i = 0; i < 10; ++i)
    {
      DynObject elemRef;
      arrayRef.GetElement(i, elemRef);
      *elemRef.CastBuffer<int>() = i;
    }
  }

  const char* luaScriptArray = R"-(
local testArrayFun = function(obj)
  return obj.TestArray
end
return testArrayFun
)-";

  const char* luaScriptArray2 = R"-(
local testArrayFun2 = function(array)
print(array[0])
array[3] = 23
array[6] = 26
array[9] = 29
end
return testArrayFun2
)-";

  luabind::object testArrayFun;
  luaCtx.DoString(luaScriptArray, testArrayFun);

  luabind::object testArrayFun2;
  luaCtx.DoString(luaScriptArray2, testArrayFun2);

  luabind::object arrayObj;
  {
    auto luaState = luaCtx.GetState();
    auto luaCallCtx = luaState.PrepareCall(testArrayFun);
    luaCallCtx.Push(testObj);
    luaCallCtx.Call(1);

    arrayObj = luabind::object(luabind::from_stack(luaState.GetState(), -1));
  }

  {
    auto luaState = luaCtx.GetState();
    auto luaCallCtx = luaState.PrepareCall(testArrayFun2);
    luaCallCtx.Push(arrayObj);
    luaCallCtx.Call(0);
  }

  for (uint32_t i = 0; i < 10; ++i)
  {
    DynObject elemRef;
    arrayRef.GetElement(i, elemRef);
    LOG_INFO << "Element " << i << ":" << *elemRef.CastBuffer<int>() <<"\n";
  }
}

TEST(eXl_Lua, Test)
{
  InitCore();
  LuaManager::Reset();

  LuaWorld luaCtx = LuaManager::CreateWorld(nullptr);

  const char* luaScript = R"-(
local testVar = eXl.TestType()
--print(testVar)
testVar.TestInt = 1
print(testVar.TestInt)
testVar.TestInt = 2 + 3
print(testVar.TestInt)
print(testVar.TestString:c_str())
testVar.TestString = eXl.String("Try")
print(testVar.TestString:c_str())
return testVar
)-";

  luabind::object testObj;
  luaCtx.DoString(luaScript, testObj);

  DynObject objRef = LuaManager::GetObjectRef(testObj, s_TypeToRegister);

  if (objRef.IsValid())
  {
    LOG_INFO << *objRef.GetField<int>(0) << "\n";
    LOG_INFO << *objRef.GetField<String>(1) << "\n";
  }

  const char* luaScriptGC = R"-(
  print(collectgarbage("collect"))
print(collectgarbage("count"))
)-";

  luabind::object dummy;
  luaCtx.DoString(luaScriptGC, dummy);

  testObj = luabind::object();
  
  luaCtx.DoString(luaScriptGC, dummy);

  const char* luaScriptFunc = R"-(
local testFun = function (obj)
  obj.TestInt = obj.TestInt + 4
  print(obj.TestInt)
end
return testFun
)-";

  luabind::object testFun;
  luaCtx.DoString(luaScriptFunc, testFun);

  DynObject testObjC;
  testObjC.SetType(s_TypeToRegister, s_TypeToRegister->Build(), true);

  *testObjC.GetField<int>(0) = 2;
  *testObjC.GetField<String>(1) = "TestString";

  testObj = LuaManager::GetLuaRef(luaCtx.GetState().GetState(), testObjC);

  auto callTestFN = [&luaCtx, &testFun, &testObj]
  {
    auto luaState = luaCtx.GetState();
    auto luaCallCtx = luaState.PrepareCall(testFun);
    luaCallCtx.Push(testObj);
    luaCallCtx.Call(0);
  };

  callTestFN();

  luaCtx.DoString(luaScriptGC, dummy);

  callTestFN();

  testObj = luabind::object();

  luaCtx.DoString(luaScriptGC, dummy);

  if (testObjC.IsValid())
  {
    LOG_INFO << *testObjC.GetField<int>(0) << "\n";
    LOG_INFO << *testObjC.GetField<String>(1) << "\n";
  }
}