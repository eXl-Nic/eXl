/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/utils/mphf.hpp>
#include <functional>
#include <xxhash.h>

namespace eXl
{

  namespace
  {
    struct Edge
    {
      uint32_t nodeIdx[3];
    };

    struct Node
    {
      uint32_t value;
      Set<uint32_t> edges;
    };
  }

  void MPHF_Data::Clear()
  {
    m_AssignmentTable.clear();
    m_RankTable.clear();
  }

  Err MPHF_Data::Build(uint32_t const* iHashValues, uint32_t iNumKeys)
  {
    // Evaluates the amount of bits needed to store meaningful hash values.
    m_HashLen = [](uint32_t iVal)
    {
      uint32_t POT = 0;
      while (iVal > 0)
      {
        iVal /= 2;
        POT++;
      };

      return POT;
    }(iNumKeys);

    for (uint32_t numTry = 0; numTry < 3; ++numTry, m_HashLen++)
    {
      m_Mask = (1 << m_HashLen) - 1;

      m_AssignmentTable.resize(3 * ((m_Mask + 1) / 32), UINT64_MAX);
      m_RankTable.resize(m_AssignmentTable.size(), 0);

      Vector<Edge> edges;
      edges.resize(iNumKeys);

      UnorderedMap<uint32_t, uint32_t> valueToNode;
      Vector<Node> nodes;
      nodes.reserve(iNumKeys * 3);
      Vector<uint32_t> nodesByDegree;

      // Build the graph. Node == hash value. Edge == values assigned to a given key.
      for (uint32_t i = 0; i < iNumKeys; ++i)
      {
        uint32_t curNodes[3];

        // Build orthogonal hash functions from the given values.
        curNodes[0] = iHashValues[3 * i + 0] & m_Mask;
        curNodes[1] = (iHashValues[3 * i + 1] & m_Mask) + (m_Mask + 1);
        curNodes[2] = (iHashValues[3 * i + 2] & m_Mask) + 2 * (m_Mask + 1);

        for (uint32_t idx = 0; idx < 3; ++idx)
        {
          auto iter = valueToNode.find(curNodes[idx]);
          if (iter == valueToNode.end())
          {
            Node newNode;
            newNode.value = curNodes[idx];
            nodes.push_back(newNode);
            iter = valueToNode.insert(std::make_pair(curNodes[idx], nodes.size() - 1)).first;
            nodesByDegree.push_back(iter->second);
          }
          Node& curNode = nodes[iter->second];
          curNode.edges.insert(i);
          edges[i].nodeIdx[idx] = iter->second;
        }
      }

      auto compareNodeByDegree = [&nodes](uint32_t const& iNode1, uint32_t const& iNode2)
      {
        return nodes[iNode1].edges.size() > nodes[iNode2].edges.size();
      };

      std::sort(nodesByDegree.begin(), nodesByDegree.end(), compareNodeByDegree);

      Vector<uint32_t> sortedEdges;
      for (uint32_t i = 0; i < edges.size(); ++i)
      {
        if (nodesByDegree.empty())
        {
          break;
        }
        Node& curNode = nodes[nodesByDegree.back()];
        if (curNode.edges.size() > 2)
        {
          // Cyclic graph.
          break;
        }

        if (curNode.edges.empty())
        {
          break;
        }

        uint32_t edge = *curNode.edges.begin();
        sortedEdges.push_back(edge);
        for (auto node : edges[edge].nodeIdx)
        {
          nodes[node].edges.erase(edge);
        }
        std::sort(nodesByDegree.begin(), nodesByDegree.end(), compareNodeByDegree);
        while (!nodesByDegree.empty()
          && nodes[nodesByDegree.back()].edges.empty())
        {
          nodesByDegree.pop_back();
        }
      }

      if (sortedEdges.size() != edges.size())
      {
        continue;
      }

      Vector<bool> visited(nodes.size(), false);

      // "Hypergraph peeling" -> assigning a unique value to every key.
      for (uint32_t i = 0; i < edges.size(); ++i)
      {
        uint32_t curEdgeIdx = sortedEdges.back();
        sortedEdges.pop_back();
        Edge& curEdge = edges[curEdgeIdx];
        uint32_t nodeIdx;
        uint32_t nodeHash = 0;
        for (; nodeHash < 3; ++nodeHash)
        {
          nodeIdx = curEdge.nodeIdx[nodeHash];
          if (!visited[nodeIdx])
          {
            break;
          }
        }

        uint32_t nodeValue = nodes[nodeIdx].value;
        uint32_t bucket = nodeValue / 32;
        uint32_t bits = nodeValue % 32;

        int32_t assignment = nodeHash;
        uint32_t testSum = 0;
        for (auto otherNodeIdx : curEdge.nodeIdx)
        {
          if (visited[otherNodeIdx])
          {
            uint32_t otherNodeValue = nodes[otherNodeIdx].value;
            assignment -= GetLabel(otherNodeValue);
            testSum += GetLabel(otherNodeValue);
          }
        }

        assignment = assignment % 3;
        if (assignment < 0)
        {
          assignment += 3;
        }

        uint64_t bitValue = assignment;
        bitValue <<= (2 * bits);

        m_AssignmentTable[bucket] &= ~(3ull << (2 * bits));
        m_AssignmentTable[bucket] |= bitValue;

        for (auto allNodeIdx : curEdge.nodeIdx)
        {
          visited[allNodeIdx] = true;
        }
      }

      uint32_t tot = 0;
      for (uint32_t i = 0; i < m_AssignmentTable.size(); ++i)
      {
        m_RankTable[i] = tot;
        tot += CountBits(m_AssignmentTable[i]);
      }

      return Err::Success;
    }
    return Err::Failure;
  }

  void StringMPH::Hash(KString iStr, uint32_t(&oHashes)[3]) const
  {
    size_t value = boost::hash_value(iStr);

    size_t hash1 = m_Seeds[0];
    boost::hash_combine(hash1, value);

    size_t hash2 = XXH32(iStr.data(), iStr.size(), m_Seeds[1]);
    size_t hash3 = m_Seeds[2] ^ std::hash<KString>()(iStr);

    oHashes[0] = hash1;
    oHashes[1] = hash2;
    oHashes[2] = hash3;
  }

  void StringMPH::UpdateSeeds(Random& iRand)
  {
    m_Seeds[0] = iRand.Generate();
    m_Seeds[1] = iRand.Generate();
    m_Seeds[2] = iRand.Generate();
  }
}