#pragma once

#include <core/coredef.hpp>

namespace eXl
{
  template <typename T>
  class PooledList
  {
  public:

    struct ListNode
    {
      ListNode()
      {
        //memset(m_Value, 0xED, sizeof(m_Value));
      }

      void Kill()
      {
        //bool wasAlreadyKilled = true;
        //for(auto byte : m_Value)
        //{
        //  if(byte != 0xED)
        //  {
        //    wasAlreadyKilled = false;
        //  }
        //}
        //
        //eXl_ASSERT(!wasAlreadyKilled);
        ValuePtr()->~T();
        //memset(m_Value, 0xED, sizeof(m_Value));
      }

      void Reset(T&& iValue)
      {
        //bool wasProperlyKilled = true;
        //for(auto byte : m_Value)
        //{
        //  if(byte != 0xED)
        //  {
        //    wasProperlyKilled = false;
        //  }
        //}
        //
        //eXl_ASSERT(wasProperlyKilled);

        new(ValuePtr()) T(std::move(iValue));
        m_Next = -1;
        m_Prev = -1;
      }

      //void Reset(const T& iValue)
      //{
      //  new(ValuePtr()) T(iValue);
      //  m_Next = -1;
      //  m_Prev = -1;
      //}

      T* ValuePtr()
      {
        return reinterpret_cast<T*>(m_Value);
      }

      T const* ValuePtr() const
      {
        return reinterpret_cast<T const*>(m_Value);
      }

      uint8_t m_Value[sizeof(T)];
      int32_t m_Next = -1;
      int32_t m_Prev = -1;
    };

    struct Pool
    {
      void Reserve(uint32_t iNum)
      {
        m_Nodes.reserve(iNum);
        m_FreeIdx.reserve(iNum);
      }

      uint32_t AllocateNode()
      {
        uint32_t newNodeIdx;
        if(m_FreeIdx.empty())
        {
          newNodeIdx = m_Nodes.size();
          m_Nodes.push_back(ListNode());
        }
        else
        {
          newNodeIdx = m_FreeIdx.back();
          m_FreeIdx.pop_back();
        }

        return newNodeIdx;
      }

      void FreeNode(uint32_t iIdx)
      {
        m_Nodes[iIdx].Kill();
        m_FreeIdx.push_back(iIdx);
        //eXl_ASSERT(m_FreeIdx.size() <= m_Nodes.size());
      }

      Vector<ListNode> m_Nodes;
      Vector<uint32_t> m_FreeIdx;;
    };

    PooledList(Pool& iPool)
      : m_Pool(iPool)
    {
    }

    ~PooledList()
    {
      Clear();
    }

    void Clear()
    {
      auto iter = Begin();
      while(iter != End())
      {
        Erase(iter);
        iter = Begin(); 
      }
    }

    PooledList(PooledList const& iList)
      : m_Pool(iList.m_Pool)
    {
      for(auto iter = iList.Begin(); iter != iList.End(); ++iter)
      {
        PushBack(*iter);
      }
    }

    PooledList& operator=(PooledList const& iList)
    {
      this->~PooledList();
      new(this) PooledList(iList);

      return *this;
    }

    PooledList(PooledList&& iList)
      : m_Pool(iList.m_Pool)
    {
      m_Begin = iList.m_Begin; 
      m_Last = iList.m_Last;
      iList.m_Begin = iList.m_Last = -1;
    }

    PooledList& operator=(PooledList&& iList)
    {
      this->~PooledList();
      new(this) PooledList(iList.m_Pool);
      m_Begin = iList.m_Begin; 
      m_Last = iList.m_Last;
      iList.m_Begin = iList.m_Last = -1;

      return *this;
    }

    struct ConstIterator
    {
      friend class PooledList;
    public:

      typedef T const iter_value_t;
      typedef T const& iter_reference_t;
      //iter_difference_t;
      typedef T const&& iter_rvalue_reference_t;


      ConstIterator(PooledList<T> const& iList, int32_t iCur)
        : m_List(iList)
        , m_Cur(iCur)
      {
      }

      ConstIterator(ConstIterator const& iIter)
        : m_List(iIter.m_List)
        , m_Cur(iIter.m_Cur)
      {
      }

      ConstIterator& operator=(ConstIterator const& iIter)
      {
        this->~ConstIterator();
        new(this) ConstIterator(iIter);

        return *this;
      }

      bool operator ==(ConstIterator const& iIter) const
      {
        //eXl_ASSERT(&m_List == &iIter.m_List);
        return m_Cur == iIter.m_Cur;
      }

      bool operator !=(ConstIterator const& iIter) const
      {
        return !(*this == iIter);
      }

      ConstIterator& operator++()
      {
        ListNode const& node = GetNode();
        m_Cur = node.m_Next;
        return *this;
      }

      ConstIterator operator++(int) const
      {
        ListNode const& node = GetNode();

        ConstIterator iter (m_List, node.m_Next);
        return iter;
      }

      T const& operator*() const
      {
        return *GetNode().ValuePtr();
      }

    private:

      ListNode const& GetNode() const
      {
        return m_List.Nodes()[m_Cur];
      }

      PooledList<T> const& m_List;
      int32_t m_Cur;
    };

    struct Iterator : protected ConstIterator
    {
      friend class PooledList;
    public:

      typedef T  iter_value_t;
      typedef T& iter_reference_t;
      //iter_difference_t;
      typedef T&& iter_rvalue_reference_t;

