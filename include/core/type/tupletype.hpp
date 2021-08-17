/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/type.hpp>
#include <core/rtti.hpp>
#include <list>

namespace eXl
{
  
  /**
     TupleType class is a specialization of a type for structures. It can handle POD types and contain other TupleType.
  **********************************************************************/
  class EXL_CORE_API TupleType : public Type
  {
    DECLARE_RTTI(TupleType,Type);
    
  public:
    /**
       Check to know if the current type is a tuple type (can use DynamicCast instead, now that types are resources.)
    **********************************************************************/
    const TupleType* IsTuple()const{return this;}

    /**
       Get the number of fields composing the current type.
    **********************************************************************/
    virtual size_t GetNumField()const=0;

    /**
       Get the type of a field.
       @param iNum index of the field in the structure
       @return the type of the field if iNum is a valid index, nullptr otherwise
    **********************************************************************/
    virtual const Type* GetFieldDetails(unsigned int iNum)const=0;

    /**
       Get the type of a field.
       @param iFieldName Field's name.
       @return the type of the field if iFieldName was really a field's name, nullptr otherwise
    **********************************************************************/
    virtual const Type* GetFieldDetails(TypeFieldName iFieldName)const=0;

    /**
       Get the type of a field form its index, and also returns its name.
       @param iNum index of the field in the structure
       @param oFieldName reference to a string which will contain the field's name.
       @return the type of the field if iNum is a valid index, nullptr otherwise
    **********************************************************************/
    virtual const Type* GetFieldDetails(unsigned int iNum,TypeFieldName& oFieldName)const=0;

    /**
       Get the type of a field from its name, and also returns its index.
       @param iFieldName Field's name.
       @param oNumField reference to an unsigned int which will contain the field's index.
       @return the type of the field if iFieldName was really a field's name, nullptr otherwise
    **********************************************************************/
    virtual const Type* GetFieldDetails(TypeFieldName iFieldName,unsigned int& oNumField)const=0;

    virtual Err ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType)const=0;

    /**
       Check if the type is a user type, that is to say a type created through the type manager.
       Types automatically created by the MsgManager are not UserTypes.
    **********************************************************************/
    //inline bool UserType()const{return m_Flags & UserTypeFlag;}
    
    /**
       Project a field contained in a dynamic object into another one.
       The returned object will reference iObj, make sure that iObj outlives the returned object.
       @param iObj dynamic object whose type is this.
       @param iIdx index of the field to access.
       @param oObj pointer to an already created object which will hold the field. If it's nullptr, an newly created object will be returned.
       @return oObj if it was not nullptr and the field was valid, a newly created object if oObj was nullptr and the field valid, nullptr otherwise.
    **********************************************************************/
    virtual void* GetField (void* iObj, unsigned int iIdx, Type const*& oType)const =0;

    /**
       Project a field contained in a dynamic object into another one.
       The returned object will reference iObj, make sure that iObj outlives the returned object.
       @param iObj dynamic object whose type is this.
       @param iIdx name of the field to access.
       @param oObj pointer to an already created object which will hold the field. If it's nullptr, an newly created object will be returned.
       @return oObj if it was not nullptr and the field was valid, a newly created object if oObj was nullptr and the field valid, nullptr otherwise.
    **********************************************************************/
    virtual void* GetField (void* iObj, TypeFieldName iName, Type const*& oType)const =0;
    
    /**
       Const variant of the other accessors.
    **********************************************************************/
    virtual void const* GetField (void const* iObj, unsigned int, Type const*& oType)const =0;

    /**
       Const variant of the other accessors.
    **********************************************************************/
    virtual void const* GetField (void const* iObj, TypeFieldName iName, Type const*& oType)const=0;

    /**
       Convert objects on the Lua stack to an object of type this. The difference with ConvertFromLua is that 
       this method expect all the fields to be on the stack, while ConvertFromLua expects a lua table
       indexed by the field names.
       @param iState Lua state.
       @param ioIndex index on the Lua stack. The method will start reading from ioIndex and will write in ioIndex where it stopped.
       @param oObj pointer to an already created object which will hold the field. If it's nullptr, an newly created object will be returned.
       @return oObj if it was not nullptr and the conversion was a success, a newly created object if oObj was nullptr and the conversion was a success, nullptr otherwise.
    **********************************************************************/
    //virtual DynObject* ConvertFromLuaRaw(lua_State* iState,unsigned int& ioIndex,DynObject* oObj=nullptr)const=0;
#ifdef EXL_LUA
    virtual Err ConvertFromLuaRaw_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const=0;
#endif

    Err ConvertFromLuaRaw(lua_State* iState,unsigned int& ioIndex,void*& oObj) const;

    Err Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const override;

    Err Unstream_Uninit(void* oData, Unstreamer* iUnstreamer) const override;

    Err Stream(void const* iData, Streamer* iStreamer) const override;

    bool CanAssignFrom(Type const* iType) const override;

    Err Assign_Uninit(Type const* inputType, void const* iData, void* oData) const override;

  protected:
    TupleType(TypeName iName,size_t iId,size_t iSize,unsigned int iFlags);
    
  };
}
