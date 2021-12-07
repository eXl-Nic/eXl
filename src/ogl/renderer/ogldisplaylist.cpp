/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglrendercontext.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/oglstatecollection.inl>
#include <ogl/renderer/ogltypesconv.hpp>

#include <algorithm>

namespace eXl
{
  void OGLDepthCommand::Apply()
  {
    if(m_Flag & ReadZ)
    {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
    }
    else
    {
      glDisable(GL_DEPTH_TEST);
    }

    if(m_Flag & WriteZ)
    {
      glDepthMask(GL_TRUE);
    }
    else
    {
      glDepthMask(GL_FALSE);
    }
  }

  void OGLScissorCommand::Apply()
  {
    if(m_Flag & EnableScissor)
    {
      glEnable(GL_SCISSOR_TEST);
      glScissor(m_ScissorCoord[0],m_ScissorCoord[1],m_ScissorCoord[2],m_ScissorCoord[3]);
    }
    else
    {
      glDisable(GL_SCISSOR_TEST);
    }
  }

  void OGLViewportCommand::Apply()
  {
    glViewport(m_Orig.X(),m_Orig.Y(), m_Size.X(),m_Size.Y());
  }

  void OGLBlendCommand::Apply()
  {
    if (m_Flag & Enabled)
    {
      glEnable(GL_BLEND);
      glBlendFuncSeparate(GetGLBlend(srcRGB), GetGLBlend(dstRGB), GetGLBlend(srcAlpha), GetGLBlend(dstAlpha));
    }
    else
    {
      glDisable(GL_BLEND);
    }
  }

  OGLDisplayList::OGLDisplayList()
  {
    m_DataSetSeek.reserve(1024);
    m_DataSetStore.reserve(1024);
  }

  void OGLDisplayList::InitForPush()
  {
    m_CurDataSet = -1;
    m_CurAssembly = nullptr;
    m_CurProgram = nullptr;
    m_DataSetSeek.clear();
    m_DataSetStore.clear();
    m_Commands.clear();
    m_Keys.clear();

    m_States.InitForPush();
  }

  void OGLDisplayList::SetDefaultViewport(Vector2i const& iOrig, Vector2i const& iSize)
  {
    OGLViewportCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::ViewportCommand;
    defCommand.m_Orig = iOrig;
    defCommand.m_Size = iSize;
    m_States.SetDefaultCommand(defCommand);
  }

  void OGLDisplayList::SetDefaultDepth(bool iWriteZ, bool iReadZ)
  {
    OGLDepthCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::DepthCommand | (iWriteZ ? OGLDepthCommand::WriteZ:0) | (iReadZ ? OGLDepthCommand::ReadZ:0);
    m_States.SetDefaultCommand(defCommand);
  }

  void OGLDisplayList::SetDefaultScissor(Vector2i const& iScissorOrig, Vector2i const& iScissorSize)
  {
    OGLScissorCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::ScissorCommand | (iScissorSize.X() > 0 && iScissorSize.Y() > 0 ? OGLScissorCommand::EnableScissor:0);
    defCommand.m_ScissorCoord[0] = iScissorOrig.X();
    defCommand.m_ScissorCoord[1] = iScissorOrig.Y();
    defCommand.m_ScissorCoord[2] = iScissorSize.X();
    defCommand.m_ScissorCoord[3] = iScissorSize.Y();
    m_States.SetDefaultCommand(defCommand);
  }

  void OGLDisplayList::SetDefaultBlend(bool iEnabled, OGLBlend iSrc, OGLBlend iDst)
  {
    OGLBlendCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::BlendCommand | (iEnabled ? OGLBlendCommand::Enabled : 0);
    defCommand.srcRGB = iSrc;
    defCommand.dstRGB = iDst;
    defCommand.srcAlpha = iSrc;
    defCommand.dstAlpha = iDst;
    m_States.SetDefaultCommand(defCommand);
  }

  void OGLDisplayList::SetViewport(Vector2i const& iOrig, Vector2i const& iSize)
  {
    FlushDraws();
    OGLViewportCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::ViewportCommand;
    defCommand.m_Orig = iOrig;
    defCommand.m_Size = iSize;
    m_States.SetCommand(defCommand);
  }

