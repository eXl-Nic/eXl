/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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