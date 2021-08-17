#pragma once

#include <core/coredef.hpp>
#include <engine/enginelib.hpp>

#include <math/vector3.hpp>
#include <engine/common/world.hpp>

namespace eXl
{
  namespace Network
  {
    enum class NetRole
    {
      None,
      Server,
      Client
    };

    struct ClientInputData
    {
      bool m_Moving = false;
      Vector3f m_Dir;
    };

    struct ClientData
    {
      bool m_Moving = false;
      Vector3f m_Dir;
      Vector3f m_Pos;
    };

    EXL_ENGINE_API Err Connect(NetRole iRole, String const& iURL, uint16_t iPort);

    EXL_ENGINE_API void Tick(float iDelta);

    struct MovedObject
    {
      ObjectHandle object;
      ClientData data;
    };

    EXL_ENGINE_API Vector<MovedObject> const& GetMovedObjects();

    EXL_ENGINE_API void SetClientInput(ObjectHandle iChar, ClientInputData const& iInput);

    EXL_ENGINE_API void UpdateObjects(Vector<MovedObject> const&);
    
    // Client
    extern EXL_ENGINE_API std::function<ObjectHandle(ClientData const&)> OnNewObjectReceived;
    extern EXL_ENGINE_API std::function<void(ObjectHandle)> OnPlayerAssigned;

    // Server
    extern EXL_ENGINE_API std::function<ObjectHandle(void)> OnNewPlayer;
    extern EXL_ENGINE_API std::function<void(ObjectHandle, ClientInputData const&)> OnClientCommand;
  };
}