/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

namespace eXl
{
#define SERIALIZE_METHODS \
public: \
  Err Stream(Streamer& iStreamer) const; \
  Err Unstream(Unstreamer& iStreamer); \
private: \
  Err Serialize(Serializer iSerializeer)

#define IMPLEMENT_SERIALIZE_METHODS(Class) \
  Err Class::Stream(Streamer& iStreamer) const \
  {return const_cast<Class*>(this)->Serialize(Serializer(iStreamer));} \
  Err Class::Unstream(Unstreamer& iStreamer) \
  {return Serialize(Serializer(iStreamer));} \

  class Serializer
  {
  public:

    Serializer(Streamer& iStreamer)
      : m_Streamer(&iStreamer)
    {}
    Serializer(Unstreamer& iStreamer)
      : m_Unstreamer(&iStreamer)
    {}

    template <class T>
    inline Err operator &=(T& iObj)
    {
      if (IsReading())
      {
        return m_Unstreamer->Read(&iObj);
      }
      else
      {
        return m_Streamer->Write(&iObj);
      }
    }

    template <class T>
    inline Err operator &=(T const& iObj)
    {
      eXl_ASSERT_REPAIR_RET(IsWriting(), Err::Error);
      return m_Streamer->Write(&iObj);
    }

    //template <class T, typename std::enable_if<!IsVectorType<T>::s_Value, bool>::type = true>
    //inline void operator &=(T& iObj)
    //{
    //
    //}

    Err PushKey(String const& iKey)
    {
      if (IsReading())
      {
        return m_Unstreamer->PushKey(iKey);
      }
      else
      {
        return m_Streamer->PushKey(iKey);
      }
    }

    Err PopKey()
    {
      if (IsReading())
      {
        return m_Unstreamer->PopKey();
      }
      else
      {
        return m_Streamer->PopKey();
      }
    }

    Err BeginStruct()
    {
      if (IsReading())
      {
        return m_Unstreamer->BeginStruct();
      }
      else
      {
        return m_Streamer->BeginStruct();
      }
    }

    Err EndStruct()
    {
      if (IsReading())
      {
        return m_Unstreamer->EndStruct();
      }
      else
      {
        return m_Streamer->EndStruct();
      }
    }

    template <typename T>
    inline Err HandleMap(T& iMap)
    {
      using entry_type = typename T::value_type;
      using name_type = typename std::remove_const<typename entry_type::first_type>::type;
      using value_type = typename entry_type::second_type;
      if (IsReading())
      {
        Err SequencePresent = m_Unstreamer->BeginSequence();
        if (SequencePresent)
        {
          do
          {
            name_type key;
            m_Unstreamer->BeginStruct();
            m_Unstreamer->PushKey("Key");
            m_Unstreamer->Read(&key);
            m_Unstreamer->PopKey();

            value_type value;
            m_Unstreamer->PushKey("Value");
            m_Unstreamer->Read(&value);
            m_Unstreamer->PopKey();
            m_Unstreamer->EndStruct();

            iMap.insert(std::make_pair(std::move(key), std::move(value)));
          } while (m_Unstreamer->NextSequenceElement());
        }
        return SequencePresent != Err::Error ? Err::Success : SequencePresent;
      }
      else
      {
        Err SequenceStarted = m_Streamer->BeginSequence();
        if (SequenceStarted)
        {
          for (auto const& entry : iMap)
          {
            m_Streamer->BeginStruct();
            m_Streamer->PushKey("Key");
            m_Streamer->Write(&entry.first);
            m_Streamer->PopKey();
            m_Streamer->PushKey("Value");
            m_Streamer->Write(&entry.second);
            m_Streamer->PopKey();
            m_Streamer->EndStruct();
          }
          return m_Streamer->EndSequence();
        }
        return SequenceStarted;
      }
    }

