/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

template <class T>
Handle<T>::Handle():m_LinkPtr(nullptr)
{	    
}	    

template <class T>
Handle<T>::Handle(T* iObj):m_LinkPtr(nullptr)
{
  T::WeakRefLinkType::Set(reinterpret_cast<typename T::WeakRefLinkType*&>(m_LinkPtr),iObj);
  //_Set(iObj);
}

template <class T>
Handle<T>::Handle(Handle<T>&& r)
{
  //Allow r to drop a nullptr ref
  T* test = r.Get();
  m_LinkPtr = r.m_LinkPtr;
  r.m_LinkPtr = nullptr;
}

template <class T>
Handle<T>::Handle(Handle<T> const& r):m_LinkPtr(nullptr)
{
  //Allow r to drop a nullptr ref
  T* test = r.Get();
  T::WeakRefLinkType::Set(reinterpret_cast<typename T::WeakRefLinkType*&>(m_LinkPtr),test);
}

template <class T>
template <class Y>
Handle<T>::Handle(Handle<Y> const& r):m_LinkPtr(nullptr)
{
 //Allow r to drop a nullptr ref and test for inheritance
  T* test = r.Get();
  WeakRefType::Set(m_LinkPtr,test);
  //_Set(test);
}

template <class T>
Handle<T>& Handle<T>::operator=(Handle<T> const& r)
{
  T* test = r.Get();
  T::WeakRefLinkType::Set(reinterpret_cast<typename T::WeakRefLinkType*&>(m_LinkPtr),test);
  //_Set(test);
  return *this;
}

template <class T>
template <class Y>
Handle<T>& Handle<T>::operator=(Handle<Y> const& r)
{
  //m_LinkPtr=nullptr;
  T* test = r.Get();
  T::WeakRefLinkType::Set(reinterpret_cast<typename T::WeakRefLinkType*&>(m_LinkPtr),test);
  //_Set(test);
  return *this;
}

template<class T>
Handle<T>& Handle<T>::operator=(T* r)
{
  T::WeakRefLinkType::Set(reinterpret_cast<typename T::WeakRefLinkType*&>(m_LinkPtr),r);
  //_Set(r);
  return *this;
}

template<class T>
Handle<T>::~Handle()
{
  if(m_LinkPtr != nullptr)
  {
    reinterpret_cast<typename T::WeakRefLinkType*>(m_LinkPtr)->Release();
  }
}

template <class T>
void Handle<T>::Reset()
{
  T::WeakRefLinkType::Set(reinterpret_cast<typename T::WeakRefLinkType*&>(m_LinkPtr),nullptr);
  //_Set(nullptr);
}

template <class T>
void Handle<T>::Set(T * r)
{
  T::WeakRefLinkType::Set(reinterpret_cast<typename T::WeakRefLinkType*&>(m_LinkPtr),r);
  //_Set(r);
}

template <class T>
T* Handle<T>::Get()const
{
  T* res = nullptr;
  if(m_LinkPtr != nullptr)
  {
    res = reinterpret_cast<typename T::WeakRefLinkType*>(m_LinkPtr)->Get();
    if(res == nullptr)
    {
      reinterpret_cast<typename T::WeakRefLinkType*>(m_LinkPtr)->Release();
      m_LinkPtr = nullptr;
    }
  }
  return res;
  //return (T*)_Get();
}

/*template <class T>
T* Handle<T>::GetNoRelease()const
{
  return (T*)IRH_GetNoRelease(m_LinkPtr);
  //return (T*)_GetNoRelease();
}*/

template <class T>
bool Handle<T>::operator==(const Handle& iHandle)const
{
  return Get()==iHandle.Get();
}

template <class T>
bool Handle<T>::operator<(const Handle& iHandle)const
{
  return m_LinkPtr<iHandle.m_LinkPtr;
}

/*template <class T>
void Handle<T>::ResetNoRelease(){
  IRH_ResetNoRelease(m_LinkPtr);
  //_ResetNoRelease();
}*/

/*template <class T>
void Handle<T>::SetNoAddRef(void* iPtr){
  IRH_SetNoAddRef(m_LinkPtr,(RCOIRData*)iPtr);
  //_SetNoAddRef((RCOIRData*)iPtr);
}*/
