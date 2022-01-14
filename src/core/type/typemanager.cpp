/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/typemanager.hpp>
#include <core/type/tupletypestruct.hpp>
#include <core/type/enumtype.hpp>
#include <core/type/arraytype.hpp>
#include <core/type/objectptrtype.hpp>
#include <core/type/classtyperttiobject.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <core/log.hpp>

namespace eXl
{
  namespace TypeManager
  {
    namespace detail
    {

      struct TypeCmpName
      {
        typedef TypeName const result_type;
        const TypeName operator()(const Type* iType) const
        {
          return iType->GetName();
        }
      };
      
      struct TypeCmpID
      {
        typedef size_t result_type;
        size_t operator()(TupleType* iType) const
        {
          return iType->GetTypeId();
        }
      };
      
      typedef boost::multi_index::multi_index_container<Type*,
        boost::multi_index::indexed_by<
          //boost::multi_index::ordered_unique<TypeCmpID>,
          boost::multi_index::ordered_unique<TypeCmpName>,
          boost::multi_index::ordered_unique<boost::multi_index::identity<Type*> >  
        > >TypeMap;

      //typedef TypeMap::nth_index<0>::type TypeMap_by_ID;
      typedef TypeMap::nth_index<0>::type TypeMap_by_Name;
      typedef TypeMap::nth_index<1>::type TypeMap_by_Type;

      //typedef std::pair<const TupleType*,const TupleType*> ViewKey;
      typedef UnorderedMap<Type const*, ArrayType const*> ArrayCacheMap;
      typedef UnorderedMap<std::pair<Type const*, uint32_t>, ArrayType const*> SmallArrayCacheMap;

      struct ClassTypeEntry
      {
        Rtti const* rtti;
        ClassType const* classType;
        void SetPtrType(ObjectPtrType const* iType)
        {
          classPtrType = iType;
        }
        ObjectPtrType const* GetPtrType() const
        {
          return ((flags & 1) == 0) ? classPtrType : nullptr;
        }
        void SetHandleType(ResourceHandleType const* iType)
        {
          resHandleType = iType; 
          flags |= 1;
        }
        ResourceHandleType const* GetHandleType() const
        {
          return ((flags & 1) == 1) ? (ResourceHandleType const*)(*((size_t*)&resHandleType) & ~1) : nullptr;
        }
        union
        {
          unsigned int flags;
          ObjectPtrType const* classPtrType;
          ResourceHandleType const* resHandleType;
        };
      };

      struct ClassCmpRtti
      {
        typedef Rtti const* result_type;
        result_type operator()(ClassTypeEntry const& iEntry) const
        {
          return iEntry.rtti;
        }
      };

      struct ClassCmpParentRtti
      {
        typedef Rtti const* result_type;
        result_type operator()(ClassTypeEntry const& iEntry) const
        {
          return iEntry.rtti->GetFather();
        }
      };

      struct ClassCmpName
      {
        typedef KString const& result_type;
        result_type operator()(ClassTypeEntry const& iEntry) const
        {
          return iEntry.rtti->GetName();
        }
      };

      typedef boost::multi_index::multi_index_container<ClassTypeEntry,
        boost::multi_index::indexed_by<
          boost::multi_index::ordered_unique<ClassCmpRtti>
         ,boost::multi_index::ordered_non_unique<ClassCmpParentRtti>
         ,boost::multi_index::ordered_unique<ClassCmpName>
        >  
        > ClassMap;

      typedef ClassMap::nth_index<0>::type ClassMap_by_Rtti;
      typedef ClassMap::nth_index<1>::type ClassMap_by_ParentRtti;
      typedef ClassMap::nth_index<2>::type ClassMap_by_Name;

      static const size_t UsrTypeFlag = size_t(1)<<(sizeof(size_t)*8-1);

    }

    struct TMData
    {
      detail::TypeMap       m_TypeMap;
      detail::ArrayCacheMap m_ArrayMap;
      detail::SmallArrayCacheMap m_SmallArrayMap;
      detail::ClassMap      m_ClassMap;

      void Clear()
      {
        m_TypeMap.clear();
        for (auto Type : m_ArrayMap)
        {
          eXl_DELETE Type.second;
        }
        m_ArrayMap.clear();
        m_ClassMap.clear();
      }

      static TMData& Get()
      {
        static TMData s_Data;
        return s_Data;
      }
    };