    template <typename T, 
      typename Predicate = std::less<typename T::value_type::first_type>>
    inline Err HandleMapSorted(T& iMap, Predicate iPred = Predicate())
    {
      using entry_type = typename T::value_type;
      using name_type = typename std::remove_const<typename entry_type::first_type>::type;
      using value_type = typename entry_type::second_type;
      if (IsReading())
      {
        Err SequencePresent = m_Unstreamer->BeginSequence();
        if (SequencePresent)
        {
          do
          {
            name_type key;
            m_Unstreamer->BeginStruct();
            m_Unstreamer->PushKey("Key");
            m_Unstreamer->Read(&key);
            m_Unstreamer->PopKey();

            value_type value;
            m_Unstreamer->PushKey("Value");
            m_Unstreamer->Read(&value);
            m_Unstreamer->PopKey();
            m_Unstreamer->EndStruct();

            iMap.insert(std::make_pair(std::move(key), std::move(value)));
          } while (m_Unstreamer->NextSequenceElement());
        }
        return SequencePresent != Err::Error ? Err::Success : SequencePresent;
      }
      else
      {
        Err SequenceStarted = m_Streamer->BeginSequence();
        if (SequenceStarted)
        {
          Vector<entry_type const*> sortedEntries;
          for (auto const& entry : iMap)
          {
            sortedEntries.push_back(&entry);
          }

          std::sort(sortedEntries.begin(), sortedEntries.end(), [&]
          (entry_type const* iEntry1, entry_type const* iEntry2)
            {
              return iPred(iEntry1->first, iEntry2->first);
            });

          for(entry_type const* entry : sortedEntries)
          {
            m_Streamer->BeginStruct();
            m_Streamer->PushKey("Key");
            m_Streamer->Write(&entry->first);
            m_Streamer->PopKey();
            m_Streamer->PushKey("Value");
            m_Streamer->Write(&entry->second);
            m_Streamer->PopKey();
            m_Streamer->EndStruct();
          }
          return m_Streamer->EndSequence();
        }
        return SequenceStarted;
      }
    }

    template <typename T,
      typename Predicate = std::less<T>>
      inline Err HandleArraySorted(Vector<T> const& iArray, Predicate iPred = Predicate())
    {
      eXl_ASSERT_REPAIR_RET(IsWriting(), Err::Error);

      Vector<T const*> sortedValues;
      for (auto const& value : iArray)
      {
        sortedValues.push_back(&value);
      }

      std::sort(sortedValues.begin(), sortedValues.end(), [&]
      (T const* iVal1, T const* iVal2)
        {
          return iPred(*iVal1, *iVal2);
        });

      for (T const* value : sortedValues)
      {
        m_Streamer->Write(value);
      }

      return Err::Success;
    }

    template <typename T,
      typename Predicate = std::less<T>>
    inline Err HandleArraySorted(Vector<T>& iArray, Predicate iPred = Predicate())
    {
      if (IsReading())
      {
        return m_Unstreamer->Read(&iArray);
      }
      else
      {
        return HandleArraySorted(const_cast<Vector<T> const&>(iArray), iPred);
      }
    }

    template <class T, typename FunctorRead, typename FunctorWrite>
    inline Err HandleSequence(T& iObj, FunctorRead const& iFunR, FunctorWrite const& iFunW)
    {
      if (IsReading())
      {
        Err ArrayPresent = m_Unstreamer->BeginSequence();
        if (ArrayPresent)
        {
          do
          {
            iFunR(iObj, *m_Unstreamer);
          } while (m_Unstreamer->NextSequenceElement());
        }
        return ArrayPresent != Err::Error ? Err::Success : ArrayPresent;
      }
      else
      {
        Err SequenceStarted = m_Streamer->BeginSequence();
        if (SequenceStarted)
        {
          for (auto const& obj : iObj)
          {
            iFunW(obj, *m_Streamer);
          }
          return m_Streamer->EndSequence();
        }
        return SequenceStarted;
      }
    }

    template <class T, typename FunctorRead, typename FunctorWrite>
    inline Err HandleSequence(T const& iObj, FunctorRead const& iFunR, FunctorWrite const& iFunW)
    {
      eXl_ASSERT_REPAIR_RET(IsWriting(), Err::Error);
      
      Err SequenceStarted = m_Streamer->BeginSequence();
      if (SequenceStarted)
      {
        for (auto const& obj : iObj)
        {
          iFunW(obj, *m_Streamer);
        }
        return m_Streamer->EndSequence();
      }
      return SequenceStarted;
    }

    Streamer* GetStreamer() const { return m_Streamer; }
    Unstreamer* GetUnstreamer() const { return m_Unstreamer; }

    bool IsReading() const { return m_Unstreamer != nullptr; }
    bool IsWriting() const { return m_Streamer != nullptr; }

  protected:
    Streamer* m_Streamer = nullptr;
    Unstreamer* m_Unstreamer = nullptr;
  };

  template <typename T>
  Err Serialize(T const& iObj, Serializer iSerializer)
  {
    eXl_ASSERT_REPAIR_RET(iSerializer.IsWriting(), Err::Error);
    iObj.Stream(*iSerializer.GetStreamer());
  }

  //template <typename T>
  //Err Serialize(T& iObj, Serializer iSerializer)
  //{
  //  if (iSerializer.IsWriting())
  //  {
  //    iObj.Stream(*iSerializer.GetStreamer());
  //  }
  //  else
  //  {
  //    iObj.Unstream(*iSerializer.GetUnstreamer());
  //  }
  //}
}
