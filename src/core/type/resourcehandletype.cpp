/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/resourcehandletype.hpp>
#include <core/type/typemanager.hpp>
#include <core/resource/resource.hpp>
#include <core/lua/luaconverter.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(ResourceHandleType);

  ResourceHandleType::ResourceHandleType()
    :CoreType("ResourceHandleType",
          0,
          sizeof(ResourceHandle<Resource>),
          Type_Is_CoreType)
    ,m_Rtti(Resource::StaticRtti())
  {
    //m_Flags |= Dynamic;
  }

  ResourceHandleType::ResourceHandleType(Rtti const& iRtti)
    : CoreType(TypeName(AString("ResourceHandle_For") + iRtti.GetName().data()),
          0,
      sizeof(ResourceHandle<Resource>),
          Type_Is_CoreType)
    ,m_Rtti(iRtti)
  {
    //m_Flags |= Dynamic;
  }
  
  void* ResourceHandleType::Alloc()const
  {
    return TypeTraits::Alloc<ResourceHandle<Resource>>();
  }

  
  void ResourceHandleType::Free(void* iObj)const
  {
    TypeTraits::Free<ResourceHandle<Resource>>(iObj);
  }

  
  void* ResourceHandleType::Construct(void* iObj)const
  {
    void* obj = TypeTraits::DefaultCTor<ResourceHandle<Resource>>(iObj);
    
    return obj;
  }
  
  void ResourceHandleType::Destruct(void* iObj)const
  {
    TypeTraits::DTor<ResourceHandle<Resource>>(iObj);
  }
  
  Err ResourceHandleType::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer)const
  {
    Resource::UUID ID;
    Err err = UnstreamResourceHandle(ID, m_Rtti, *iUnstreamer);

    if (err)
    {
      TypeTraits::DefaultCTor<ResourceHandle<Resource>>(oData);
      reinterpret_cast<ResourceHandle<Resource>*>(oData)->SetUUID(ID);
    }

    return err;
  }
  
  Err ResourceHandleType::Stream(void const* iData, Streamer* iStreamer)const
  {
    ResourceHandle<Resource> const* handle = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
    
    return StreamResourceHandle(handle->GetUUID(), m_Rtti, *iStreamer);
  }

  Resource::UUID const& ResourceHandleType::GetUUID(void const* iData) const
  {
    static Resource::UUID s_Dummy;
    eXl_ASSERT_REPAIR_RET(iData != nullptr, s_Dummy);

    ResourceHandle<Resource> const* handle = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
    return handle->GetUUID();
  }

  void ResourceHandleType::SetUUID(void* iData, Resource::UUID const& iUUID) const
  {
    eXl_ASSERT_REPAIR_RET(iData != nullptr, );

    ResourceHandle<Resource>* handle = reinterpret_cast<ResourceHandle<Resource>*>(iData);
    handle->SetUUID(iUUID);
  }

  Err ResourceHandleType::Copy_Uninit(void const* iData, void* oData) const
  {
    if (iData == nullptr || oData == nullptr)
    {
      RETURN_FAILURE;
    }
    ResourceHandle<Resource> const* handleOrig = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
    ResourceHandle<Resource>* handleDest = reinterpret_cast<ResourceHandle<Resource>*>(oData);
    new (handleDest) ResourceHandle<Resource>(*handleOrig);
     
    RETURN_SUCCESS;
  }
#ifdef EXL_LUA
  
  luabind::object ResourceHandleType::ConvertToLua(void const* iObj,lua_State* iState)const
  {
    //if(iState==nullptr)
      return luabind::object();
    //return eXl::LuaConverter<RttiOPtr>::ConvertToLua(iObj,this,iState);
  }

  
  Err ResourceHandleType::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    //if(iState==nullptr)
      RETURN_FAILURE;
    //if(oObj==nullptr)
    //  RETURN_FAILURE;
    //
    //eXl::LuaConverter<RttiOPtr>::ConvertFromLua(this,oObj,iState,ioIndex);
    //
    //RETURN_SUCCESS;
  }
#endif
  Err ResourceHandleType::Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const
  {
    if(iVal1 == nullptr || iVal2 == nullptr)
      RETURN_FAILURE;

    ResourceHandle<Resource> const* handle1 = reinterpret_cast<ResourceHandle<Resource> const*>(iVal1);
    ResourceHandle<Resource> const* handle2 = reinterpret_cast<ResourceHandle<Resource> const*>(iVal2);

    oRes = handle1->GetUUID() == handle2->GetUUID() ? CompEqual : CompDifferent;

    RETURN_SUCCESS;
  }

  bool ResourceHandleType::CanAssignFrom(Type const* iOtherType) const
  {
    if(iOtherType == this)
      return true;
    else
    {
      ResourceHandleType const* otherObject = ResourceHandleType::DynamicCast(iOtherType);
      if(otherObject)
      {
        if(otherObject->GetMinimalRtti().IsKindOf(m_Rtti))
        {
          return true;
        }
      }
      else
      {
        return false;
      }
    }
    return false;
  }

  Err ResourceHandleType::Assign_Uninit(Type const* inputType, void const* iData, void* oData) const
  {
    if(iData != nullptr && oData != nullptr && CanAssignFrom(inputType))
    {
      ResourceHandle<Resource> const* sourceHandle = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
      ResourceHandle<Resource>* newHandle = new(oData) ResourceHandle<Resource>;
      newHandle->SetUUID(sourceHandle->GetUUID());
      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }
}