  void OGLDisplayList::SetScissor(Vector2i const& iScissorOrig, Vector2i const& iScissorSize)
  {
    FlushDraws();
    OGLScissorCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::ScissorCommand | (iScissorSize.X() > 0 && iScissorSize.Y() > 0 ? OGLScissorCommand::EnableScissor:0);
    defCommand.m_ScissorCoord[0] = iScissorOrig.X();
    defCommand.m_ScissorCoord[1] = iScissorOrig.Y();
    defCommand.m_ScissorCoord[2] = iScissorSize.X();
    defCommand.m_ScissorCoord[3] = iScissorSize.Y();
    m_States.SetCommand(defCommand);
  }

  void OGLDisplayList::SetDepth(bool iWriteZ, bool iReadZ)
  {
    FlushDraws();
    OGLDepthCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::DepthCommand | (iWriteZ ? OGLDepthCommand::WriteZ:0) | (iReadZ ? OGLDepthCommand::ReadZ:0);
    m_States.SetCommand(defCommand);
  }

  void OGLDisplayList::SetBlend(bool iEnabled, OGLBlend iSrc, OGLBlend iDst)
  {
    OGLBlendCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::BlendCommand | (iEnabled ? OGLBlendCommand::Enabled : 0);
    defCommand.srcRGB = iSrc;
    defCommand.dstRGB = iDst;
    defCommand.srcAlpha = iSrc;
    defCommand.dstAlpha = iDst;
    m_States.SetCommand(defCommand);
  }

  void OGLDisplayList::SetProgram(OGLCompiledProgram const* iProgram)
  {
    FlushDraws();
    m_CurProgram = iProgram;
  }

  void OGLDisplayList::SetVAssembly(OGLVAssembly const* iAssembly)
  {
    FlushDraws();
    m_CurAssembly = iAssembly;
  }

  void OGLDisplayList::PushData(OGLShaderData const* iData)
  {
    //FlushDraws();
    OGLShaderDataSet const* prevSet = m_CurDataSet != -1 ? &m_DataSetStore[m_CurDataSet] : nullptr;
    OGLShaderDataSet curSet(iData, prevSet, static_cast<uint32_t>(m_DataSetStore.size()));
    auto iter = m_DataSetSeek.find(curSet);
    if(iter == m_DataSetSeek.end())
    {
      auto res = m_DataSetSeek.insert(curSet);
      m_DataSetStore.push_back(curSet);
      iter = res.first;
    }
    m_CurDataSet = iter->m_Id;
  }

  void OGLDisplayList::PopData()
  {
    //FlushDraws();
    if(m_CurDataSet != -1)
    {
      m_CurDataSet = m_DataSetStore[m_CurDataSet].m_PrevSet;
    }
  }

  void OGLDisplayList::FlushDraws()
  {
    if (m_PendingDraws.size() == 1)
    {
      uint16_t curState = m_States.GetStateId();
      CommandKey newKey;
      newKey.m_Offset = m_Commands.size();
      newKey.m_Key = (uint64_t(m_PendingDraws[0].key) << 48) | uint64_t(curState) << 32 | (m_DataSetStore[m_PendingDraws[0].data].m_RenderHash);

      m_Commands.resize(m_Commands.size() + sizeof(OGLDraw) + sizeof(OGLGeometry));
      OGLDraw* draw = (OGLDraw*)((uint8_t*)&m_Commands[0] + newKey.m_Offset);
      draw->m_Flags = OGLRenderCommand::DrawCommand | OGLDraw::Draw /*| m_PendingDraws[0].topo*/;
      draw->m_Prog = m_CurProgram;
      draw->m_StateId = curState;
      draw->m_VDecl = m_CurAssembly;
      draw->m_Topo = m_PendingDraws[0].topo;

      OGLGeometry* geom = (OGLGeometry*)(draw+1);
      geom->m_Mat = m_PendingDraws[0].data;
      geom->m_Num = m_PendingDraws[0].num;
      geom->m_Offset = m_PendingDraws[0].offset;

      m_Keys.push_back(newKey);

      m_PendingDraws.clear();
    }
    else if(m_PendingDraws.size() > 1)
    {
      uint8_t curState = m_States.GetStateId();

      CommandKey newKey;
      newKey.m_Offset = m_Commands.size();
      newKey.m_Key = (uint64_t(m_PendingDraws[0].key) << 48) | uint64_t(curState) << 32 | (m_DataSetStore[m_PendingDraws[0].data].m_RenderHash);
      
      m_Commands.resize(m_Commands.size() + sizeof(OGLDraw) + sizeof(uint32_t) + m_PendingDraws.size()*sizeof(OGLGeometry));

      OGLDraw* drawGroup = (OGLDraw*)((uint8_t*)&m_Commands[0] + newKey.m_Offset);
      drawGroup->m_Flags = OGLRenderCommand::DrawCommand | OGLDraw::DrawGroup /*| m_PendingDraws[0].topo*/;
      drawGroup->m_Prog = m_CurProgram;
      drawGroup->m_StateId = curState;
      drawGroup->m_VDecl = m_CurAssembly;
      drawGroup->m_Topo = m_PendingDraws[0].topo;

      uint32_t* numDraws = (uint32_t*)(drawGroup+1);
      *numDraws = m_PendingDraws.size();

      OGLGeometry* geom = (OGLGeometry*)(numDraws + 1);

      for(uint32_t i = 0; i<m_PendingDraws.size(); ++i)
      {
        geom->m_Mat = m_PendingDraws[i].data;
        geom->m_Num = m_PendingDraws[i].num;
        geom->m_Offset = m_PendingDraws[i].offset;
        ++geom;
      }

      m_Keys.push_back(newKey);

      m_PendingDraws.clear();
    }
  }

