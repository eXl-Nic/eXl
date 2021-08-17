/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#if 0 

#ifndef TUPLETYPE_INCLUDED
#include <core/type/tupletype.hpp>
#endif

namespace eXl{
  namespace TypeManager{
    EXL_CORE_API const TupleType* GetView(const TupleType*,const TupleType*);
  }

  /**
     Specialization of TupleType. Its role is to adapt a TupleType to another one having at least all the fields of the other Type.
     It performs an inderection when trying to access the fields, so they can be directly accessed by index instead of having to perform
     queries by name.
  **********************************************************************/
  class TupleTypeView : public TupleType{

    //Purpose : every input is of type m_From and output is of type m_To.
    friend const TupleType* TypeManager::GetView(const TupleType*,const TupleType*);

  public:

    virtual void* Alloc()const{return m_To->Alloc();}

    virtual void Free(void* iObj)const{m_To->Free(iObj);}

    virtual void* Construct(void* iObj)const{return m_To->Construct(iObj);}
    
    virtual void Destruct(void* iObj)const{m_To->Destruct(iObj);}

    /*virtual DynObject* BuildFromObject(const ConstDynObject* iObj)const;

    virtual void CopyInObject(const ConstDynObject* iObj,DynObject* oObj)const ;*/
    
    /*virtual DynObject* BuildFromString(const String& iStr,DynObject* ioObject=nullptr)const{
      eXl_ASSERT_MSG(false,"Not Implemented");
      return nullptr;
    }*/

    //virtual String BuildString(const ConstDynObject* iObject)const;
    
    /*virtual luabind::object ConvertToLua(const ConstDynObject* iObj,lua_State* iState)const;
    
    virtual DynObject* ConvertFromLua(lua_State* iState,unsigned int& ioIndex,DynObject* oObj=nullptr)const;

    virtual DynObject* ConvertFromLuaRaw(lua_State* iState,unsigned int& ioIndex,DynObject* oObj=nullptr)const;*/


    
    void* GetField (void* iObj,unsigned int,Type const*& oType)const;

    void* GetField (void* iObj,const String& iName,Type const*& oType)const;
    
    void const* GetField (void const* iObj,unsigned int,Type const*& oType)const;

    void const* GetField (void const* iObj,const String& iName,Type const*& oType)const;

    virtual const Type* GetFieldDetails(unsigned int iNum,const String*& oFieldName)const{return m_To->GetFieldDetails(iNum,oFieldName);}

    virtual const Type* GetFieldDetails(const String& iFieldName,unsigned int& oNumField)const{return m_To->GetFieldDetails(iFieldName,oNumField);}

    virtual const Type* GetFieldDetails(unsigned int iNum)const{return m_To->GetFieldDetails(iNum);}

    virtual const Type* GetFieldDetails(const String& iFieldName)const{return m_To->GetFieldDetails(iFieldName);}

    virtual size_t GetNumField()const{return m_To->GetNumField();}

    inline const TupleType* GetFrom()const{return m_From;}
    
    inline const TupleType* GetTo()const{return m_To;}
    
  protected:
    
    //iFrom must include all the fields of iTo, otherwise return nullptr.
    static const TupleTypeView* MakeView(const TupleType* iFrom,const TupleType* iTo);

    TupleTypeView(const TupleType* iFrom,const TupleType* iTo);
    
  private:
    const TupleType* m_From;
    const TupleType* m_To;
    std::vector<unsigned int> m_TransTable;
  };
}

#endif