    UsrTypeReg BeginTypeRegistration(TypeName iName)
    {
      return UsrTypeReg(iName);
    }
    
    UsrTypeReg::UsrTypeReg(TypeName iName)
      : m_Name(iName)
      , m_Offset(0)
    {
      
    }

    UsrTypeReg& UsrTypeReg::AddField(TypeFieldName iName,Type const* iType)
    {
      if(iType!=nullptr)
      {
        List<FieldDesc>::iterator iter = m_Fields.begin();
        List<FieldDesc>::iterator iterEnd = m_Fields.end();
        for(;iter!=iterEnd;iter++)
        {
          if(iName==iter->GetName())
          {
            eXl_ASSERT_MSG(iType == iter->GetType(),"Inconsistant");
            return *this;
          }
        }
        m_Fields.push_back(FieldDesc(iName,m_Offset,iType));
        m_Offset+=iType->GetSize();
      }
      return *this;
    }

    UsrTypeReg& UsrTypeReg::AddFieldsFrom(const TupleType* iType)
    {
      if(iType==nullptr)
        return *this;

      unsigned int numFields = iType->GetNumField();
      for(unsigned int i=0;i<numFields;i++)
      {
        bool nextField = false;
        TypeFieldName name;
        const Type* type = iType->GetFieldDetails(i,name);
        List<FieldDesc>::iterator iter = m_Fields.begin();
        List<FieldDesc>::iterator iterEnd = m_Fields.end();
        for(;iter!=iterEnd;iter++)
        {
          if(name==iter->GetName())
          {
            nextField=true;
            break;
          }
        }
        if(nextField)
          continue;
        m_Fields.push_back(FieldDesc(name,m_Offset,type));
        m_Offset+=iType->GetSize();
      }
      return *this;
    }
    
    const TupleType* UsrTypeReg::EndRegistration()
    {
      return MakeTuple(m_Name,m_Fields,nullptr);
    }

    const TupleType* MakeTuple(TypeName iName,const List<FieldDesc>& iList,size_t* iId)
    {
      if(!iList.empty())
      {
        size_t newId = iId==nullptr ? detail::UsrTypeFlag : *iId;
        TupleType* newType = TupleTypeStruct::MakeTuple(iName,iList,newId);
        List<FieldDesc>::const_iterator iter =  iList.begin();
        List<FieldDesc>::const_iterator iterEnd =  iList.end();
        return newType;
      }
      return nullptr;
    }

    const Type* RegisterType(Type const* iType)
    {
      if(iType == nullptr)
        return nullptr;

      eXl_ASSERT_REPAIR_RET(iType->IsCoreType() || iType->IsEnum(), nullptr);

      detail::TypeMap_by_Type::iterator iter = TMData::Get().m_TypeMap.get<1>().find(const_cast<Type*>(iType));
      eXl_ASSERT_MSG(iter == TMData::Get().m_TypeMap.get<1>().end(),"Already registered");
      
      if(iter!=TMData::Get().m_TypeMap.get<1>().end())
      {
        LOG_WARNING<<"Type "<<iType->GetName()<<"already registered"<<"\n";
        return *iter;
      }

      TMData::Get().m_TypeMap.insert(const_cast<Type*>(iType));
      return iType;
    }

    const Type* GetCoreTypeFromName(TypeName iName)
    {
      auto const& typeMapByName = TMData::Get().m_TypeMap.get<0>();
      auto iter = typeMapByName.find(iName);
      if (iter != typeMapByName.end())
      {
        return *iter;
      }
      return nullptr;
    }

    Vector<Type const*> GetCoreTypes()
    {
      Vector<Type const*> coreTypes;
      for (auto const& entry : TMData::Get().m_TypeMap)
      {
        if (entry->IsCoreType())
        {
          coreTypes.push_back(entry);
        }
      }

      return coreTypes;
    }

    TypeManager::EnumTypeReg::EnumTypeReg(TypeName iName)
      : m_Name(iName)
    {
      
    }

    TypeManager::EnumTypeReg& TypeManager::EnumTypeReg::AddValue(TypeEnumName iName)
    {
      for(unsigned int i = 0;i<m_Enums.size();++i)
      {
        if(m_Enums[i] == iName)
        {
          LOG_WARNING<<"Enum "<<iName<<" already exists"<<"\n";
          return *this;
        }
      }

      m_Enums.push_back(iName);
      return *this;
    }

    namespace detail
    {

