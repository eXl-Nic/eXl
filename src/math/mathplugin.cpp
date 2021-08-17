#include <math/mathplugin.hpp>
#include <math/mathexp.hpp>

#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/matrix4.hpp>
#include <math/quaternion.hpp>
#include <math/aabb2d.hpp>

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
  IMPLEMENT_TYPE(Vector4f)
  IMPLEMENT_TYPE(Vector3f)
  IMPLEMENT_TYPE(Vector2f)
  IMPLEMENT_TYPE(Vector4i)
  IMPLEMENT_TYPE(Vector3i)
  IMPLEMENT_TYPE(Vector2i)
  IMPLEMENT_TYPE(Quaternionf)
  IMPLEMENT_TYPE(Matrix4f)

  IMPLEMENT_TAG_TYPE(AABB2Di)
  IMPLEMENT_TAG_TYPE(AABB2Df)

  void DeclareMath()
  {
#if EXL_TYPE_ENABLED

    TypeManager::RegisterCoreType<Vector4i>();
    TypeManager::RegisterCoreType<Vector3i>();
    TypeManager::RegisterCoreType<Vector2i>();
    TypeManager::RegisterCoreType<Matrix4f>();
    TypeManager::RegisterCoreType<Vector4f>();
    TypeManager::RegisterCoreType<Vector3f>();
    TypeManager::RegisterCoreType<Vector2f>();
    TypeManager::RegisterCoreType<Quaternionf>();
    
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
