/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <core/type/tupletypeview.hpp>
#if 0
#include <dynobject.hpp>

namespace eXl{
  TupleTypeView::TupleTypeView(const TupleType* iFrom,const TupleType* iTo):
    TupleType(String("View"))+iFrom->GetName()+EXL_TEXT("To"+iTo->GetName(),0,iTo->GetSize(),0),
    m_From(iFrom),m_To(iTo){
    
  }

  const TupleTypeView* TupleTypeView::MakeView(const TupleType* iFrom,const TupleType* iTo){
    std::vector<unsigned int> tempAssoc;
    unsigned int numFields = iTo->GetNumField();
    for(unsigned int i=0;i<numFields;i++){
      const String* namePtr=nullptr;
      const Type* typeTo = iTo->GetFieldDetails(i,namePtr);
      if(typeTo==nullptr){
        LOG_WARNING<<"Err in GetFieldDetail"<<"\n";
        return nullptr;
      }
      unsigned int fieldNumFrom=0;
      const Type* typeFrom = iFrom->GetFieldDetails(*namePtr,fieldNumFrom);
      if(typeFrom!=typeTo){
        if(typeFrom==nullptr)
          LOG_WARNING<<"Field ")<<*namePtr<<EXL_TEXT("in type "<<iTo->GetName()
                     <<" absent from ")<<iFrom->GetName()<<EXL_TEXT("\n";
        else
          LOG_WARNING<<"Unmatched types for type "<<iFrom->GetName()
                     <<" and "<<iTo->GetName()
                     <<" for field ")<<*namePtr<<EXL_TEXT("\n";
        return nullptr;
      }
      tempAssoc.push_back(fieldNumFrom);
    }
    TupleTypeView* resType = eXl_NEW TupleTypeView(iFrom,iTo);
    resType->m_TransTable = tempAssoc;
    resType->AddDependency(iFrom);
    resType->AddDependency(iTo);
    eXl_ASSERT_MSG(resType->FinishResource(),"Problem in finishing resource");
    return resType;
  }

  DynObject* TupleTypeView::BuildFromObject(const ConstDynObject* iObj)const{
    if(iObj->GetType()!=m_From || iObj->GetBuffer()==nullptr) return nullptr;
    DynObject* newObj = eXl_NEW DynObject;
    newObj->SetType(m_To,Build(),true);
    DynObject toBuffer;
    ConstDynObject fromBuffer;
    for(unsigned int i=0;i<m_TransTable.size();i++){
      ConstDynObject* check = m_To->GetField(newObj,i,&toBuffer);
      if(check!=nullptr)
        check=m_From->GetField(iObj,m_TransTable[i],&fromBuffer);
      eXl_ASSERT_MSG(check!=nullptr,"Problem in getting fields");
      fromBuffer.GetType()->CopyInObject(&fromBuffer,&toBuffer);
    }
    return newObj;
  }

  void TupleTypeView::CopyInObject(const ConstDynObject* iObj,DynObject* oObj)const{
    if(iObj==nullptr || iObj->GetType()!=m_From || iObj->GetBuffer()==nullptr) return;
    if(oObj==nullptr || oObj->GetType()!=m_To) return;
    if(oObj->GetBuffer()==nullptr) oObj->SetType(m_To,m_To->Build(),true);

    for(unsigned int i=0;i<m_TransTable.size();i++){
      DynObject toBuffer;
      ConstDynObject fromBuffer;
      ConstDynObject* check = m_To->GetField(oObj,i,&toBuffer);
      if(check!=nullptr)
        check = m_From->GetField(iObj,m_TransTable[i],&fromBuffer);
      eXl_ASSERT_MSG(check!=nullptr,"Problem in getting fields");
      fromBuffer.GetType()->CopyInObject(&fromBuffer,&toBuffer);
    }

  }
  
  luabind::object TupleTypeView::ConvertToLua(const ConstDynObject* iObj,lua_State* iState)const{
    if(iObj==nullptr || iObj->GetType()!=m_From)return luabind::object();
    luabind::object ret = luabind::newtable(iState);
    for(unsigned int i=0;i<m_TransTable.size();i++){
      const unsigned int fieldIdx = m_TransTable[i];
      ConstDynObject field;
      ConstDynObject* res = GetField(iObj,fieldIdx,&field);
      eXl_ASSERT_MSG(res==&field,"Get field failure");
      const String* fieldName=nullptr;
      m_To->GetFieldDetails(fieldIdx,fieldName);
      ret[*fieldName]=field.GetType()->ConvertToLua(&field,iState);
    }
    return ret;
  }
  
  DynObject* TupleTypeView::ConvertFromLua(lua_State* iState,unsigned int& ioIndex,DynObject* oObj)const{
    eXl_ASSERT_MSG(false,"Not Implemented");
    return nullptr;
  }

  DynObject* TupleTypeView::ConvertFromLuaRaw(lua_State* iState,unsigned int& ioIndex,DynObject* oObj)const{
    eXl_ASSERT_MSG(false,"Not Implemented");
    return nullptr;
  }
  
  DynObject* TupleTypeView::GetField (DynObject* iObj,unsigned int iIdx,DynObject* oObj)const{
    if(iObj==nullptr || iObj->GetType()!=m_From || iObj->GetBuffer()==nullptr) return nullptr;
    if(iIdx>=m_TransTable.size())return nullptr;
    const Type* fieldType = m_To->GetFieldDetails(iIdx);
    if(oObj==nullptr){
      oObj = eXl_NEW DynObject(fieldType,nullptr);
    }
    return m_From->GetField(iObj,m_TransTable[iIdx],oObj);
  }
  
  DynObject* TupleTypeView::GetField (DynObject* iObj,const String& iName,DynObject* oObj)const{
    unsigned int index;
    const Type* fieldType = m_To->GetFieldDetails(iName,index);
    if(fieldType!=nullptr)
      return GetField(iObj,index,oObj);
    return nullptr;
  }
  
  ConstDynObject* TupleTypeView::GetField (const ConstDynObject* iObj,unsigned int iIdx,ConstDynObject* oObj)const{
    if(iObj==nullptr || iObj->GetType()!=m_From || iObj->GetBuffer()==nullptr) return nullptr;
    if(iIdx>=m_TransTable.size())return nullptr;
    const Type* fieldType = m_To->GetFieldDetails(iIdx);
    if(oObj==nullptr){
      oObj = eXl_NEW ConstDynObject(fieldType,nullptr);
    }
    return m_From->GetField(iObj,m_TransTable[iIdx],oObj);
  }
  
  ConstDynObject* TupleTypeView::GetField (const ConstDynObject* iObj,const String& iName,ConstDynObject* oObj)const{
    unsigned int index;
    const Type* fieldType = m_To->GetFieldDetails(iName,index);
    if(fieldType!=nullptr)
      return GetField(iObj,index,oObj);
    return nullptr;
  }

  String TupleTypeView::BuildString(const ConstDynObject* iObject)const{
    String res;
    if(iObject->IsValid() && iObject->GetType() == m_From){
      res = "<";
      for(unsigned int i=0;i<m_TransTable.size();i++){
        const unsigned int fieldIdx = m_TransTable[i];
        ConstDynObject field;
        ConstDynObject* fieldRes = GetField(iObject,fieldIdx,&field);
        eXl_ASSERT_MSG(fieldRes==&field,"Get field failure");
        res.append(field.GetType()->BuildString(&field));
        if(i!=(GetNumField()-1))
          res.append(",");
      }
      res = res + ">";
    }else
      LOG_WARNING<<"Bad object"<<"\n";
    return res;
  }
  
}
#endif