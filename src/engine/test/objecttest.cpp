
#include <gtest/gtest.h>

#include <engine/common/object.hpp>
#include <engine/gfx/tileset.hpp>
#include <math/matrix4.hpp>


using namespace eXl;

struct Tree
{
  struct Node
  {
    Node* m_Parent;
    Node* m_Left = nullptr;
    Node* m_Right = nullptr;
    int m_Value;
    int m_Balance = 0;
  };

  Node* Find(int iValue)
  {
    Node* curNode = m_Root;
    while(curNode)
    {
      if(curNode->m_Value == iValue)
      {
        return curNode;
      }

      Node* dir = iValue < curNode->m_Value ? curNode->m_Left : curNode->m_Right;
      curNode = dir;
    }

    return  nullptr;
  }

  void Insert(int iValue)
  {
    Node** curNode = &m_Root;
    Node* prevNode = nullptr;
    while(*curNode)
    {
      Node** dir = iValue < (*curNode)->m_Value ? &(*curNode)->m_Left : &(*curNode)->m_Right;
      prevNode = *curNode;
      curNode = dir;
    }

    *curNode = new Node;
    (*curNode)->m_Parent = prevNode;
    (*curNode)->m_Value = iValue;

    Node* btNode = *curNode;

    while(prevNode)
    {
      if(btNode == prevNode->m_Left)
      {
        if(prevNode->m_Balance < 0)
        {
          if(btNode->m_Balance < 0)
          {
            Rotate(prevNode, btNode);
          }
          else
          {
            Node* newLeft = Rotate(btNode, btNode->m_Right);
            Rotate(prevNode, newLeft);
          }
          break;
        }
        else
        {
          prevNode->m_Balance -= 1;
          if(prevNode->m_Balance == 0)
          {
            break;
          }
        }
      }
      else
      {
        if(prevNode->m_Balance > 0)
        {
          if(btNode->m_Balance > 0)
          {
            Rotate(prevNode, btNode);
          }
          else
          {
            Node* newRight = Rotate(btNode, btNode->m_Left);
            Rotate(prevNode, newRight);
          }
          break;
        }
        else
        {
          prevNode->m_Balance += 1;
          if(prevNode->m_Balance == 0)
          {
            break;
          }
        }
      }

      btNode = prevNode;
      prevNode = prevNode->m_Parent;
    }

    CheckInvariants();
  }

  Node* Rotate(Node* iTree, Node* iPivot)
  {
    Node* treeParent = iTree->m_Parent;
    if(iTree->m_Left == iPivot)
    {
      iTree->m_Left = iPivot->m_Right;
      if(iPivot->m_Right)
      {
        iPivot->m_Right->m_Parent = iTree;
      }
      iPivot->m_Right = iTree;
      iTree->m_Parent = iPivot;
    }
    else
    {
      iTree->m_Right = iPivot->m_Left;
      if(iPivot->m_Left)
      {
        iPivot->m_Left->m_Parent = iTree;
      }
      iPivot->m_Left = iTree;
      iTree->m_Parent = iPivot;
    }

    iPivot->m_Parent = treeParent;
    if(treeParent)
    {
      if(treeParent->m_Left == iTree)
      {
        treeParent->m_Left = iPivot;
      }
      else
      {
        treeParent->m_Right = iPivot;
      }
    }
    else
    {
      m_Root = iPivot;
    }

    //iTree->m_Balance = ;
    //iPivot->m_Balance = ;
    return iPivot;
  }

  unsigned int ComputeHeight(Node* iNode)
  {
    if(!iNode)
    {
      return 0;
    }
    return 1 + Mathi::Max(ComputeHeight(iNode->m_Left), ComputeHeight(iNode->m_Right));
  }

  void CheckInvariants()
  {
    if(!m_Root)
    {
      return;
    }
    Vector<Node*> stack;
    stack.push_back(m_Root);

    while(!stack.empty())
    {
      Node* curNode = stack.back();

      ASSERT_GE(curNode->m_Balance, -1);
      ASSERT_LE(curNode->m_Balance, 1);

      stack.pop_back();
      if(curNode->m_Left)
      {
        ASSERT_TRUE(curNode->m_Left->m_Parent == curNode);
        stack.push_back(curNode->m_Left);
      }
      if(curNode->m_Right)
      {
        ASSERT_TRUE(curNode->m_Right->m_Parent == curNode);
        stack.push_back(curNode->m_Right);
      }
      if(curNode->m_Left)
      {
        ASSERT_GT(curNode->m_Value, curNode->m_Left->m_Value);
      }
      if(curNode->m_Right)
      {
        ASSERT_LE(curNode->m_Value, curNode->m_Right->m_Value);
      }
      ASSERT_EQ(ComputeHeight(curNode->m_Right) - ComputeHeight(curNode->m_Left), curNode->m_Balance);
    }
  }

