/*
Copyright 2009-2019 Nicolas Colombe

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
        const TypeName operator()(const Type* iType) const{
          return iType->GetName();
        }
      };
      
      struct TypeCmpID
      {
        typedef size_t result_type;
        size_t operator()(TupleType* iType) const{
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

      void _RegisterInLua(lua_State* iState)
      {
  
      }
    }

    struct TMData
    {
      //Type const*   m_CoreTypes[64];
      //size_t        m_IDGen;
      detail::TypeMap       m_TypeMap;
      detail::ArrayCacheMap m_ArrayMap;
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
        //for(;iter!=iterEnd;iter++)
        //{
        //  newType->AddDependency(iter->GetType());
        //}
        //if(RegisterType(newType,iCont)==nullptr)
        //{
        //  //if(iId==nullptr)
        //  //  TMData::Get().m_IDGen--;
        //  newType->Destroy();
        //  return nullptr;
        //}
        return newType;
      }
      return nullptr;
    }


    //const Type* GetType(const String& iName)
    //{
    //  detail::TypeMap_by_Name::iterator iter = TMData::Get().m_TypeMap.get<1>().find(iName);
    //  if(iter == TMData::Get().m_TypeMap.get<1>().end())
    //  {
    //    return nullptr;
    //  }
    //  return *iter;
    //}

    const Type* GetType(size_t iId)
    {
      //if(iId >0)
      //{
      //  if(iId < 64)
      //  {
      //    return TMData::Get().m_CoreTypes[iId];
      //  }
      //  else
      //  {
      //    detail::TypeMap_by_ID::iterator iter = TMData::Get().m_TypeMap.get<0>().find(iId);
      //    if(iter != TMData::Get().m_TypeMap.get<0>().end())
      //    {
      //      return *iter;
      //    }
      //  }
      //}
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

      //TupleType* signature = TupleType::DynamicCast(iType);
      //size_t typeId = iType->GetTypeId();
      //if(typeId > 0)
      //{
      //  if(typeId < 64)
      //  {
      //    if(TMData::Get().m_CoreTypes[typeId] != nullptr)
      //    {
      //      
      //      return TMData::Get().m_CoreTypes[typeId];
      //    }
      //    else
      //    {
      //      TMData::Get().m_CoreTypes[typeId] = iType;
      //    }
      //  }
      //  else if(/*typeId & StorageFlag &&*/ signature != nullptr)
      //  {
      //    detail::TypeMap_by_ID::iterator iter2 = TMData::Get().m_TypeMap.get<0>().find(iType->GetTypeId());
      //    if(iter2 != TMData::Get().m_TypeMap.get<0>().end())
      //    {
      //      //iCont = nullptr;
      //      signature = *iter2;
      //      iType = signature;
      //      //iType = eXl_NEW SignatureProxy(signature);
      //    }
      //    else
      //    {
      //      //signature->FinishResource(iCont);
      //      //iCont = nullptr;
      //      
      //      TMData::Get().m_TypeMap.insert(signature);
      //    }
      //  }
      //}
     

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

    /*
    const TupleType* GetView(const TupleType* iFrom,const TupleType* iTo){
      detail::TypeViewMap_by_Key::iterator iter = TMData::Get().m_TVMap.get<0>().find(std::make_pair(iFrom,iTo),detail::CompView());
      if(iter==TMData::Get().m_TVMap.get<0>().end()){
        const TupleTypeView* newView = TupleTypeView::MakeView(iFrom,iTo);
        std::pair<detail::TypeViewMap::iterator,bool> res = TMData::Get().m_TVMap.insert(newView);
        eXl_ASSERT_MSG(res.second,"View Map insertion problem");
        return newView;
      }
      return *iter;
    }
    */

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

    SignTypeReg BeginSignatureRegistration()
    {
      return SignTypeReg();
    }

    SignTypeReg::SignTypeReg():m_Offset(0)
    {

    }

    SignTypeReg& SignTypeReg::AddParam(const Type* iType)
    {
      if(m_Fields.size() == 5/*10 en 64 bits*/)
      {
        LOG_WARNING << "Too much parameters" <<"\n";
      }

      TupleType const* tupleType = iType->IsTuple();
      if(tupleType != nullptr && !(tupleType->IsCoreType()))
      {
        LOG_WARNING << "Err : " << iType->GetName() << " is not registered as a Core type" << "\n";
        return *this;
      }
      
      AString fileName("Field");
      StringUtil::FromSizeT(m_Fields.size());

      m_Fields.push_back(FieldDesc(TypeFieldName(fileName.c_str()),m_Offset,iType));
      m_Offset += iType->GetSize();
      return *this;
    }

    const TupleType* SignTypeReg::EndRegistration()
    {
      if(m_Fields.empty())
        return nullptr;

      size_t signType=0;//StorageFlag;
      std::list<FieldDesc>::iterator iter = m_Fields.begin();
      std::list<FieldDesc>::iterator iterEnd = m_Fields.end();
      for(unsigned int i = 0; iter != iterEnd;++i,++iter)
      {
        if(!iter->GetType()->IsCoreType())
        {
          return nullptr;
        }
        
        signType |= iter->GetType()->GetTypeId()<<(6*i);
      }
      TupleType* newType = TupleTypeStruct::MakeTuple(TypeName(AString("Signature")+StringUtil::AFromSizeT(signType)),m_Fields,signType);
      Type const* regType = RegisterType(newType);
      if(regType)
      {
        return regType->IsTuple();
      }
      return nullptr;
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

#if 0
    class ClassTypeRegImpl : public HeapObject
    {
    public:
      inline ClassTypeRegImpl(Rtti const& iRtti, ClassTypeRttiObject::ObjectFactory iObjectFactory)
        :m_Rtti(iRtti)
        ,m_ObjectFactory(iObjectFactory)
      {
      }

      Rtti const& m_Rtti;
      ClassTypeRttiObject::ObjectFactory m_ObjectFactory;
      
    };

    ClassType const* GetClassForName(TypeName iName)
    {
      detail::ClassMap_by_Name::iterator iter = TMData::Get().m_ClassMap.get<2>().find(iName);
      if(iter != TMData::Get().m_ClassMap.get<2>().end())
      {
        return iter->classType;
      }
      return nullptr;
    }

    Err RegisterTransientClassForRtti(Rtti const& iRtti)
    {
      Err err = Err::Failure;
      if(&iRtti != &RttiObject::StaticRtti()
      && &iRtti != &RttiObjectRefC::StaticRtti())
      {
        detail::ClassMap_by_Rtti::iterator iter = TMData::Get().m_ClassMap.get<0>().find(&iRtti);
        if(iter == TMData::Get().m_ClassMap.get<0>().end())
        {
          ClassType const* ParentRttiClass = nullptr;
          if(iRtti.GetFather() != &RttiObject::StaticRtti()
          && iRtti.GetFather() != &RttiObjectRefC::StaticRtti())
          {
            detail::ClassMap_by_Rtti::iterator iter = TMData::Get().m_ClassMap.get<0>().find(iRtti.GetFather());
            if(iter != TMData::Get().m_ClassMap.get<0>().end())
            {
              ParentRttiClass = iter->classType;
              err = Err::Success;
            }
          }
          else
          {
            err = Err::Success;
          }
          if(err)
          {
            detail::ClassTypeEntry newEntry;
            newEntry.rtti = &iRtti;
              
            if(iRtti.IsKindOf(RttiObjectRefC::StaticRtti()))
            {
              ClassType const* transientType = eXl_NEW ClassTypeRttiObject(TypeName("TransientTypeFor"+iRtti.GetName()),iRtti,ParentRttiClass,nullptr);
              newEntry.classType = transientType;

              ObjectPtrType* newPtrType = eXl_NEW ObjectPtrType(iRtti);
              newEntry.SetPtrType(newPtrType);
            }
            //else if(iRtti.IsKindOf(Resource::StaticRtti()))
            //{
            //  ClassType const* transientType = eXl_NEW TransientClassType("TransientTypeFor"+iRtti.GetName(),iRtti,ParentRttiClass);
            //  newEntry.classType = transientType;
            //
            //  ResourceHandleType* newHandleType = eXl_NEW ResourceHandleType(iRtti);
            //  newEntry.SetHandleType(newHandleType);
            //  newHandleType->FinishResource(iCont);
            //}
            else
            {
              err = Err::Failure;
            }
            if(err)
            {
              TMData::Get().m_ClassMap.insert(newEntry);
            }
          }
        }
      }
      return err;
    }

    Err RegisterClassForRtti(Rtti const& iRtti, ClassType const* iType)
    {
      if(&iRtti != &RttiObject::StaticRtti()
        && &iRtti != &RttiObjectRefC::StaticRtti())
      {
        detail::ClassMap_by_Rtti::iterator iter = TMData::Get().m_ClassMap.get<0>().find(&iRtti);
        if(iter == TMData::Get().m_ClassMap.get<0>().end())
        {
          detail::ClassTypeEntry newEntry;
          newEntry.classType = iType;
          newEntry.rtti = &iRtti;
          if(iRtti.IsKindOf(RttiObjectRefC::StaticRtti()))
          {
            ObjectPtrType* newPtrType = eXl_NEW ObjectPtrType(iRtti);
            newEntry.classPtrType = newPtrType;
          }
          //else if(iRtti.IsKindOf(Resource::StaticRtti()))
          //{
          //  ResourceHandleType* newHandleType = eXl_NEW ResourceHandleType(iRtti);
          //  newEntry.SetHandleType(newHandleType);
          //  newHandleType->FinishResource(iCont);
          //}

          TMData::Get().m_ClassMap.insert(newEntry);
          RETURN_SUCCESS;
        }
      }
      RETURN_FAILURE;
    }
#endif
    ClassType const* GetClassForRtti(Rtti const& iRtti)
    {
      detail::ClassMap_by_Rtti::iterator iter = TMData::Get().m_ClassMap.get<0>().find(&iRtti);
      if(iter != TMData::Get().m_ClassMap.get<0>().end())
      {
        return iter->classType;
      }
      return nullptr;
    }

    ObjectPtrType const* GetObjectPtrForRtti(Rtti const& iRtti)
    {
      detail::ClassMap_by_Rtti::iterator iter = TMData::Get().m_ClassMap.get<0>().find(&iRtti);
      if(iter != TMData::Get().m_ClassMap.get<0>().end())
      {
        return iter->GetPtrType();
      }
      return nullptr;
    }

    namespace detail
    {
      void _TMClear()
      {
        TMData::Get().Clear();
      }
#if 0
      ClassTypeRegImpl* BeginClassRegistration(Rtti const& iRtti, RttiObject* (*iObjectFactory)())
      {
        if(&iRtti == &RttiObject::StaticRtti()
          || &iRtti == &RttiObjectRefC::StaticRtti())
        {
          return nullptr;
        }

        detail::ClassMap_by_Rtti::iterator iter = TMData::Get().m_ClassMap.get<0>().find(&iRtti);
        if(iter != TMData::Get().m_ClassMap.get<0>().end())
        {
          return nullptr;
        }

        return eXl_NEW ClassTypeRegImpl(iRtti,iObjectFactory);
      }
#endif
      //void AddProperty(ClassTypeRegImpl* iImpl, Type const* iType, std::string const& iName, void (*GetterFun)(Type const*, void const* iObj, void* oProp), void //(*SetterFun)(Type const*, void* iObj, void const* iProp))
      //{
      //  if(iImpl)
      //  {
      //    iImpl->AddProperty(iType,iName,GetterFun,SetterFun);
      //  }
      //}
#if 0
      ClassType const* EndClassReg(ClassTypeRegImpl* iRegImpl)
      {
        ClassType const* newType = nullptr;
        if(iRegImpl)
        {
          Rtti const* parentRtti = iRegImpl->m_Rtti.GetFather();
          if(parentRtti != nullptr)
          {
            ClassType const* ParentRttiClass = GetClassForRtti(*parentRtti);
            if((ParentRttiClass != nullptr && (ClassTypeRttiObject::DynamicCast(ParentRttiClass) != nullptr) )
            || *parentRtti == RttiObject::StaticRtti()
            || *parentRtti == RttiObjectRefC::StaticRtti())
            {
              ClassType const* newType = eXl_NEW ClassTypeRttiObject(
                TypeName("ClassTypeFor" + iRegImpl->m_Rtti.GetName()),
                iRegImpl->m_Rtti,
                ParentRttiClass,
                //iRegImpl->m_Properties,
                iRegImpl->m_ObjectFactory);

              ClassTypeEntry newEntry;
              newEntry.classType = newType;
              newEntry.rtti = &iRegImpl->m_Rtti;
              if(iRegImpl->m_Rtti.IsKindOf(RttiObjectRefC::StaticRtti()))
              {
                ObjectPtrType* newPtrType = eXl_NEW ObjectPtrType(iRegImpl->m_Rtti);
                newEntry.classPtrType = newPtrType;
              }
              //else if(iRegImpl->m_Rtti.IsKindOf(Resource::StaticRtti()))
              //{
              //  ResourceHandleType* newHandleType = eXl_NEW ResourceHandleType(iRegImpl->m_Rtti);
              //  newEntry.SetHandleType(newHandleType);
              //  newHandleType->FinishResource(iCont);
              //}

              TMData::Get().m_ClassMap.insert(newEntry);
              
            }
          }
          eXl_DELETE iRegImpl;
        }
        return newType;
      }
#endif
    }

  }
}
