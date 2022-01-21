/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/mathplugin.hpp>
#include <math/mathexp.hpp>

#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/matrix4.hpp>
#include <math/quaternion.hpp>
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
  Type const* Get_Vector4f_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vector4f>("Vector4f")
        .AddField("X", &Vector4f::m_X)
        .AddField("Y", &Vector4f::m_Y)
        .AddField("Z", &Vector4f::m_Z)
        .AddField("W", &Vector4f::m_W)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vector3f_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vector3f>("Vector3f")
        .AddField("X", &Vector3f::m_X)
        .AddField("Y", &Vector3f::m_Y)
        .AddField("Z", &Vector3f::m_Z)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vector2f_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vector2f>("Vector2f")
        .AddField("X", &Vector2f::m_X)
        .AddField("Y", &Vector2f::m_Y)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vector4i_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vector4i>("Vector4i")
        .AddField("X", &Vector4i::m_X)
        .AddField("Y", &Vector4i::m_Y)
        .AddField("Z", &Vector4i::m_Z)
        .AddField("W", &Vector4i::m_W)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vector3i_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vector3i>("Vector3i")
        .AddField("X", &Vector3i::m_X)
        .AddField("Y", &Vector3i::m_Y)
        .AddField("Z", &Vector3i::m_Z)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Vector2i_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Vector2i>("Vector2i")
        .AddField("X", &Vector2i::m_X)
        .AddField("Y", &Vector2i::m_Y)
        .EndRegistration();
    }();
    return s_Type;
  }

  Type const* Get_Quaternionf_NativeType()
  {
    static Type const* s_Type = []
    {
      return TypeManager::BeginNativeTypeRegistration<Quaternionf>("Quaternionf")
        .AddField("X", &Quaternionf::m_X)
        .AddField("Y", &Quaternionf::m_Y)
        .AddField("Z", &Quaternionf::m_Z)
        .AddField("W", &Quaternionf::m_W)
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

  IMPLEMENT_TYPE(Matrix4f)
  IMPLEMENT_TYPE(AABB2DPolygoni)

  void DeclareMath()
  {
#if EXL_TYPE_ENABLED

    TypeManager::RegisterCoreType<AABB2DPolygoni>();
    TypeManager::RegisterCoreType<Matrix4f>();
    
#endif
  }

#ifdef EXL_LUA

  LUA_REG_FUN(BindMath)
  {
    luabind::module(iState, "eXl")[

      luabind::class_<Vector3f>("Vector3f")
        .def(luabind::constructor<float, float, float>())
        .property("x", &Vector3f::GetX, &Vector3f::SetX)
        .property("y", &Vector3f::GetY, &Vector3f::SetY)
        .property("z", &Vector3f::GetZ, &Vector3f::SetZ)
        .def("Normalize", (float (Vector3f::*)())&Vector3f::Normalize)
        .def("Length", &Vector3f::Length)
        //Trouver pkoi operateur marchent pas.
        .def(luabind::self + Vector3f())
        .def(luabind::self - Vector3f())
        .def(luabind::self * float())
        .def(luabind::self / float())
        ,

        luabind::class_<Vector2f>("Vector2f")
        .def(luabind::constructor<float, float>())
        .property("x", &Vector2f::GetX, &Vector2f::SetX)
        .property("y", &Vector2f::GetY, &Vector2f::SetY)
        .def("Normalize", ((float (Vector2f::*)())&Vector2f::Normalize))
        .def("Length", &Vector2f::Length)
        .def(luabind::self + luabind::other<Vector2f>())
        .def(luabind::self - luabind::other<Vector2f>())
        .def(luabind::self * float())
        .def(luabind::self / float())
        ,

        luabind::class_<Vector2i>("Vector2i")
        .def(luabind::constructor<int, int>())
        .property("x", &Vector2i::GetX, &Vector2i::SetX)
        .property("y", &Vector2i::GetY, &Vector2i::SetY)
        .def(luabind::self + Vector2i())
        .def(luabind::self - Vector2i())
        ,

        luabind::class_<AABB2Df>("AABB2Df")
        .def(luabind::constructor<float, float, float, float>())
        .def(luabind::constructor<Vector2f, Vector2f>())
        .def("AbsorbPoint", (void (AABB2Df::*)(AABB2Df const&))(&AABB2Df::Absorb))
        .def("AbsorbBox", (void (AABB2Df::*)(Vector2f const&))(&AABB2Df::Absorb))
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
        .def(luabind::constructor<Vector2i, Vector2i>())
        .def("AbsorbPoint", (void (AABB2Di::*)(AABB2Di const&))(&AABB2Di::Absorb))
        .def("AbsorbBox", (void (AABB2Di::*)(Vector2i const&))(&AABB2Di::Absorb))
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

        luabind::class_<Quaternionf>("Quaternionf")
        .def(luabind::constructor<float, float, float, float>())
        .def(luabind::constructor<Vector3f const&, float>()),

        luabind::class_<Matrix4f>("Matrix4f")
        .def(luabind::constructor<>())
        .def("MakeIdentity", &Matrix4f::MakeIdentity)
        .scope
        [
          luabind::def("FromPosition", &Matrix4f::FromPosition)
        ]

    ];

    luabind::detail::class_registry* reg = luabind::detail::class_registry::get_registry(iState);

    LuaStateHandle iStateHandle = LuaManager::GetHandle(iState);

    luabind::object _G = luabind::globals(iState);
    _G["eXl"]["Vector3f"]["ZERO"] = Vector3f::ZERO; 
    _G["eXl"]["Vector3f"]["UNIT_X"] = Vector3f::UNIT_X; 
    _G["eXl"]["Vector3f"]["UNIT_Y"] = Vector3f::UNIT_Y; 
    _G["eXl"]["Vector3f"]["UNIT_Z"] = Vector3f::UNIT_Z;
    _G["eXl"]["Vector3f"]["ONE"] = Vector3f::ONE;
    _G["eXl"]["Vector2f"]["ZERO"] = Vector2f::ZERO; 
    _G["eXl"]["Vector2f"]["UNIT_X"] = Vector2f::UNIT_X; 
    _G["eXl"]["Vector2f"]["UNIT_Y"] = Vector2f::UNIT_Y; 
    _G["eXl"]["Vector2f"]["ONE"] = Vector2f::ONE;
    _G["eXl"]["Quaternionf"]["IDENTITY"] = Quaternionf::IDENTITY;
    _G["eXl"]["Mathf"] = luabind::newtable(iState);
    _G["eXl"]["Mathf"]["PI"] = Mathf::PI;

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
