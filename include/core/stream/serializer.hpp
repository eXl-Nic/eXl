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
