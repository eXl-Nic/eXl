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
#ifdef EXL_WITH_OGL
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
#endif
  }

  void OGLScissorCommand::Apply()
  {
#ifdef EXL_WITH_OGL
    if(m_Flag & EnableScissor)
    {
      glEnable(GL_SCISSOR_TEST);
      glScissor(m_ScissorCoord[0],m_ScissorCoord[1],m_ScissorCoord[2],m_ScissorCoord[3]);
    }
    else
    {
      glDisable(GL_SCISSOR_TEST);
    }
#endif
  }

  void OGLViewportCommand::Apply()
  {
#ifdef EXL_WITH_OGL
    glViewport(m_Orig.x,m_Orig.y, m_Size.x,m_Size.y);
#endif
  }

  void OGLBlendCommand::Apply()
  {
#ifdef EXL_WITH_OGL
    if (m_Flag & Enabled)
    {
      glEnable(GL_BLEND);
      glBlendFuncSeparate(GetGLBlend(srcRGB), GetGLBlend(dstRGB), GetGLBlend(srcAlpha), GetGLBlend(dstAlpha));
    }
    else
    {
      glDisable(GL_BLEND);
    }
#endif
  }

  OGLDisplayList::OGLDisplayList(OGLSemanticManager& iSemantics)
    : m_Semantics(iSemantics)
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

  void OGLDisplayList::SetDefaultViewport(Vec2i const& iOrig, Vec2i const& iSize)
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

  void OGLDisplayList::SetDefaultScissor(Vec2i const& iScissorOrig, Vec2i const& iScissorSize)
  {
    OGLScissorCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::ScissorCommand | (iScissorSize.x > 0 && iScissorSize.y > 0 ? OGLScissorCommand::EnableScissor:0);
    defCommand.m_ScissorCoord[0] = iScissorOrig.x;
    defCommand.m_ScissorCoord[1] = iScissorOrig.y;
    defCommand.m_ScissorCoord[2] = iScissorSize.x;
    defCommand.m_ScissorCoord[3] = iScissorSize.y;
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

  void OGLDisplayList::SetViewport(Vec2i const& iOrig, Vec2i const& iSize)
  {
    FlushDraws();
    OGLViewportCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::ViewportCommand;
    defCommand.m_Orig = iOrig;
    defCommand.m_Size = iSize;
    m_States.SetCommand(defCommand);
  }

  void OGLDisplayList::SetScissor(Vec2i const& iScissorOrig, Vec2i const& iScissorSize)
  {
    FlushDraws();
    OGLScissorCommand defCommand;
    defCommand.m_Flag = OGLRenderCommand::StateCommand | OGLStateCommand::ScissorCommand | (iScissorSize.x > 0 && iScissorSize.y > 0 ? OGLScissorCommand::EnableScissor:0);
    defCommand.m_ScissorCoord[0] = iScissorOrig.x;
    defCommand.m_ScissorCoord[1] = iScissorOrig.y;
    defCommand.m_ScissorCoord[2] = iScissorSize.x;
    defCommand.m_ScissorCoord[3] = iScissorSize.y;
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
    if (iProgram != m_CurProgram)
    {
      FlushDraws();
      m_CurProgram = iProgram;
    }
  }

  void OGLDisplayList::SetVAssembly(OGLVAssembly const* iAssembly)
  {
    if (iAssembly != m_CurAssembly)
    {
      FlushDraws();
      m_CurAssembly = iAssembly;
    }
  }

  void OGLDisplayList::PushData(OGLShaderData const* iData)
  {
    eXl_ASSERT(iData != nullptr);
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

  void OGLDisplayList::FillDrawHeader(OGLDraw& iDraw, uint8_t iTopo)
  {
    iDraw.m_Prog = m_CurProgram;
    iDraw.m_StateId = m_States.GetStateId();
    iDraw.m_VDecl = m_CurAssembly;
    iDraw.m_Topo = m_PendingDraws[0].topo;
  }

  void OGLDisplayList::FillGeom(OGLGeometry& iGeom, PendingDraw const& iDraw)
  {
    iGeom.m_Mat = iDraw.data;
    iGeom.m_Num = iDraw.num;
    iGeom.m_Offset = iDraw.offset;
    iGeom.m_BaseVertex = iDraw.baseVertex;
  }

  void OGLDisplayList::FillinstancedGeom(OGLInstancedGeometry& iGeom, PendingDraw const& iDraw)
  {
    iGeom.m_Mat = iDraw.data;
    iGeom.m_Num = iDraw.num;
    iGeom.m_Offset = iDraw.offset;
    iGeom.m_BaseVertex = iDraw.baseVertex;
    iGeom.m_Instances = iDraw.instances;
#ifndef __ANDROID__
    iGeom.m_BaseInstance = iDraw.baseInstance;
#endif
  }

  void OGLDisplayList::FlushDraws()
  {
    if (m_PendingDraws.size() == 1)
    {
      uint16_t curState = m_States.GetStateId();
      CommandKey newKey;
      newKey.m_Offset = m_Commands.size();
      newKey.m_Key = (uint64_t(m_PendingDraws[0].key) << 48) | uint64_t(curState) << 32 | (m_DataSetStore[m_PendingDraws[0].data].m_RenderHash);

      if (m_PendingDraws[0].instances == 0)
      {
        m_Commands.resize(m_Commands.size() + sizeof(OGLDraw) + sizeof(OGLGeometry));
        OGLDraw* draw = (OGLDraw*)((uint8_t*)&m_Commands[0] + newKey.m_Offset);
        draw->m_Flags = OGLRenderCommand::DrawCommand | OGLDraw::Draw;
        FillDrawHeader(*draw, m_PendingDraws[0].topo);

        OGLGeometry* geom = (OGLGeometry*)(draw + 1);
        FillGeom(*geom, m_PendingDraws[0]);
      }
      else
      {
        m_Commands.resize(m_Commands.size() + sizeof(OGLDraw) + sizeof(OGLInstancedGeometry));
        OGLDraw* draw = (OGLDraw*)((uint8_t*)&m_Commands[0] + newKey.m_Offset);
        draw->m_Flags = OGLRenderCommand::DrawCommand | OGLDraw::Draw | OGLDraw::DrawInstanced;
        FillDrawHeader(*draw, m_PendingDraws[0].topo);

        OGLInstancedGeometry* geom = (OGLInstancedGeometry*)(draw + 1);
        FillinstancedGeom(*geom, m_PendingDraws[0]);
      }

      m_Keys.push_back(newKey);

      m_PendingDraws.clear();
    }
    else if(m_PendingDraws.size() > 1)
    {
      uint8_t curState = m_States.GetStateId();

      CommandKey newKey;
      newKey.m_Offset = m_Commands.size();
      newKey.m_Key = (uint64_t(m_PendingDraws[0].key) << 48) | uint64_t(curState) << 32 | (m_DataSetStore[m_PendingDraws[0].data].m_RenderHash);
      
      if (m_PendingDraws[0].instances == 0)
      {
        m_Commands.resize(m_Commands.size() + sizeof(OGLDraw) + sizeof(uint32_t) + m_PendingDraws.size() * sizeof(OGLGeometry));

        OGLDraw* drawGroup = (OGLDraw*)((uint8_t*)&m_Commands[0] + newKey.m_Offset);
        drawGroup->m_Flags = OGLRenderCommand::DrawCommand | OGLDraw::DrawGroup;
        FillDrawHeader(*drawGroup, m_PendingDraws[0].topo);

        uint32_t* numDraws = (uint32_t*)(drawGroup + 1);
        *numDraws = m_PendingDraws.size();

        OGLGeometry* geom = (OGLGeometry*)(numDraws + 1);

        for (uint32_t i = 0; i < m_PendingDraws.size(); ++i)
        {
          FillGeom(*geom, m_PendingDraws[i]);
          ++geom;
        }
      }
      else
      {
        m_Commands.resize(m_Commands.size() + sizeof(OGLDraw) + sizeof(uint32_t) + m_PendingDraws.size() * sizeof(OGLInstancedGeometry));

        OGLDraw* drawGroup = (OGLDraw*)((uint8_t*)&m_Commands[0] + newKey.m_Offset);
        drawGroup->m_Flags = OGLRenderCommand::DrawCommand | OGLDraw::DrawGroup | OGLDraw::DrawInstanced;
        FillDrawHeader(*drawGroup, m_PendingDraws[0].topo);

        uint32_t* numDraws = (uint32_t*)(drawGroup + 1);
        *numDraws = m_PendingDraws.size();

        OGLInstancedGeometry* geom = (OGLInstancedGeometry*)(numDraws + 1);

        for (uint32_t i = 0; i < m_PendingDraws.size(); ++i)
        {
          FillinstancedGeom(*geom, m_PendingDraws[i]);
          ++geom;
        }
      }

      m_Keys.push_back(newKey);

      m_PendingDraws.clear();
    }
  }

  void OGLDisplayList::Clear(uint16_t iKey, bool iClearColor, bool iClearDepth, Vec4 const& iColor, float iDepth)
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

  void OGLDisplayList::PushDraw(uint16_t iKey, uint8_t iTopo, uint32_t iNum, uint32_t iOffset, uint32_t iBaseVertex)
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
        || m_PendingDraws.back().instances != 0
        || m_PendingDraws.back().data != m_CurDataSet))
    {
      FlushDraws();  
    }
    PendingDraw newDraw = {m_CurDataSet, iNum, iOffset, iBaseVertex, 0
#ifndef __ANDROID__
      , 0
#endif
      , iKey, iTopo};
    m_PendingDraws.push_back(newDraw);
  }