  void OGLDisplayList::Clear(uint16_t iKey, bool iClearColor, bool iClearDepth, Vector4f const& iColor, float iDepth)
  {
    uint8_t curState = m_States.GetStateId();
    CommandKey newKey;
    newKey.m_Offset = m_Commands.size();
    newKey.m_Key = iKey;

    m_Commands.resize(m_Commands.size() + sizeof(OGLClear));
    OGLClear* clearCmd = (OGLClear*)((uint8_t*)&m_Commands[0] + newKey.m_Offset);

    clearCmd->m_Flags = OGLRenderCommand::DrawCommand | OGLDraw::Clear | (iClearColor ? OGLClear::Color : 0) | (iClearDepth ? OGLClear::Depth : 0);
    clearCmd->m_StateId = curState;
    clearCmd->m_ClearColor = iColor;
    clearCmd->m_ClearDepth = iDepth;

    m_Keys.push_back(newKey);
  }

  void OGLDisplayList::PushDraw(uint16_t iKey, uint8_t iTopo, uint32_t iNum, uint32_t iOffset)
  {
    switch(iTopo)
    {
    case OGLDraw::Point:
    case OGLDraw::LineList:
    case OGLDraw::LineStrip:
    case OGLDraw::TriangleList:
    case OGLDraw::TriangleStrip:
      break;
    default:
      eXl_ASSERT_MSG(false,"Incorrect topology");
      return;
    }

    eXl_ASSERT_MSG(m_CurProgram != NULL,"Program missing");
    eXl_ASSERT_MSG(m_CurAssembly != NULL,"Vertex assembly missing");

    if(m_PendingDraws.size() != 0
       && (m_PendingDraws.back().key != iKey
        || m_PendingDraws.back().topo != iTopo
        || m_PendingDraws.back().data != m_CurDataSet))
    {
      FlushDraws();  
    }

    PendingDraw newDraw = {m_CurDataSet, iNum, iOffset, iKey, iTopo};
    m_PendingDraws.push_back(newDraw);
  }

  void OGLVAssembly::Apply(OGLRenderContext* iCtx)const
  {
    for(uint32_t i = 0; i<m_Attribs.size(); ++i)
    {
      VtxAttrib const& curAttr = m_Attribs[i];
      iCtx->SetVertexAttrib(curAttr.m_AttribId, curAttr.m_VBuffer, curAttr.m_Num, curAttr.m_Stride, curAttr.m_Offset);
    }
  }

  namespace
  {
    OGLConnectivity topology [] = 
    {
      OGLConnectivity::POINT, 
      OGLConnectivity::LINELIST,
      OGLConnectivity::LINESTRIP,
      OGLConnectivity::TRIANGLELIST,
      OGLConnectivity::TRIANGLESTRIP
    };
  }