      EnumType const* _MakeEnumType(TypeName iName, Vector<TypeEnumName> & iVal)
      {
        //size_t newId = ++TMData::Get().m_IDGen;
        EnumType* newType = eXl_NEW EnumType(iName,0);
        newType->m_Enums.swap(iVal);
        Type const* res = TypeManager::RegisterType(newType);
        if(res == nullptr)
        {
          //TMData::Get().m_IDGen--;
          eXl_DELETE newType;
          return nullptr;
        }
        return newType;
      }
    }

    Type const* EnumTypeReg::EndRegistration()
    {
      if(m_Name != "" && m_Enums.size() > 0)
      {        
        return detail::_MakeEnumType(m_Name,m_Enums);
      }else
      {
        LOG_WARNING<<"Invalid Enum definition"<<"\n";
        return nullptr;
      }
    }

    EnumTypeReg BeginEnumTypeRegistration(TypeName iName)
    {
      return EnumTypeReg(iName);
    }

    ArrayType const* GetArrayType(Type const* iType)
    {
      if(iType != nullptr)
      {
        detail::ArrayCacheMap::iterator iter = TMData::Get().m_ArrayMap.find(iType);
        if(iter == TMData::Get().m_ArrayMap.end())
        {
          return nullptr;
        }
        else
        {
          return iter->second;
        }
      }
      return nullptr;
    }

    void RegisterArrayType(ArrayType const* iType)
    {
      if (iType != nullptr)
      {
        eXl_ASSERT_REPAIR_RET(iType->IsCoreType(), );
        detail::ArrayCacheMap::iterator iter = TMData::Get().m_ArrayMap.find(iType);
        if (iter == TMData::Get().m_ArrayMap.end())
        {
          TMData::Get().m_ArrayMap.insert(std::make_pair(iType->GetElementType(), iType));
        }
      }
    }


    ArrayType const* GetSmallArrayType(Type const* iType, uint32_t iBufferSize)
    {
      if (iType != nullptr)
      {
        detail::SmallArrayCacheMap::iterator iter = TMData::Get().m_SmallArrayMap.find(std::make_pair(iType, iBufferSize));
        if (iter == TMData::Get().m_SmallArrayMap.end())
        {
          return nullptr;
        }
        else
        {
          return iter->second;
        }
      }
      return nullptr;
    }

    void RegisterSmallArrayType(ArrayType const* iType, uint32_t iBufferSize)
    {
      if (iType != nullptr)
      {
        eXl_ASSERT_REPAIR_RET(iType->IsCoreType(), );
        auto key = std::make_pair(iType->GetElementType(), iBufferSize);
        detail::SmallArrayCacheMap::iterator iter = TMData::Get().m_SmallArrayMap.find(key);
        if (iter == TMData::Get().m_SmallArrayMap.end())
        {
          TMData::Get().m_SmallArrayMap.insert(std::make_pair(key, iType));
        }
      }
    }

    Err ListDerivedClassesForRtti(Rtti const& iRtti, List<Rtti const*>& oList)
    {
      oList.clear();

      List<Rtti const*> tempList;
      List<Rtti const*> tempList2;
      tempList.push_back(&iRtti);
      while(!tempList.empty())
      {
        List<Rtti const*>::iterator iter = tempList.begin();
        List<Rtti const*>::iterator iterEnd = tempList.end();
        for(; iter != iterEnd; ++iter)
        {
          std::pair<detail::ClassMap_by_ParentRtti::iterator,
            detail::ClassMap_by_ParentRtti::iterator> iterRange = TMData::Get().m_ClassMap.get<1>().equal_range(*iter);
          detail::ClassMap_by_ParentRtti::iterator iterEntry = iterRange.first;
          for(; iterEntry != iterRange.second; ++iterEntry)
          {
            tempList2.push_back(iterEntry->rtti);
            oList.push_back(iterEntry->rtti);
          }
        }
        tempList.clear();
        tempList.swap(tempList2);
      }
      RETURN_SUCCESS;
    }

    ResourceHandleType const* GetResourceHandleForRtti(Rtti const& iRtti)
    {
      detail::ClassMap_by_Rtti::iterator iter = TMData::Get().m_ClassMap.get<0>().find(&iRtti);
      if(iter != TMData::Get().m_ClassMap.get<0>().end())
      {
        return iter->GetHandleType();
      }
      return nullptr;
    }

    namespace detail
    {
      void _TMClear()
      {
        TMData::Get().Clear();
      }
    }
  }
}
