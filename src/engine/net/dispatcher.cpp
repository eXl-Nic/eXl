
#include <engine/net/network.hpp>

namespace eXl
{

  namespace Network
  {
    void ServerDispatcher::CreateObject(ObjectId iId, ClientData const& iData)
    {
      eXl_ASSERT_REPAIR_RET(m_Objects.count(iId) == 0, void());
      auto insertRes = m_PendingUpdates.insert(std::make_pair(iId, iData));
      if (!insertRes.second)
      {
        insertRes.first->second = iData;
      }
    }

    void ServerDispatcher::UpdateObject(ObjectId iId, ClientData const& iData)
    {
      eXl_ASSERT_REPAIR_RET(m_Objects.count(iId) != 0 ||m_PendingUpdates.count(iId) != 0, void());
      auto insertRes = m_PendingUpdates.insert(std::make_pair(iId, iData));
      if (!insertRes.second)
      {
        insertRes.first->second = iData;
      }
    }

    void ServerDispatcher::DeleteObject(ObjectId iId)
    {

    }

    void ServerDispatcher::AddClient(ClientId iClient, ObjectId iObject)
    {
      eXl_ASSERT_REPAIR_RET(m_ConnectedClients.count(iClient) == 0, void());
      eXl_ASSERT_REPAIR_RET(m_Objects.count(iObject) != 0 || m_PendingUpdates.count(iObject) != 0, void());
      m_NewClients.insert(std::make_pair(iClient, iObject));
    }

    void ServerDispatcher::Flush(Server& iServer)
    {
      for (auto const& update : m_PendingUpdates)
      {
        auto dataIter = m_Objects.find(update.first);
        bool const firstCreation = m_Objects.end() == dataIter;
        for (auto client : m_ConnectedClients)
        {
          if (firstCreation)
          {
            iServer.CreateObject(client.first, update.first, update.second);
          }
          else
          {
            iServer.UpdateObject(client.first, update.first, update.second);
          }
        }
        if (firstCreation)
        {
          m_Objects.insert(update).first;
        }
        else
        {
          dataIter->second = update.second;
        }
      }
      m_PendingUpdates.clear();

      for (auto const& newClient : m_NewClients)
      {
        for (auto const& object : m_Objects)
        {
          iServer.CreateObject(newClient.first, object.first, object.second);
        }
        m_ConnectedClients.insert(newClient);
      }
      m_NewClients.clear();
    }
  }
}