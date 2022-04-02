/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/mathplugin.hpp>
#include <math/mathexp.hpp>

#include <math/aabb2d.hpp>
#include <math/aabb2dpolygon.hpp>

#if EXL_TYPE_ENABLED
#include <core/type/typemanager.hpp>
#include <core/type/dynobject.hpp>
#include <core/type/coretype.hpp>
#include <core/type/tagtype.hpp>
#endif

#ifdef EXL_LUA
#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <luabind/out_value_policy.hpp>
#include <core/lua/luamanager.hpp>
#endif

namespace eXl
{
  Type const* Get_Vec4_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vec4>("Vec4")
        .AddField("X", &Vec4::x)
        .AddField("Y", &Vec4::y)
        .AddField("Z", &Vec4::z)
        .AddField("W", &Vec4::w)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vec3_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vec3>("Vec3")
        .AddField("X", &Vec3::x)
        .AddField("Y", &Vec3::y)
        .AddField("Z", &Vec3::z)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vec2_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vec2>("Vec2")
        .AddField("X", &Vec2::x)
        .AddField("Y", &Vec2::y)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vec4i_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vec4i>("Vec4i")
        .AddField("X", &Vec4i::x)
        .AddField("Y", &Vec4i::y)
        .AddField("Z", &Vec4i::z)
        .AddField("W", &Vec4i::w)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vec3i_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vec3i>("Vec3i")
        .AddField("X", &Vec3i::x)
        .AddField("Y", &Vec3i::y)
        .AddField("Z", &Vec3i::z)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vec2i_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vec2i>("Vec2i")
        .AddField("X", &Vec2i::x)
        .AddField("Y", &Vec2i::y)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Quaternion_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Quaternion>("Quaternion")
        .AddField("X", &Quaternion::x)
        .AddField("Y", &Quaternion::y)
        .AddField("Z", &Quaternion::z)
        .AddField("W", &Quaternion::w)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_AABB2Di_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<AABB2Di>("AABB2Di")
        .AddField("Min", &AABB2Di::m_Min)
        .AddField("Max", &AABB2Di::m_Max)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_AABB2Df_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<AABB2Df>("AABB2Df")
        .AddField("Min", &AABB2Df::m_Min)
        .AddField("Max", &AABB2Df::m_Max)
        .EndRegistration();
    }();
    return s_Type;
  }

  IMPLEMENT_TYPE(Mat4)
  IMPLEMENT_TYPE(AABB2DPolygoni)

  void DeclareMath()
  {
#if EXL_TYPE_ENABLED

    TypeManager::RegisterCoreType<AABB2DPolygoni>();
    TypeManager::RegisterCoreType<Mat4>();
    
#endif
  }