  void OGLDisplayList::HandleDataSet(OGLRenderContext* iCtx, OGLShaderDataSet const* iSet)
  {
    m_Timestamp++;
    while(iSet != nullptr)
    {
      uint32_t numData = iSet->m_AdditionalData->GetNumData();
      if(numData > 0)
      {
        OGLShaderData::ShaderData const* dataPtr = iSet->m_AdditionalData->GetDataDescPtr();
        for(uint32_t i = 0; i<numData; ++i)
        {
          if(m_CurrentSetupData[dataPtr->m_DataSlot].dataSet != iSet && m_CurrentSetupData[dataPtr->m_DataSlot].timestamp < m_Timestamp)
          {
            iCtx->SetUniformData(dataPtr->m_DataSlot,dataPtr->m_Data);
            m_CurrentSetupData[dataPtr->m_DataSlot].dataSet = iSet;
          }
          m_CurrentSetupData[dataPtr->m_DataSlot].timestamp = m_Timestamp;
          ++dataPtr;
        }
      }

      uint32_t numUBO = iSet->m_AdditionalData->GetNumUBO();
      if (numUBO > 0)
      {
        OGLShaderData::UBOData const* dataPtr = iSet->m_AdditionalData->GetUBODescPtr();
        for (uint32_t i = 0; i < numUBO; ++i)
        {
          if (m_CurrentSetupUBO[dataPtr->m_Slot].dataSet != iSet && m_CurrentSetupUBO[dataPtr->m_Slot].timestamp < m_Timestamp)
          {
            iCtx->SetUniformBuffer(dataPtr->m_Slot, dataPtr->m_DataBuffer.get());
            m_CurrentSetupUBO[dataPtr->m_Slot].dataSet = iSet;
          }
          m_CurrentSetupUBO[dataPtr->m_Slot].timestamp = m_Timestamp;
          ++dataPtr;
        }
      }

      uint32_t numTex = iSet->m_AdditionalData->GetNumTexture();
      if(numTex > 0)
      {
        OGLShaderData::TextureData const* texPtr = iSet->m_AdditionalData->GetTexturePtr();
        for(uint32_t i = 0; i<numTex; ++i)
        {
          if(m_CurrentSetupTexture[texPtr->m_TextureSlot].dataSet != iSet && m_CurrentSetupTexture[texPtr->m_TextureSlot].timestamp < m_Timestamp)
          {
            iCtx->SetTexture(texPtr->m_TextureSlot,texPtr->m_Texture.get());
            m_CurrentSetupTexture[texPtr->m_TextureSlot].dataSet = iSet;
          }
          m_CurrentSetupTexture[texPtr->m_TextureSlot].timestamp = m_Timestamp;
          ++texPtr;
        }
      }
      if (iSet->m_PrevSet == -1)
      {
        return;
      }
      else
      {
        iSet = &m_DataSetStore[iSet->m_PrevSet];
      }
    }
  }