#ifndef __ANDROID__
  void OGLDisplayList::PushDrawInstanced(uint16_t iKey, uint8_t iTopo, uint32_t iNum, uint32_t iOffset, uint32_t iBaseVertex, uint32_t iNumInstances, uint32_t iBaseInstance)
#else
  void OGLDisplayList::PushDrawInstanced(uint16_t iKey, uint8_t iTopo, uint32_t iNum, uint32_t iOffset, uint32_t iBaseVertex, uint32_t iNumInstances)
#endif
  {
    if (iNumInstances == 0)
    {
      PushDraw(iKey, iTopo, iNum, iOffset, iBaseVertex);
    }

    switch (iTopo)
    {
    case OGLDraw::Point:
    case OGLDraw::LineList:
    case OGLDraw::LineStrip:
    case OGLDraw::TriangleList:
    case OGLDraw::TriangleStrip:
      break;
    default:
      eXl_ASSERT_MSG(false, "Incorrect topology");
      return;
    }

    eXl_ASSERT_MSG(m_CurProgram != NULL, "Program missing");
    eXl_ASSERT_MSG(m_CurAssembly != NULL, "Vertex assembly missing");

    if (m_PendingDraws.size() != 0
      && (m_PendingDraws.back().key != iKey
        || m_PendingDraws.back().topo != iTopo
        || m_PendingDraws.back().instances == 0
        || m_PendingDraws.back().data != m_CurDataSet))
    {
      FlushDraws();
    }

    PendingDraw newDraw = { m_CurDataSet, iNum, iOffset, iBaseVertex, iNumInstances
#ifndef __ANDROID__
      , iBaseInstance
#endif
      , iKey, iTopo };
    m_PendingDraws.push_back(newDraw);
  }

  void OGLVAssembly::Apply(OGLSemanticManager const& iSemantics, OGLRenderContext* iCtx)const
  {
    for(uint32_t i = 0; i<m_Attribs.size(); ++i)
    {
      VtxAttrib const& curAttr = m_Attribs[i];
      uint32_t slot = iSemantics.GetSlotForName(curAttr.m_AttribName);
      iCtx->SetVertexAttrib(slot, curAttr.m_VBuffer.get(), curAttr.m_Num, curAttr.m_Stride, curAttr.m_Offset);
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
          uint32_t slot = m_Semantics.GetSlotForName(dataPtr->m_Name);
          if(m_CurrentSetupData[slot].dataSet != iSet && m_CurrentSetupData[slot].timestamp < m_Timestamp)
          {
            iCtx->SetUniformData(slot, dataPtr->m_Data);
            m_CurrentSetupData[slot].dataSet = iSet;
          }
          m_CurrentSetupData[slot].timestamp = m_Timestamp;
          ++dataPtr;
        }
      }

      uint32_t numUBO = iSet->m_AdditionalData->GetNumUBO();
      if (numUBO > 0)
      {
        OGLShaderData::UBOData const* dataPtr = iSet->m_AdditionalData->GetUBODescPtr();
        for (uint32_t i = 0; i < numUBO; ++i)
        {
          uint32_t slot = m_Semantics.GetSlotForName(dataPtr->m_Name);
          if (m_CurrentSetupUBO[slot].dataSet != iSet && m_CurrentSetupUBO[slot].timestamp < m_Timestamp)
          {
            iCtx->SetUniformBuffer(slot, dataPtr->m_DataBuffer.get());
            m_CurrentSetupUBO[slot].dataSet = iSet;
          }
          m_CurrentSetupUBO[slot].timestamp = m_Timestamp;
          ++dataPtr;
        }
      }

      uint32_t numTex = iSet->m_AdditionalData->GetNumTexture();
      if(numTex > 0)
      {
        OGLShaderData::TextureData const* texPtr = iSet->m_AdditionalData->GetTexturePtr();
        for(uint32_t i = 0; i<numTex; ++i)
        {
          uint32_t slot = m_Semantics.GetSlotForName(texPtr->m_Name);
          if(m_CurrentSetupTexture[slot].dataSet != iSet && m_CurrentSetupTexture[slot].timestamp < m_Timestamp)
          {
            iCtx->SetTexture(slot,texPtr->m_Texture.get());
            m_CurrentSetupTexture[slot].dataSet = iSet;
          }
          m_CurrentSetupTexture[slot].timestamp = m_Timestamp;
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
    m_CurrentSetupData.resize(m_Semantics.GetNumUniforms(),DataSetup());
    m_CurrentSetupUBO.clear();
    m_CurrentSetupUBO.resize(m_Semantics.GetNumUniforms(), DataSetup());
    m_CurrentSetupTexture.clear();
    m_CurrentSetupTexture.resize(m_Semantics.GetNumTextures(),DataSetup());

    m_CurProgram = NULL;
    m_CurAssembly = NULL;
    m_CurDataSet = 0;

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
                  drawCmd->m_VDecl->Apply(m_Semantics, iCtx);
                  m_CurAssembly = drawCmd->m_VDecl;
                }

                OGLBuffer const* idxBuff = drawCmd->m_VDecl->m_IBuffer.get();
                uint32_t idxOffset = drawCmd->m_VDecl->m_IOffset;

                uint32_t numDraws;
                if ((drawCmd->m_Flags & OGLDraw::DrawInstanced) == 0)
                {
                  OGLGeometry const* geom;

                  if ((drawCmd->m_Flags & OGLDraw::DrawGroup) == 0)
                  {
                    geom = (OGLGeometry const*)(drawCmd + 1);
                    numDraws = 1;
                  }
                  else
                  {
                    numDraws = *(uint32_t*)(drawCmd + 1);
                    geom = (OGLGeometry const*)((uint32_t*)(drawCmd + 1) + 1);
                  }

                  if (idxBuff)
                  {
                    for (uint32_t draws = 0; draws < numDraws; ++draws)
                    {
                      if (m_CurDataSet != geom->m_Mat)
                      {
                        if (geom->m_Mat != -1)
                        {
                          HandleDataSet(iCtx, &m_DataSetStore[geom->m_Mat]);
                        }
                        m_CurDataSet = geom->m_Mat;
                      }
                      iCtx->DrawIndexed(idxBuff, topo, idxOffset + geom->m_Offset * sizeof(uint32_t), geom->m_BaseVertex, geom->m_Num);
                      ++geom;
                    }
                  }
                  else
                  {
                    for (uint32_t draws = 0; draws < numDraws; ++draws)
                    {
                      if (m_CurDataSet != geom->m_Mat)
                      {
                        if (geom->m_Mat != -1)
                        {
                          HandleDataSet(iCtx, &m_DataSetStore[geom->m_Mat]);
                        }
                        m_CurDataSet = geom->m_Mat;
                      }
                      iCtx->Draw(topo, geom->m_Offset, geom->m_Num);
                      geom++;
                    }
                  }
                }
                else
                {
                  OGLInstancedGeometry const* geom;

                  if ((drawCmd->m_Flags & OGLDraw::DrawGroup) == 0)
                  {
                    geom = (OGLInstancedGeometry const*)(drawCmd + 1);
                    numDraws = 1;
                  }
                  else
                  {
                    numDraws = *(uint32_t*)(drawCmd + 1);
                    geom = (OGLInstancedGeometry const*)((uint32_t*)(drawCmd + 1) + 1);
                  }

                  if (idxBuff)
                  {
                    for (uint32_t draws = 0; draws < numDraws; ++draws)
                    {
                      if (m_CurDataSet != geom->m_Mat)
                      {
                        if (geom->m_Mat != -1)
                        {
                          HandleDataSet(iCtx, &m_DataSetStore[geom->m_Mat]);
                        }
                        m_CurDataSet = geom->m_Mat;
                      }
#ifndef __ANDROID__
                      iCtx->DrawIndexedInstanced(idxBuff, topo, geom->m_Instances, geom->m_BaseInstance, idxOffset + geom->m_Offset * sizeof(uint32_t), geom->m_BaseVertex, geom->m_Num);
#else
                      iCtx->DrawIndexedInstanced(idxBuff, topo, geom->m_Instances, idxOffset + geom->m_Offset * sizeof(uint32_t), geom->m_BaseVertex, geom->m_Num);
#endif
                      ++geom;
                    }
                  }
                  else
                  {
                    for (uint32_t draws = 0; draws < numDraws; ++draws)
                    {
                      if (m_CurDataSet != geom->m_Mat)
                      {
                        if (geom->m_Mat != -1)
                        {
                          HandleDataSet(iCtx, &m_DataSetStore[geom->m_Mat]);
                        }
                        m_CurDataSet = geom->m_Mat;
                      }
#ifndef __ANDROID__
                      iCtx->DrawInstanced(topo, geom->m_Instances, geom->m_BaseInstance, geom->m_Offset, geom->m_Num);
#else
                      iCtx->DrawInstanced(topo, geom->m_Instances, geom->m_Offset, geom->m_Num);
#endif
                      geom++;
                    }
                  }
                }
              }
              break;
            case OGLDraw::Clear:
              {
#ifdef EXL_WITH_OGL
                OGLClear const* clearCmd = (OGLClear const*)curCmd;
                if (clearCmd->m_StateId != m_States.GetCurState())
                {
                  m_States.ApplyCommand(clearCmd->m_StateId);
                }
                if (clearCmd->m_Flags & OGLClear::Color)
                {
                  glClearColor(clearCmd->m_ClearColor.x, clearCmd->m_ClearColor.y, clearCmd->m_ClearColor.z, clearCmd->m_ClearColor.w);
                  glClear(GL_COLOR_BUFFER_BIT);
                }
                if (clearCmd->m_Flags & OGLClear::Depth)
                {
                  glClearDepthf(clearCmd->m_ClearDepth);
                  glClear(GL_DEPTH_BUFFER_BIT);
                }
#endif
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
