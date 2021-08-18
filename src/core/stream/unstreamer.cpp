/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/stream/unstreamer.hpp>
#include <core/rtti.hpp>

namespace eXl
{
  Err Unstreamer::Begin()
  {
    return Err::Success;
  }

  Err Unstreamer::End()
  {
    return Err::Success;
  }

  //template <>
  //Err TypeTraits::Unstream<Handle<Resource const> >(Handle<Resource const>* oObj, Unstreamer* iUnstreamer)
  //{
  //  Err err = Err::Failure;
  //  if(oObj)
  //  {
  //    String value;
  //    iUnstreamer->ReadString(&value);
  //    if(value != "nullptr")
  //    {
  //      ResourceContainer* cont = iUnstreamer->GetLoadingContext()->GetCurrentContainer();
  //      if(cont)
  //      {
  //        String resourceName;
  //        size_t pos = value.rfind(ResourceContainer::GetResourceRef());
  //        if(pos != String::npos)
  //        {
  //          resourceName = value.substr(pos + ResourceContainer::GetResourceRef().size());
  //          size_t prefixPos = value.find(ResourceContainer::GetContainerPrefixSep());
  //          if(prefixPos != String::npos)
  //          {
  //            String prefix = value.substr(0,prefixPos);
  //            unsigned int offset = prefixPos + ResourceContainer::GetContainerPrefixSep().size();
  //            String containerPath = value.substr(offset,(pos - offset));
  //            ResourceContainer* rootContainer = ResourceContainer::GetRoot();
  //            if(prefix == rootContainer->GetHandlerPrefix())
  //            {
  //              cont = rootContainer;
  //            }
  //            else
  //            {
  //              if(containerPath.find(rootContainer->GetName()) == 0)
  //              {
  //                //Absolute path
  //                containerPath = containerPath.substr(rootContainer->GetName().size() + ResourceContainer::GetContainersSep().size());
  //                cont = rootContainer->GetContainerFromPath(prefix, containerPath,nullptr);
  //              }
  //              else
  //              {
  //                //Relative path
  //                cont = cont->GetContainerFromPath(prefix, containerPath,iUnstreamer->GetLoadingContext());
  //              }
  //            }
  //          }
  //        }
  //        else
  //        {
  //          value.swap(resourceName);
  //        }
  //        if(cont != nullptr)
  //        {
  //          Resource const* rsc = nullptr;
  //          if(resourceName.find(Resource::GetDynamicPrefix()) == 0)
  //          {
  //            String regularName = resourceName.substr(Resource::GetDynamicPrefix().size());
  //            unsigned int prefixPos;
  //            while((prefixPos = regularName.find(Resource::GetDynamicPrefix())) != String::npos)
  //            {
  //              regularName.replace(regularName.begin() + prefixPos,regularName.begin() + prefixPos +Resource::GetDynamicPrefix().size(),EXL_TEXT(""));
  //            }
  //            rsc = cont->GetResource(regularName);
  //            if(rsc == nullptr)
  //            {
  //              rsc = ResourceManager::GenerateDynamicResource(cont,resourceName);
  //            }
  //          }
  //          else
  //          {
  //
  //            size_t posId = resourceName.find(ResourceContainer::GetIdentifierPrefix());
  //            if(posId != String::npos)
  //            {
  //              unsigned int Id = StringUtil::ToUInt(resourceName.substr(posId + ResourceContainer::GetIdentifierPrefix().length()));
  //              rsc = cont->GetResource(Id);
  //            }
  //            else
  //            {
  //              rsc = cont->GetResource(resourceName);
  //            }
  //          }
  //          if(rsc)
  //          {
  //            new (oObj) Handle<Resource const>(rsc);
  //            err = Err::Success;
  //          }
  //        }
  //      }
  //    }
  //    else
  //    {
  //      new (oObj) Handle<Resource const>;
  //      err = Err::Success;
  //    }
  //  }
  //  return err;
  //}

  template <>
  Err TypeTraits::Unstream<IntrusivePtr<RttiObjectRefC> >(IntrusivePtr<RttiObjectRefC> * oObj, Unstreamer& iUnstreamer)
  {
    return Err::Undefined;
  }
}