/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/rtti.hpp>

namespace eXl
{
  Rtti const& RttiObject::StaticRtti()
  {
    static Rtti s_Rtti("RttiObject", nullptr);
    return s_Rtti;
  }

  Type const* RttiObject::GetType()
  {
    static ClassType s_Class("RttiObject", StaticRtti(), nullptr);

    return &s_Class;
  }

  const Rtti& RttiObject::GetRtti() const
  {
    return StaticRtti();
  }

  Rtti::Rtti(KString name, const Rtti* father) 
    : ClassName(name)
    , fatherRtti(father)//,id(count++)
  {
    //Rtti::GetRttiList().push_back(this);
  }

  /*void Rtti::InitRtti()
    {
    depth=ComputeDepth();
    }*/

  /*unsigned int Rtti::ComputeDepth() const
    {
    return fatherRtti==nullptr ? 0 : fatherRtti->ComputeDepth()+1;
    }*/

  bool Rtti::IsKindOf(const Rtti& Type) const
  {
    if(&Type==this)
    {
      return true;
    }
    if(fatherRtti!=nullptr/* && fatherRtti->GetDepth()>=Type.GetDepth()*/)
    {
      return fatherRtti->IsKindOf(Type);
    }
    return false;
  }

  /*const Rtti* Rtti::CommonAncestor(const Rtti& rtti)const
    {
    const Rtti* mostderived=nullptr;
    const Rtti* lessderived=nullptr;
    if(depth>rtti.depth)
    {
    mostderived=this;
    lessderived=&rtti;
    }
    else
    {
    mostderived=&rtti;
    lessderived=this;
    }
    while(mostderived->depth>lessderived->depth)
    {
    mostderived=mostderived->fatherRtti;
    }
    
    while(mostderived->depth!=0)
    {
    if(mostderived->id==lessderived->id)
    {
    return mostderived;
    }
    mostderived=mostderived->fatherRtti;
    lessderived=lessderived->fatherRtti;
    }
    if(mostderived->id==lessderived->id)
    {
    return mostderived;
    }
    else
    {
    return nullptr;
    }
    }*/
  RttiObject::RttiObject(){}
  RttiObject::~RttiObject(){}

  //RttiObjectRefC::RttiObjectRefC():m_RefCount(0){}
  //RttiObjectRefC::RttiObjectRefC(const RttiObjectRefC& iObj):m_RefCount(0){}
  //
  //IMPLEMENT_RefC(RttiObjectRefC);
  

  //RttiObject* RttiObject::Duplicate() const
  //{
  //  return nullptr;
  //}

}