      Iterator(PooledList<T>& iList, int32_t iCur)
        : ConstIterator(iList, iCur)
      {
      }

      Iterator(Iterator const& iIter)
        : ConstIterator(iIter)
      {
      }

      Iterator& operator=(Iterator const& iIter)
      {
        this->~Iterator();
        new(this) Iterator(iIter);

        return *this;
      }

      bool operator ==(Iterator const& iIter) const
      {
        //eXl_ASSERT(&m_List == &iIter.m_List);
        return this->m_Cur == iIter.m_Cur;
      }

      bool operator !=(Iterator const& iIter) const
      {
        return !(*this == iIter);
      }

      Iterator& operator++()
      {
        ConstIterator::operator++();
        return *this;
      }

      Iterator operator++(int) const
      {
        Iterator iter (*this);
        ++iter;
        return iter;
      }

      T& operator*() const
      {
        return *GetNode().ValuePtr();
      }
    private:
      ListNode& GetNode() const
      {
        return const_cast<PooledList&>(this->m_List).Nodes()[this->m_Cur];
      }
    };

    ConstIterator Begin() const
    {
      return ConstIterator(*this, m_Begin);
    }

    ConstIterator End() const
    {
      return ConstIterator(*this, -1);
    }

    Iterator Begin()
    {
      return Iterator(*this, m_Begin);
    }

    Iterator End()
    {
      return Iterator(*this, -1);
    }

    void PushFront(T iValue)
    {
      uint32_t newNodeIdx = m_Pool.AllocateNode();
      auto& newNode = Nodes()[newNodeIdx];
      newNode.Reset(std::move(iValue));

      if(m_Begin >= 0)
      {
        auto& firstNode = Nodes()[m_Begin];
        firstNode.m_Prev = newNodeIdx;
        newNode.m_Next = &firstNode - Nodes().data();
      }
      else
      {
        m_Last = newNodeIdx;
      }
      m_Begin = newNodeIdx;
    }

    void PushBack(T iValue)
    {
      uint32_t newNodeIdx = m_Pool.AllocateNode();
      auto& newNode = Nodes()[newNodeIdx];
      newNode.Reset(std::move(iValue));

      if(m_Last >= 0)
      {
        auto& lastNode = Nodes()[m_Last];
        lastNode.m_Next = newNodeIdx;
        newNode.m_Prev = &lastNode - Nodes().data();
      }
      else
      {
        m_Begin = newNodeIdx;
      }
      m_Last = newNodeIdx;
    }

    void Insert(Iterator iWhere,  T const& iValue)
    {
      //eXl_ASSERT(&iWhere.m_List == this);
      Insert(ConstIterator(*this, iWhere.m_Cur), iValue);
    }

    void Insert(ConstIterator iWhere,  T iValue)
    {
      //eXl_ASSERT(&iWhere.m_List == this);

      if(iWhere == End())
      {
        PushBack(iValue);
      }
      else
      {
        uint32_t newNodeIdx = m_Pool.AllocateNode();
        auto& newNode = Nodes()[newNodeIdx];
        newNode.Reset(std::move(iValue));

        ListNode& nodeWhere = Nodes()[iWhere.m_Cur];
        ListNode* prevNode = nodeWhere.m_Prev != -1 ? &Nodes()[nodeWhere.m_Prev] : nullptr;

        nodeWhere.m_Prev = newNodeIdx;
        newNode.m_Next = iWhere.m_Cur;

        if(prevNode)
        {  
          prevNode->m_Next = newNodeIdx;
          newNode.m_Prev = prevNode - Nodes().data();
        }
        else
        {
          m_Begin = newNodeIdx;
        }
      }
    }

    void Erase(ConstIterator const& iIter)
    {
      //eXl_ASSERT(&iIter.m_List == this);
      Erase(iIter.m_Cur);
    }

    void Erase(Iterator const& iIter)
    {
      //eXl_ASSERT(&iIter.m_List == this);
      Erase(iIter.m_Cur);
    }

  protected:

    Vector<ListNode> const& Nodes() const
    {
      return m_Pool.m_Nodes;
    }

    Vector<ListNode>& Nodes()
    {
      return m_Pool.m_Nodes;
    }

    void Erase(uint32_t iIdx)
    {
      if(iIdx != -1)
      {
        ListNode& nodeToErase = Nodes()[iIdx];
        ListNode* prevNode = nodeToErase.m_Prev != -1 ? &Nodes()[nodeToErase.m_Prev] : nullptr;
        ListNode* nextNode = nodeToErase.m_Next != -1 ? &Nodes()[nodeToErase.m_Next] : nullptr;

        if(prevNode)
        {
          if(nextNode)
          {
            prevNode->m_Next = nextNode - Nodes().data();
            nextNode->m_Prev = prevNode - Nodes().data();
          }
          else
          {
            prevNode->m_Next = -1;
            m_Last = prevNode - Nodes().data();
          }
        }
        else
        {
          if(nextNode)
          {
            nextNode->m_Prev = -1;
            m_Begin = nextNode - Nodes().data();
          }
          else
          {
            m_Begin = m_Last = -1;
          }
        }

        m_Pool.FreeNode(iIdx);
      }
    }

    Pool& m_Pool;
    int32_t m_Begin = -1;
    int32_t m_Last = -1;
  };
}