#ifdef EXL_LUA

  LUA_REG_FUN(BindMath)
  {
    luabind::module(iState, "eXl")[

      luabind::class_<Vec3>("Vec3")
        .def(luabind::constructor<float, float, float>())
        .def_readwrite("x", &Vec3::x)
        .def_readwrite("y", &Vec3::y)
        .def_readwrite("z", &Vec3::z)
        .def("length", &glm::length<3, float, glm::defaultp>)
        //Trouver pkoi operateur marchent pas.
        .def(luabind::self + Vec3())
        .def(luabind::self - Vec3())
        .def(luabind::self * float())
        .def(luabind::self / float())
        ,

        luabind::class_<Vec2>("Vec2")
        .def(luabind::constructor<float, float>())
        .def_readwrite("x", &Vec2::x)
        .def_readwrite("y", &Vec2::y)
        .def("length", &glm::length<2, float, glm::defaultp>)
        .def(luabind::self + luabind::other<Vec2>())
        .def(luabind::self - luabind::other<Vec2>())
        .def(luabind::self * float())
        .def(luabind::self / float())
        ,

        luabind::class_<Vec2i>("Vec2i")
        .def(luabind::constructor<int, int>())
        .def_readwrite("x", &Vec2i::x)
        .def_readwrite("y", &Vec2i::y)
        .def(luabind::self + Vec2i())
        .def(luabind::self - Vec2i())
        ,

        luabind::class_<AABB2Df>("AABB2Df")
        .def(luabind::constructor<float, float, float, float>())
        .def(luabind::constructor<Vec2, Vec2>())
        .def("AbsorbPoint", (void (AABB2Df::*)(AABB2Df const&))(&AABB2Df::Absorb))
        .def("AbsorbBox", (void (AABB2Df::*)(Vec2 const&))(&AABB2Df::Absorb))
        .def("Contains", &AABB2Df::Contains)
        .def("GetCenter", &AABB2Df::GetCenter)
        .def("GetSize", &AABB2Df::GetSize)
        .def("Intersect", &AABB2Df::Intersect)
        .def("IsInside", &AABB2Df::IsInside)
        .def("SetCommonBox", &AABB2Df::SetCommonBox)
        .def("MinX", (float (AABB2Df::*)()const)(&AABB2Df::MinX))
        .def("MinY", (float (AABB2Df::*)()const)(&AABB2Df::MinY))
        .def("MaxX", (float (AABB2Df::*)()const)(&AABB2Df::MaxX))
        .def("MaxY", (float (AABB2Df::*)()const)(&AABB2Df::MaxY))
        ,

        luabind::class_<AABB2Di>("AABB2Di")
        .def(luabind::constructor<int, int, int, int>())
        .def(luabind::constructor<Vec2i, Vec2i>())
        .def("AbsorbPoint", (void (AABB2Di::*)(AABB2Di const&))(&AABB2Di::Absorb))
        .def("AbsorbBox", (void (AABB2Di::*)(Vec2i const&))(&AABB2Di::Absorb))
        .def("Contains", &AABB2Di::Contains)
        .def("GetCenter", &AABB2Di::GetCenter)
        .def("GetSize", &AABB2Di::GetSize)
        .def("Intersect", &AABB2Di::Intersect)
        .def("IsInside", &AABB2Di::IsInside)
        .def("SetCommonBox", &AABB2Di::SetCommonBox)
        .def("MinX", (int (AABB2Di::*)()const)(&AABB2Di::MinX))
        .def("MinY", (int (AABB2Di::*)()const)(&AABB2Di::MinY))
        .def("MaxX", (int (AABB2Di::*)()const)(&AABB2Di::MaxX))
        .def("MaxY", (int (AABB2Di::*)()const)(&AABB2Di::MaxY))
        ,

        luabind::class_<Quaternion>("Quaternion")
        .def(luabind::constructor<float, float, float, float>()),
        //.def(luabind::constructor<Vec3 const&, float>()),

        luabind::class_<Mat4>("Mat4")
        .def(luabind::constructor<>())

    ];

    luabind::detail::class_registry* reg = luabind::detail::class_registry::get_registry(iState);

    LuaStateHandle iStateHandle = LuaManager::GetHandle(iState);

    //luabind::object _G = luabind::globals(iState);
    //_G["eXl"]["Vec3"]["ZERO"] = Vec3::ZERO; 
    //_G["eXl"]["Vec3"]["UNIT_X"] = Vec3::UNIT_X; 
    //_G["eXl"]["Vec3"]["UNIT_Y"] = Vec3::UNIT_Y; 
    //_G["eXl"]["Vec3"]["UNIT_Z"] = Vec3::UNIT_Z;
    //_G["eXl"]["Vec3"]["ONE"] = Vec3::ONE;
    //_G["eXl"]["Vec2"]["ZERO"] = Vec2::ZERO; 
    //_G["eXl"]["Vec2"]["UNIT_X"] = Vec2::UNIT_X; 
    //_G["eXl"]["Vec2"]["UNIT_Y"] = Vec2::UNIT_Y; 
    //_G["eXl"]["Vec2"]["ONE"] = Vec2::ONE;
    //_G["eXl"]["Quaternionf"]["IDENTITY"] = Quaternionf::IDENTITY;
    //_G["eXl"]["Mathf"] = luabind::newtable(iState);
    //_G["eXl"]["Mathf"]["PI"] = Mathf::Pi();

    return 0;
  }

#endif

  MathPlugin::MathPlugin()
    : Plugin("eXl_Math")
  {

  }

  void MathPlugin::_Load()
  {
    DeclareMath();
#ifdef EXL_LUA
    LuaManager::AddRegFun(&BindMath);
#endif
    //LOG_INFO<<"Loaded Math Plugin"<<"\n";
  }

  PLUGIN_FACTORY(MathPlugin)
}
