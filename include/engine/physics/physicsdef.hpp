/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/weakref.hpp>
#include <math/vector3.hpp>
#include <math/quaternion.hpp>
#include <engine/common/world.hpp>
#include <engine/game/archetype.hpp>

namespace eXl
{
  class PhysicComponent;
  class PhysicFlags
  {
  public:
    enum
    {
      Static=1<<0,
      IsGhost=1<<1,
      NeedContactNotify=1<<2,
      FullContactNotify=1<<3 | NeedContactNotify,
      NoGravity=1<<4,
      Kinematic=1<<5,
      IsTrigger=1<<6 | IsGhost,
      AddSensor = 1<<7,
      AlignRotToVelocity = 1<<8,
      Compound = 1<<9,
      MasterCompound = 1<<10,
      LockX = 1<<11,
      LockY = 1<<12,
      LockZ = 1<<13,
      LockRotation = 1<<14,
    };
  };

  struct EXL_ENGINE_API GeomDef
  {
    EXL_REFLECT;

    enum Shape
    {
      Sphere,
      Cylinder,
      Box,
      Ray,
      Capsule
    };

    static inline GeomDef MakeSphere(float iRay,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef ret ={iOrient, iPos, Vector3f(iRay,0.0,0.0), Sphere}; 
      return ret;
    }
    static inline GeomDef MakeCylinder(float iRay,float iH,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef ret ={iOrient, iPos,Vector3f(iRay,iH,0.0), Cylinder}; 
      return ret;
    }
    static inline GeomDef MakeBox(const Vector3f& iDimension,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef ret ={iOrient, iPos, iDimension, Box}; 
      return ret;
    }
    static inline GeomDef MakeCapsule(float iRay,float iH,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef ret ={iOrient, iPos, Vector3f(iRay,iH,0.0), Capsule}; 
      return ret;
    }

    bool operator== (GeomDef const& iOther) const
    {
      return orient == iOther.orient && position == iOther.position
        && geomData == iOther.geomData && shape == iOther.shape;
    }

    Quaternionf orient;
    Vector3f position;
    Vector3f geomData;
    Shape shape;
  };

  inline size_t hash_value(GeomDef const& iDef)
  {
    size_t seed = iDef.shape;
    boost::hash_combine(seed, iDef.orient);
    boost::hash_combine(seed, iDef.position);
    boost::hash_combine(seed, iDef.geomData);

    return seed;
  }

  struct TriggerDef
  {
    TriggerDef();
    GeomDef  m_Geom;
    uint16_t m_Category;
    uint16_t m_Filter;
  };

  struct TriggerCallback
  {
    using ObjectPair = std::pair<ObjectHandle, ObjectHandle>;

    virtual void OnEnter(const Vector<ObjectPair>& NewPairs) {};
    virtual void OnLeave(const Vector<ObjectPair>& NewPairs) {};
  };

  using TriggerCallbackHandle = ObjectTableHandle_Base;

  class EXL_ENGINE_API PhysicInitData
  { 
    EXL_REFLECT;
  public:

    PhysicInitData()
      : flags(0)
      , mass(1.0)
      , m_Category(1)
      , m_Filter(-1)
    {

    }
    
    inline void SetPosition(const Vector3f& iPos){position=iPos;}
    inline void SetOrient(const Quaternionf& iOrient){orient=iOrient;}
    inline void SetMass(float iMass){mass=iMass;}
    inline void SetFlags(unsigned int iFlags){flags=iFlags;}
    inline void SetCategory(short int iCategory,short int iFilter){m_Category=iCategory;m_Filter=iFilter;}
    inline void SetShapeObj(ObjectHandle iObj) { shapeObj = iObj; }
    inline short int GetCategory()const{return m_Category;}
    inline short int GetFilter()const{return m_Filter;}

    void AddSphere(float iRay,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef temp;
      temp.position=iPos;
      temp.orient=iOrient;
      temp.shape = GeomDef::Sphere;
      temp.geomData = Vector3f(iRay,0,0);
      geomList.push_back(temp);
    }

    void AddCylinder(float iRay,float iH,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef temp;
      temp.position=iPos;
      temp.orient=iOrient;
      temp.shape = GeomDef::Cylinder;
      temp.geomData = Vector3f(iRay,iH,0);
      geomList.push_back(temp);
    }

    void AddCapsule(float iRay,float iH,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef temp;
      temp.position=iPos;
      temp.orient=iOrient;
      temp.shape = GeomDef::Capsule;
      temp.geomData = Vector3f(iRay,iH,0);
      geomList.push_back(temp);
    }

    void AddBox(const Vector3f& iDimension,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient = Quaternionf::IDENTITY)
    {
      GeomDef temp;
      temp.position=iPos;
      temp.orient=iOrient;
      temp.shape = GeomDef::Box;
      temp.geomData = iDimension;
      geomList.push_back(temp);
    }

    //void AddHeightField(unsigned char* iData,float high,float low,int X,int Y,float width,float height,bool infinite,const Vector3f& iPos=Vector3f::ZERO,const Quaternionf& iOrient=Quaternionf::IDENTITY);

    inline const Vector3f& GetPosition()const {return position;}
    inline const Quaternionf& GetOrient()const {return orient;}
    inline unsigned int GetFlags()const {return flags;}
    inline float GetMass()const {return mass;}
    inline const SmallVector<GeomDef, 1>& GetGeoms()const {return geomList;}
    inline const ObjectHandle GetShapeObj()const { return shapeObj; }

  protected:

    SmallVector<GeomDef, 1> geomList;
    Quaternionf orient;
    Vector3f position;
    ObjectHandle shapeObj;
    float mass;
    unsigned int flags;
    uint16_t m_Category;
    uint16_t m_Filter;
  };

  struct CollisionData
  {
    ObjectHandle obj1;
    ObjectHandle obj2;
    Vector3f contactPosOn1;
    Vector3f normal1To2;
    float depth;
    //Energy --> 1/2*Mass*RelativeVelocity^2
    float energy;
  };

  using ContactFilterCallback = std::function<bool(ObjectHandle, ObjectHandle)>;

  EXL_REFLECT_ENUM(GeomDef::Shape, eXl__GeomDef__Shape, EXL_ENGINE_API);

  EXL_ENGINE_API GameDataView<PhysicInitData>* GetPhysicsInitDataView(World& iWorld);
}
#include <engine/game/commondef.hpp>

inline eXl::TriggerDef::TriggerDef()
  : m_Category(eXl::EngineCommon::s_TriggerCategory)
  , m_Filter(eXl::EngineCommon::s_TriggerMask)
{

}