  void OGLDisplayList::Render(OGLRenderContext* iCtx, OGLFramebuffer* iFBO)
  {
    FlushDraws();
    m_States.InitForRender();
    std::sort(m_Keys.begin(), m_Keys.end());

    m_CurrentSetupData.clear();
    m_CurrentSetupData.resize(OGLSemanticManager::GetNumUniforms(),DataSetup());
    m_CurrentSetupUBO.clear();
    m_CurrentSetupUBO.resize(OGLSemanticManager::GetNumUniforms(), DataSetup());
    m_CurrentSetupTexture.clear();
    m_CurrentSetupTexture.resize(OGLSemanticManager::GetNumTextures(),DataSetup());

    m_CurProgram = NULL;
    m_CurAssembly = NULL;
    m_CurDataSet = NULL;

    m_Timestamp = 0;

    iCtx->SetFramebuffer(iFBO);

    if(!m_Commands.empty() && !m_Keys.empty())
    {
      CommandKey* curKey = &m_Keys[0];
      uint8_t const* baseCommand = &m_Commands[0];

      uint32_t numCommands = m_Keys.size();
      for(uint32_t i = 0; i<numCommands; ++i, ++curKey)
      {
        uint8_t const* curCmd = baseCommand + curKey->m_Offset;
        switch((*curCmd & OGLRenderCommand::Mask) >>OGLRenderCommand::Shift)
        {
        case OGLRenderCommand::DrawCommand:
          {
            switch((*curCmd & OGLDraw::MaskDraw) /*>> OGLDraw::ShiftDraw*/)
            {
            case OGLDraw::Draw:
              {
                OGLDraw const* drawCmd = (OGLDraw const*)curCmd;
                if (drawCmd->m_StateId != m_States.GetCurState())
                {
                  m_States.ApplyCommand(drawCmd->m_StateId);
                }

                OGLConnectivity topo = topology[drawCmd->m_Topo];
            
                if(drawCmd->m_Prog != m_CurProgram)
                {
                  iCtx->SetProgram(drawCmd->m_Prog);
                  m_CurProgram = drawCmd->m_Prog;
                }
                if(drawCmd->m_VDecl != m_CurAssembly)
                {
                  drawCmd->m_VDecl->Apply(iCtx);
                  m_CurAssembly = drawCmd->m_VDecl;
                }

                OGLBuffer const* idxBuff = drawCmd->m_VDecl->m_IBuffer;
                uint32_t idxOffset = drawCmd->m_VDecl->m_IOffset;
                if(!(drawCmd->m_Flags & OGLDraw::DrawGroup))
                {
                  OGLGeometry const* geom = (OGLGeometry const*)(drawCmd + 1);
                  if(m_CurDataSet != geom->m_Mat)
                  {
                    if (geom->m_Mat != -1)
                    {
                      HandleDataSet(iCtx, &m_DataSetStore[geom->m_Mat]);
                    }
                    m_CurDataSet = geom->m_Mat;
                  }
                  if(idxBuff)
                  {
                    iCtx->DrawIndexed(idxBuff,topo,idxOffset + geom->m_Offset * sizeof(uint32_t),geom->m_Num);
                  }
                  else
                  {
                    iCtx->Draw(topo,geom->m_Offset,geom->m_Num);
                  }
                }
                else
                {
                  uint32_t numDraws = *(uint32_t*)(drawCmd + 1);
                  OGLGeometry const* geom = (OGLGeometry const*)((uint32_t*)(drawCmd + 1) + 1);
                  if(idxBuff)
                  {
                    for(uint32_t draws = 0; draws < numDraws; ++draws)
                    {
                      if(m_CurDataSet != geom->m_Mat)
                      {
                        if (geom->m_Mat != -1)
                        {
                          HandleDataSet(iCtx, &m_DataSetStore[geom->m_Mat]);
                        }
                        m_CurDataSet = geom->m_Mat;
                      }
                      iCtx->DrawIndexed(idxBuff,topo,idxOffset + geom->m_Offset * sizeof(uint32_t),geom->m_Num);
                      geom++;
                    }
                  }
                  else
                  {
                    for(uint32_t draws = 0; draws < numDraws; ++draws)
                    {
                      if(m_CurDataSet != geom->m_Mat)
                      {
                        if (geom->m_Mat != -1)
                        {
                          HandleDataSet(iCtx, &m_DataSetStore[geom->m_Mat]);
                        }
                        m_CurDataSet = geom->m_Mat;
                      }
                      iCtx->Draw(topo,geom->m_Offset,geom->m_Num);
                      geom++;
                    }
                  }
                }
              }
              break;
            case OGLDraw::Clear:
              {
                OGLClear const* clearCmd = (OGLClear const*)curCmd;
                if (clearCmd->m_StateId != m_States.GetCurState())
                {
                  m_States.ApplyCommand(clearCmd->m_StateId);
                }
                if (clearCmd->m_Flags & OGLClear::Color)
                {
                  glClearColor(clearCmd->m_ClearColor.X(), clearCmd->m_ClearColor.Y(), clearCmd->m_ClearColor.Z(), clearCmd->m_ClearColor.W());
                  glClear(GL_COLOR_BUFFER_BIT);
                }
                if (clearCmd->m_Flags & OGLClear::Depth)
                {
                  glClearDepthf(clearCmd->m_ClearDepth);
                  glClear(GL_DEPTH_BUFFER_BIT);
                }
              }
              break;
            default:
              eXl_ASSERT_MSG(false, "Incorrect draw");
            }
          }
          break;
        default:
          eXl_ASSERT_MSG(false, "Incorrect command");
          break;
        }
      }
    }

    iCtx->Clear();
  }
}
