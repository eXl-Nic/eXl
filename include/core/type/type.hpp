/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once 

#include <core/coredef.hpp>

#include <core/type/typedefs.hpp>

#include <core/type/typetraits.hpp>
#include <core/log.hpp>
#include <vector>

#include <core/rttiobject.hpp>

#ifdef EXL_LUA
#include <luabind/object.hpp>
#include <luabind/back_reference.hpp>
#include <luabind/adopt_policy.hpp>


extern "C" struct lua_State;
#endif

namespace eXl
{
  class DynObject;
  class CoreType;
  class TupleType;

  namespace TypeManager
  {
    class UsrTypeReg;
    template <class T>
    class NativeTypeReg;
    class SignTypeReg;
  }

  /**
     Base class of the reflexion mechanism.
     It's an interface to basic operation on dynamic objects, allocation, destruction and copy.
     Only concrete C++ types can implement this interface. Other types have to be TupleType.
  **********************************************************************/
  class EXL_CORE_API Type : public RttiObject
  {
  public:
    typedef Type TheRttiClass;
    typedef RttiObject ParentRttiClass;
    static Rtti const& StaticRtti();
    static Type const* GetType();
    const Rtti& GetRtti() const override;
    static Type* DynamicCast(::eXl::RttiObject* ptr);
    static const Type* DynamicCast(const ::eXl::RttiObject* ptr);
    //static const unsigned int Type_Rank_Mask=(1<<8)-1;
    static const unsigned int Type_Is_Ordered  = 1<<0;
    static const unsigned int Type_Is_Enum     = 1<<1;
    static const unsigned int Type_Is_CoreType = 1<<2;
    static const unsigned int Type_Is_POD      = 1<<3;
    static const unsigned int Type_Is_TagType  = 1<<4;
  public:
    /**
       Allocate and construct the object.
    **********************************************************************/
    inline void* Build()const
    {
      void* temp = Alloc();
      if(temp)Construct(temp);
      return temp;
    }

    /**
       Destroy and deallocate the object
    **********************************************************************/
    inline void Destroy(void* iObj)const
    {
      if(iObj!=nullptr)
      {
        Destruct(iObj);
        Free(iObj);
      }
    }

    /**
       Allocate the object
    **********************************************************************/
    virtual void* Alloc()const;

    /**
       Deallocate the object
    **********************************************************************/
    virtual void Free(void* iObj)const;

    /**
       Call the object's constructor on an already alloced memory
    **********************************************************************/
    virtual void* Construct(void* iObj)const;
    
    /**
       Call the object's destructor on memory
    **********************************************************************/
    virtual void Destruct(void* iObj)const;

    /**
       Create a copy of the object.
       @param iObj input object which will be copied.
       @return A copy of iObj if iObj was valid and of type this.
    **********************************************************************/
    //virtual DynObject* BuildFromObject(const ConstDynObject* iObj)const =0;

    /**
      Copy iData in oData.
      oData must be a valid memory location, not constructed.
    **********************************************************************/
    virtual Err Copy_Uninit(void const* iData, void* oData) const;

    /**
      If oData is nullptr, allocate the memory. Otherwise, assume that oData is already an instance of the type and destroy it.
    **********************************************************************/
    Err Copy(void const* iData, void*& oData) const;

    
    /**
      
    **********************************************************************/
    virtual Err Unstream_Uninit(void* oData, Unstreamer* iUnstreamer) const;

    /**
      If oData is nullptr, allocate the memory. Otherwise, assume that oData is already an instance of the type and destroy it.
    **********************************************************************/
    Err Unstream(void*& oData, Unstreamer* iUnstreamer) const;

    /**
      
    **********************************************************************/
    virtual Err Stream(void const* iData, Streamer* iStreamer) const;
#ifdef EXL_LUA
    /**
       Convert the object to lua.
       @param iObj object to convert to Lua.
       @param iState lua state which will hold the object.
       @return if iObj was not valid or not of type this, an object which will evaluate to nullptr, the lua counterpart of iObj otherwise.
    **********************************************************************/
    virtual luabind::object ConvertToLua(void const* iObj,lua_State* iState)const;

    /**

    **********************************************************************/
    virtual Err ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const;

    /**
       If oData is nullptr, allocate the memory. Otherwise, assume that oData is already an instance of the type and destroy it.
    **********************************************************************/
    Err ConvertFromLua(luabind::object iObj,void*& oData)const;

    virtual void RegisterLua(lua_State* iState) const;

    virtual luabind::object MakePropertyAccessor(lua_State* iState, Type const* iHolder, uint32_t iOffset) const;
    virtual luabind::object MakeElementAccessor(lua_State* iState, ArrayType const* iHolder) const;
#endif
    virtual Err Compare(void const* iVal1, void const* iVal2, CompRes& oRes) const{return Err::Undefined;}

    /**
       Check whether the current type is a TupleType.(can use dynamiccast now that Types are resources.)
    **********************************************************************/
    virtual const TupleType* IsTuple()const{return nullptr;}

    /**
       Check if we can assign a value described by this type from a value of type iOtherType.
       Default is return iOtherType == this;
    **********************************************************************/
    virtual bool CanAssignFrom(Type const* iOtherType) const;

    /**
       Assign value (must check assignable before.)
       Default is return Copy(iData,oData)
    **********************************************************************/
    Err Assign(Type const* inputType, void const* iData, void* oData) const;

    virtual Err Assign_Uninit(Type const* inputType, void const* iData, void* oData) const;

    inline bool IsOrdered()const{return m_Flags & Type_Is_Ordered;}
    
    inline bool IsEnum()const{return (m_Flags & Type_Is_Enum) != 0;}

    inline bool IsCoreType() const {return (m_Flags & Type_Is_CoreType) != 0;}

    inline bool IsPOD() const {return (m_Flags & Type_Is_POD) != 0;}

    inline bool IsTag() const { return (m_Flags & Type_Is_TagType) != 0; }

    inline size_t GetSize()const{return m_Size;}

    inline size_t GetTypeId()const{return m_TypeId;}
    
    inline TypeName const& GetName() const {return m_Name;}

  protected:

    Type(TypeName iName,
         size_t iTypeId,
         size_t iSize,
         unsigned int iFlags);

    void DoEnable(){};
    void DoDisable(){};

    TypeName m_Name;
    size_t m_TypeId;
    size_t m_Size;
    unsigned int m_Flags = 0;
  };

  //namespace TypeManager
  //{
  //  template<class T>
  //  const CoreType* GetCoreType();
  //}
    
  
}