  Node* m_Root = nullptr;
};

#if 0
TEST(DunAtk, AVLTree)
{
  Tree test;

  Vector<int> numbersToInsert;
  for(uint32_t i = 0; i<100; ++i)
  {
    numbersToInsert.push_back(rand() % 100);
  }

  for(uint32_t i = 0; i<100; ++i)
  {
    test.Insert(numbersToInsert[i]);
  }
}
#endif

TEST(DunAtk, ObjectReg) 
{
  auto objTable = ObjectTable<int>();

  auto newObject1 = objTable.Alloc();
  auto newObject2 = objTable.Alloc();
  auto newObject3 = objTable.Alloc();
  auto newObject4 = objTable.Alloc();

  ASSERT_TRUE(objTable.IsValid(newObject1));
  ASSERT_TRUE(objTable.IsValid(newObject2));
  ASSERT_TRUE(objTable.IsValid(newObject3));
  ASSERT_TRUE(objTable.IsValid(newObject4));

  objTable.Release(newObject2);

  ASSERT_TRUE(objTable.IsValid(newObject1));
  ASSERT_TRUE(newObject2.IsAssigned());
  ASSERT_TRUE(!objTable.IsValid(newObject2));
  ASSERT_TRUE(objTable.IsValid(newObject3));
  ASSERT_TRUE(objTable.IsValid(newObject4));

  auto objRepl2 = objTable.Alloc();

  ASSERT_TRUE(objTable.IsValid(objRepl2));
  ASSERT_TRUE(!objTable.IsValid(newObject2));
  ASSERT_TRUE(newObject2.GetId() == objRepl2.GetId());

  objTable.Release(newObject2);

  ASSERT_TRUE(objTable.IsValid(objRepl2));
  ASSERT_TRUE(!objTable.IsValid(newObject2));

  objTable.Release(newObject3);
  objTable.Release(newObject4);

  ASSERT_TRUE(objTable.IsValid(newObject1));
  ASSERT_TRUE(objTable.IsValid(objRepl2));
  ASSERT_TRUE(!objTable.IsValid(newObject3));
  ASSERT_TRUE(!objTable.IsValid(newObject4));

}

typedef ObjectTable<Matrix4f> MatrixTable;

TEST(DunAtk, ObjectTable)
{
  MatrixTable testTable;

  auto matHandle = testTable.Alloc();
  auto matrixPtr = testTable.TryGet(matHandle);

  ASSERT_TRUE(matrixPtr != nullptr);

  testTable.Release(matHandle);

  ASSERT_TRUE(!testTable.IsValid(matHandle));

  testTable.Reset();

  uint32_t const allocTestSize = 4096;

  Vector<MatrixTable::Handle> handlesToDealloc;

  for(uint32_t i = 0; i< allocTestSize; ++i)
  {
    auto newMat = testTable.Alloc();
    testTable.Get(newMat).m_Data[0] = i;
    if(i%2 == 0)
    {
      handlesToDealloc.push_back(newMat);
    }
  }

  uint32_t countIter = 0;

  auto countObj = [&countIter](Matrix4f const& iMat, MatrixTable::Handle iHandle)
  {
    ASSERT_EQ((uint32_t)iMat.m_Data[0], iHandle.GetId());
    ++countIter;
  };

  testTable.Iterate(countObj);

  ASSERT_EQ(countIter, allocTestSize);
  
  for(auto handle : handlesToDealloc)
  {
    testTable.Release(handle);
  }

  countIter = 0;
  testTable.Iterate(countObj);

  ASSERT_EQ(countIter, allocTestSize / 2);

  for(auto handle : handlesToDealloc)
  {
    auto newMat = testTable.Alloc();
    testTable.Get(newMat).m_Data[0] = newMat.GetId();
  }

  countIter = 0;
  testTable.Iterate(countObj);

  ASSERT_EQ(countIter, allocTestSize);
}

struct AlignedStuff
{
  char bytes[16];
};

#pragma pack(push, 1)
struct alignas(16) ProbObject
{
  AlignedStuff stuff;
  int flag;
};

#pragma pack(pop)

typedef ObjectTable<ProbObject> AlignedAlloc;

TEST(DunAtk, ObjectTableAlign)
{
  AlignedAlloc testAlloc;
  auto handle1 = testAlloc.Alloc();
  auto handle2 = testAlloc.Alloc();
  auto handle3 = testAlloc.Alloc();

  ProbObject* obj1 = testAlloc.TryGet(handle1);
  ProbObject* obj2 = testAlloc.TryGet(handle2);
  ProbObject* obj3 = testAlloc.TryGet(handle3);

  ASSERT_EQ(((ptrdiff_t)obj1) % 16, 0);
  ASSERT_EQ(((ptrdiff_t)obj2) % 16, 0);
  ASSERT_EQ(((ptrdiff_t)obj3) % 16, 0);
}