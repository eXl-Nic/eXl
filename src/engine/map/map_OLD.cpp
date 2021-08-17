#include <dunatk/map/map.hpp>
#include <dunatk/map/tilenode.hpp>
#include <dunatk/map/tileblock.hpp>
#include <dunatk/map/tile.hpp>
#include <core/type/tupletype.hpp>
#include <ogl/oglspritebatcher.hpp>

#include <dunatk/gfx/gfxsystem.hpp>
#include <dunatk/physics/physicsys.hpp>

namespace eXl
{
  namespace Old
  {
    TileMap::TileMap(TileSet& iSet, unsigned short iX, unsigned short iY)
      : m_WallSet(iSet)
      , m_UnitX(iX)
      , m_UnitY(iY)
    {
      if (m_WallSet.FloorIntCorner != NULL)
        eXl_ASSERT_MSG(m_WallSet.FloorExtCorner != NULL, "Missing ExtCorner");
      if (m_WallSet.WallIntCorner != NULL)
        eXl_ASSERT_MSG(m_WallSet.WallExtCorner != NULL, "Missing ExtCorner");

      int guardband = m_WallSet.wall_Thickness;
      m_Size = Vector2i(m_UnitX * m_WallSet.unitFloor, m_UnitY * m_WallSet.unitFloor);
      m_GuardSize = Vector2i(m_Size.X() + 2 * guardband, m_Size.Y() + 2 * guardband);

      Vector2f fillSize = m_WallSet.Fill->GetSize();

      AABB2Di rightBox(Vector2i(iX * m_WallSet.unitFloor, -guardband), Vector2i(guardband, iY*m_WallSet.unitFloor + 2 * guardband));
      AABB2Di leftBox(Vector2i(-guardband, -guardband), Vector2i(guardband, iY*m_WallSet.unitFloor + 2 * guardband));
      AABB2Di upperBox(Vector2i(0, iY * m_WallSet.unitFloor), Vector2i(iX*m_WallSet.unitFloor, guardband));
      AABB2Di lowerBox(Vector2i(0, -guardband), Vector2i(iX*m_WallSet.unitFloor, guardband));

      /*m_Guard = new TileBlock(NULL,TileNode::Wall);
      m_Guard->SetUserPtr((void*)-1);*/
      PolygonType guardPoly(rightBox);
      guardPoly.AddBox(upperBox);
      guardPoly.AddBox(lowerBox);
      guardPoly.AddBox(leftBox);

      m_Guard.Swap(guardPoly);

      // m_ColWorld->addCollisionObject(m_Guard->GetCollisionObject());

       //TileBlock* defaultBlock = eXl_NEW TileBlock(NULL,TileNode::Wall);
      guardPoly.AddBox(rightBox);
      guardPoly.AddBox(upperBox);
      guardPoly.AddBox(lowerBox);
      guardPoly.AddBox(leftBox);
      guardPoly.AddBox(AABB2Di(Vector2i(0, 0), Vector2i(iX*m_WallSet.unitFloor, iY*m_WallSet.unitFloor)));
      m_Walls.push_back(PolygonType());
      m_Walls.back().Swap(guardPoly);
    }

    TileMap::~TileMap()
    {
      EmptyMapView();
    }

    inline TileNode* MakeNode(TileNode* iNode, Vector2f const& iOrig, Vector2f const& iSize, Vector2f const& oldTileSize)
    {
      TileNode* newNode = eXl_NEW TileNode(*iNode);
      newNode->SetOrig(iOrig);
      newNode->SetRepeat(Mathf::Round(iSize.X() / oldTileSize.X()), Mathf::Round(iSize.Y() / oldTileSize.Y()));

      return newNode;
    }

    bool TileMap::TryOperation(TileNode const* iNodeAdded, Operation& oResult)
    {
      oResult.m_InNodes.clear();
      oResult.m_OutNodes.clear();


      return false;
    }

    bool TileMap::TryOperation(TileBlock const& iBlockAdded, BlockOperation& oResult)
    {
      oResult.m_InBlock.clear();
      oResult.m_OutBlock.clear();

      if ((iBlockAdded.GetCategory() & (TileNode::Wall | TileNode::Floor)) != 0)
      {
        PolygonType const* polyToConsider = NULL;
        polyToConsider = &iBlockAdded.GetPolygon();

        Vector<PolygonType>& blocks = iBlockAdded.GetCategory() & (TileNode::Wall) ? m_Floor : m_Walls;

        for (unsigned int i = 0; i < blocks.size(); ++i)
        {
          if (polyToConsider->GetAABB().Intersect(blocks[i].GetAABB()))
          {
            Vector<PolygonType> intersect;
            blocks[i].Intersection(*polyToConsider, intersect);
            for (unsigned int j = 0; j < intersect.size(); ++j)
            {
              Vector<PolygonType> crop;
              intersect[j].Difference(m_Guard, crop);

              for (unsigned int k = 0; k < crop.size(); ++k)
              {
                if (crop[k].GetAABB().IsInside(m_Guard.GetAABB()))
                {
                  TileBlock outBlock(NULL, iBlockAdded.GetCategory() & (TileNode::Wall) ? TileNode::Floor : TileNode::Wall);
                  oResult.m_OutBlock.push_back(outBlock);
                  oResult.m_OutBlock.back().SetPolygon(crop[k]);
                  TileBlock inBlock(iBlockAdded.GetTile(), iBlockAdded.GetCategory());
                  oResult.m_InBlock.push_back(inBlock);
                  oResult.m_InBlock.back().SwapPolygon(crop[k]);
                }
              }
            }
          }
        }

        if (polyToConsider != &iBlockAdded.GetPolygon())
        {
          delete polyToConsider;
        }

        return true;
      }

      return false;
    }

    void TileMap::CommitOperation(BlockOperation const& iOp)
    {
      if (iOp.m_InBlock.size() == 0 || iOp.m_OutBlock.size() == 0)
        return;

      Vector<PolygonType>& inPoly = iOp.m_InBlock[0].GetCategory() & TileNode::Wall ? m_Walls : m_Floor;
      Vector<PolygonType>& outPoly = iOp.m_OutBlock[0].GetCategory() & TileNode::Wall ? m_Walls : m_Floor;

      for (unsigned int i = 0; i < iOp.m_OutBlock.size(); ++i)
      {
        unsigned int curSizeOut = outPoly.size();
        for (unsigned int j = 0; j < curSizeOut; ++j)
        {
          if ((iOp.m_OutBlock[i].GetAABB().Intersect(outPoly[j].GetAABB())))
          {
            Vector<PolygonType> diff;
            outPoly[j].Difference(iOp.m_OutBlock[i].GetPolygon(), diff);
            if (diff.size() > 0)
            {
              outPoly[j].Swap(diff[0]);
            }
            else
            {
              PolygonType eraser;
              outPoly[j].Swap(eraser);
            }
            for (unsigned int k = 1; k < diff.size(); k++)
            {
              outPoly.push_back(PolygonType());
              outPoly.back().Swap(diff[k]);
            }
            break;
          }
        }
      }

      for (unsigned int i = 0; i < iOp.m_InBlock.size(); ++i)
      {
        inPoly.push_back(PolygonType(iOp.m_InBlock[i].GetPolygon()));
      }


      PolygonType::Merge(m_Walls);
      unsigned int curWallSize = m_Walls.size();
      for (unsigned int i = 0; i < curWallSize; ++i)
      {
        /*PolygonType wallWithBorder;
        m_Guard->GetPolygon().Union(newPolyWall[i],wallWithBorder);*/
        //if(wallWithBorder.Points().size() == 0)
        {
          Vector<PolygonType> newPoly;
          Vector<PolygonType> removedParts;
          m_Walls[i].RemoveTiny(m_WallSet.minWallSize, newPoly, removedParts);
          if (newPoly.size() > 0)
            m_Walls[i].Swap(newPoly[0]);
          else
          {
            PolygonType eraser;
            m_Walls[i].Swap(eraser);
          }
          for (unsigned int j = 1; j < newPoly.size(); ++j)
          {
            m_Walls.push_back(PolygonType());
            m_Walls.back().Swap(newPoly[j]);
          }
          m_Floor.insert(m_Floor.end(), removedParts.begin(), removedParts.end());
        }
        /*else
        {
          std::vector<PolygonType> newPoly;
          std::vector<PolygonType> removedParts;
          wallWithBorder.RemoveTiny(m_WallSet.minWallSize,newPoly,removedParts);
          std::vector<PolygonType> newPoly2;
          for(unsigned int j = 0 ; j<newPoly.size();++j)
          {
            std::vector<PolygonType> temp;
            newPoly[j].Difference(m_Guard->GetPolygon(),temp);
            for(unsigned int k = 0;k<temp.size();++k)
            {
              if(temp[k].Points().size()>0)
              {
                newPoly2.push_back(PolygonType());
                newPoly2.back().Swap(temp[k]);
              }
            }
          }
          if(newPoly2.size()>0)
            newPolyWall[i].Swap(newPoly2[0]);
          else
          {
            PolygonType eraser;
            newPolyWall[i].Swap(eraser);
          }
          for(unsigned int j = 1 ; j<newPoly2.size();++j)
          {
            newPolyWall.push_back(PolygonType());
            newPolyWall.back().Swap(newPoly2[j]);
          }
          std::vector<PolygonType> removedParts2;
          for(unsigned int j = 0 ; j<removedParts.size();++j)
          {
            std::vector<PolygonType> temp;
            removedParts[j].Difference(m_Guard->GetPolygon(),temp);
            for(unsigned int k = 0;k<temp.size();++k)
            {
              if(temp[k].Points().size()>0)
              {
                removedParts2.push_back(PolygonType());
                removedParts2.back().Swap(temp[k]);
              }
            }
          }
          newPolyFloor.insert(newPolyFloor.end(),removedParts2.begin(),removedParts2.end());
        }*/

      }
      PolygonType::Merge(m_Walls);
      PolygonType::Merge(m_Floor);

      /*for(unsigned int i = 0;i<m_Blocks.size();++i)
      {
        if(m_Blocks[i] != NULL)
          eXl_DELETE m_Blocks[i];
      }
      m_Blocks.clear();
      for(unsigned int i = 0;i<newPolyWall.size();++i)
      {
        TileBlock* newBlock = eXl_NEW TileBlock(NULL,TileNode::Wall);
        newBlock->SwapPolygon(newPolyWall[i]);
        m_Blocks.push_back(newBlock);
      }
      for(unsigned int i = 0;i<newPolyFloor.size();++i)
      {
        TileBlock* newBlock = eXl_NEW TileBlock(NULL,TileNode::Floor);
        newBlock->SwapPolygon(newPolyFloor[i]);
        m_Blocks.push_back(newBlock);
      }*/

    }


    void TileMap::CommitOperation(Operation const& iOp)
    {

    }

    struct BorderStruct
    {
      //TileLoc const* intBorder;
      //TileLoc const* extBorder;
      TileLoc const* border;
      TileLoc const* intCorner;
      TileLoc const* extCorner;
      unsigned int thickness;
    };

    void TileMap::BrowseLine(PolygonType::PtList const& iList, BorderStruct const& iDesc, OGLSpriteBatcher& oBatcher)
    {
      if (iList.size() > 0)
      {
        unsigned int prevIdx = 0;
        unsigned int curIdx = 1;

        Vector2i curDir = iList[curIdx] - iList[prevIdx];

        unsigned int nextIdx = curIdx + 1;

        //Vector2i lastPoint = iList.back();
        Vector2i nextDir = (iList[nextIdx] - iList[curIdx]);
        Vector2i normDir = nextDir + curDir;

        normDir.X() = (normDir.X() > 0 ? -1.0 : 1.0);
        normDir.Y() = (normDir.Y() > 0 ? -1.0 : 1.0);
        normDir = Vector2i(-normDir.Y(), normDir.X());

        Vector2i curInside = iList[curIdx] + normDir * iDesc.thickness;
        int sign = (nextDir.X() * curDir.Y() - nextDir.Y() * curDir.X()) * -1.0;
        curDir = nextDir;

        for (; curIdx < iList.size() + (iList[0] == iList.back() ? 0 : 1); ++curIdx)
        {
          Vector2i curPoint = iList[curIdx == iList.size() ? 0 : curIdx];

          curDir = Vector2i(curDir.X() != 0 ? (curDir.X() > 0 ? 1 : -1) : 0, curDir.Y() != 0 ? (curDir.Y() > 0 ? 1 : -1) : 0)*iDesc.thickness;

          TileLoc const* sprite = sign > 0 ? iDesc.intCorner : iDesc.extCorner;

          {
            Vector2f insidePt(curInside.X(), curInside.Y());
            Vector2f curPt(curPoint.X(), curPoint.Y());
            Vector2f size = (insidePt - curPt);
            size.X() = size.X() > 0.0 ? size.X() : -size.X();
            size.Y() = size.Y() > 0.0 ? size.Y() : -size.Y();
            /*Vector2f ptInt1 = insidePt + Vector2f(curDir.X(),curDir.Y()) * (sign > 0 ? 1.0:-1.0);
            Vector2f ptInt2 = curPt - Vector2f(curDir.X(),curDir.Y()) * (sign > 0 ? 1.0:-1.0);*/

            unsigned int texRot = 0;
            if (sign > 0)
            {
              texRot = normDir.X() > 0 ? (normDir.Y() > 0 ? SpriteDesc::Rot_90 : SpriteDesc::Rot_180) : (normDir.Y() > 0 ? SpriteDesc::Rot_0 : SpriteDesc::Rot_270);
            }
            else
            {
              texRot = normDir.X() > 0 ? (normDir.Y() > 0 ? SpriteDesc::Rot_270 : SpriteDesc::Rot_0) : (normDir.Y() > 0 ? SpriteDesc::Rot_180 : SpriteDesc::Rot_90);
            }

            AABB2Df dest = AABB2Df::FromCenterAndSize((insidePt + curPt) / 2.0, size);
            //iArray->AddTextureDesc(sprite->m_Tex,dest,AABB2Df(sprite->m_TexLoc),texRot);
            oBatcher.AddSprite(this, sprite->m_Tex.get(), 1.0, 0, texRot, dest, AABB2Df(sprite->m_TexLoc));
          }

          unsigned int nextIdx = (curIdx + 1) % iList.size();
          Vector2i nextPt = iList[nextIdx];
          if (nextPt == curPoint)
          {
            nextIdx = (nextIdx + 1) % iList.size();
            nextPt = iList[nextIdx];
          }

          unsigned int nextNextIdx = (nextIdx + 1) % iList.size();
          if (nextPt == iList[nextNextIdx])
          {
            nextNextIdx = (nextNextIdx + 1) % iList.size();
          }

          nextDir = iList[nextNextIdx] - nextPt;


          normDir = nextDir + curDir;
          normDir.X() = (normDir.X() > 0 ? -1.0 : 1.0);
          normDir.Y() = (normDir.Y() > 0 ? -1.0 : 1.0);
          normDir = Vector2i(-normDir.Y(), normDir.X());

          Vector2i nextInside = nextPt + normDir * iDesc.thickness;

          sign = (nextDir.X() * curDir.Y() - nextDir.Y() * curDir.X()) * -1.0;

          if (nextInside != curInside)
          {
            Vector2f insidePt = curDir.Dot((curInside - curPoint)) > 0 ? Vector2f(curInside.X(), curInside.Y()) : Vector2f(curInside.X() + curDir.X(), curInside.Y() + curDir.Y());
            Vector2f curPt = curDir.Dot((curInside - curPoint)) > 0 ? Vector2f(curPoint.X() + curDir.X(), curPoint.Y() + curDir.Y()) : Vector2f(curPoint.X(), curPoint.Y());
            Vector2f tipPt = curDir.Dot((nextInside - nextPt)) > 0 ? Vector2f(nextPt.X(), nextPt.Y()) : Vector2f(nextPt.X() - curDir.X(), nextPt.Y() - curDir.Y());
            //Vector3f tipInside = curDir.Dot((nextInside-nextPt)) > 0 ? Vector3f(nextInside.X() - curDir.X(),nextInside.Y() - curDir.Y(),-10.0):Vector3f(nextInside.X(), nextInside.Y(),-10.0);;

            sprite = iDesc.border;

            float bordLength = (curPt - tipPt).Length() / m_WallSet.unitFloor;
            Vector2f size = insidePt - tipPt;

            unsigned int texRot = 0;
            if (sign > 0)
            {
              texRot = nextDir.X() != 0 ? (nextDir.X() > 0 ? SpriteDesc::Rot_0 : SpriteDesc::Rot_180) : (nextDir.Y() > 0 ? SpriteDesc::Rot_270 : SpriteDesc::Rot_90);
            }
            else
            {
              texRot = nextDir.X() != 0 ? (nextDir.X() > 0 ? SpriteDesc::Rot_180 : SpriteDesc::Rot_0) : (nextDir.Y() > 0 ? SpriteDesc::Rot_90 : SpriteDesc::Rot_270);
            }

            size.X() = size.X() > 0.0 ? size.X() : -size.X();
            size.Y() = size.Y() > 0.0 ? size.Y() : -size.Y();

            //unsigned int texRot = normDir.X() > 0 ? (normDir.Y() > 0 ? GfxSprite::Rot_180 : GfxSprite::Rot_270) : (normDir.Y() > 0 ? GfxSprite::Rot_90 : GfxSprite::Rot_0);



            Vector2f texSize = Vector2f(sprite->m_TexLoc.MaxX() - sprite->m_TexLoc.MinX(), sprite->m_TexLoc.MaxY() - sprite->m_TexLoc.MinY());
            texSize.Y() *= bordLength;
            AABB2Df texLoc(sprite->m_TexLoc.MinX(), sprite->m_TexLoc.MinY(), sprite->m_TexLoc.MaxX(), sprite->m_TexLoc.MinY() + texSize.Y());
            //iArray->AddTextureDesc(sprite->m_Tex,AABB2Df((insidePt+tipPt) / 2.0, size),texLoc,texRot);
            oBatcher.AddSprite(this, sprite->m_Tex.get(), 1.0, 0, texRot, AABB2Df::FromCenterAndSize((insidePt + tipPt) / 2.0, size), texLoc);

          }

          curDir = nextDir;
          curInside = nextInside;
        }
      }
    }

    void TileMap::EmptyMapView()
    {

    }

    Err TileMap::UpdateMapView(ObjectHandle iMapHandle, GfxSystem& iGfxSys, PhysicsSystem& iPhxSys)
    {
      EmptyMapView();

      OGLSpriteBatcher batcher(0.0, Vector3f::ZERO);
      PhysicInitData walls;
      walls.SetFlags(PhysicFlags::Static);

      for (int i = 0; i < m_Walls.size(); ++i)
      {
        Vector<AABB2Di> boxes;
        m_Walls[i].GetBoxes(boxes);

        for (unsigned int j = 0; j < boxes.size(); ++j)
        {
          Vector2i origi(boxes[j].m_Data[0] + boxes[j].m_Data[1]);
          Vector2i sizei(boxes[j].m_Data[1] - boxes[j].m_Data[0]);
          Vector3f orig(origi.X() / 2.0, origi.Y() / 2.0, 0.0);
          Vector3f size(sizei.X(), sizei.Y(), 20.0);
          //btCollisionShape* shape = tileBlock->GetCollisionObject()->getCollisionShape();


          /*if(shape -> getShapeType() == BOX_SHAPE_PROXYTYPE)
          {
            btBoxShape* box = (btBoxShape*)shape;
            btVector3 dim = box->getHalfExtentsWithoutMargin();
            btInit->AddBox(Vector3f(dim.x()*2.0,dim.y()*2.0,20.0));
          }*/
          //btInit->AddBox(size,orig);
          walls.AddBox(size, orig);
        }

        Tile_OLD* tileExtCornerWall = m_WallSet.WallExtCorner;
        Tile_OLD* tileIntCornerWall = m_WallSet.WallIntCorner;
        Tile_OLD* tileBorderWall = m_WallSet.Wall;
        Tile_OLD* tileFill = m_WallSet.Fill;

        //Border.
        {
          BorderStruct borders;
          borders.border = tileBorderWall->GetTileLoc();
          borders.extCorner = tileExtCornerWall->GetTileLoc();
          borders.intCorner = tileIntCornerWall->GetTileLoc();
          borders.thickness = m_WallSet.wall_Thickness / 2;

          if (m_Walls[i].GetAABB() != m_Guard.GetAABB())
          {
            PolygonType::PtList const& extLine = m_Walls[i].Border();

            BrowseLine(extLine, borders, batcher);

          }

          for (unsigned int j = 0; j < m_Walls[i].Holes().size(); ++j)
          {
            BrowseLine(m_Walls[i].Holes()[j], borders, batcher);
          }
        }

        boxes.clear();

        Vector<PolygonType> center;
        m_Walls[i].Shrink(m_WallSet.wall_Thickness, center);

        if (m_Walls[i].GetAABB() == m_Guard.GetAABB())
        {
          Vector2i origBorder = (m_Size + m_GuardSize * 3) / 8;
          Vector2i sizeBorder = (m_GuardSize - m_Size) / 4;
          AABB2Di rightBox(Vector2i(origBorder.X(), 0), Vector2i(sizeBorder.X(), m_GuardSize.Y()));
          AABB2Di leftBox(Vector2i(-origBorder.X(), 0), Vector2i(sizeBorder.X(), m_GuardSize.Y()));
          AABB2Di upperBox(Vector2i(0, origBorder.Y()), Vector2i(m_GuardSize.X(), sizeBorder.Y()));
          AABB2Di lowerBox(Vector2i(0, -origBorder.Y()), Vector2i(m_GuardSize.X(), sizeBorder.Y()));

          PolygonType guardPoly(rightBox);
          guardPoly.AddBox(upperBox);
          guardPoly.AddBox(lowerBox);
          guardPoly.AddBox(leftBox);
          if (center.size() == 0)
          {
            center.push_back(PolygonType());
            guardPoly.Swap(center[0]);
          }
          else
          {
            for (unsigned int i = 0; i < center.size(); ++i)
            {
              PolygonType res;
              center[i].Union(guardPoly, res);
              if (!res.Empty())
                center[i].Swap(res);
            }
          }
        }

        for (unsigned int i = 0; i < center.size(); ++i)
        {
          Vector<AABB2Di> tempBoxes;
          center[i].GetBoxes(tempBoxes);
          boxes.insert(boxes.end(), tempBoxes.begin(), tempBoxes.end());
        }


        //*GetFieldCheckPtr<Vector3f>(&data,"Position") = Vector3f(0.0,0.0,0.0);
        /*
        Vector3f* globPos = (Vector3f*)malloc(6*sizeof(Vector3f)*boxes.size());
        Vector2f* globTc = (Vector2f*)malloc(6*sizeof(Vector3f)*boxes.size());

        ogreMesh->SetPosition(3,6*boxes.size(),(float*)globPos);
        ogreMesh->SetTC(3,(float*)globTc);
        ogreMesh->SetTopology(3,OgreMesh::TriangleList);
        */
        for (unsigned int j = 0; j < boxes.size(); ++j)
        {
          AABB2Df dest(boxes[j]);
          AABB2Df texLoc(tileFill->GetTileLoc()->m_TexLoc);
          Vector2i sizei(boxes[j].m_Data[1] - boxes[j].m_Data[0]);
          Vector2f sizeTex(texLoc.m_Data[1] - texLoc.m_Data[0]);
          texLoc = AABB2Df(texLoc.MinX(), texLoc.MinY(), texLoc.MinX() + sizeTex.X() * sizei.X(), texLoc.MinY() + sizeTex.Y() * sizei.Y());

          //spriteArray->AddTextureDesc(tileFill->GetTileLoc()->m_Tex,dest,texLoc);
          batcher.AddSprite(this, tileFill->GetTileLoc()->m_Tex.get(), 1.0, 0, 0, dest, texLoc);
        }
      }

      for (int i = 0; i < m_Floor.size(); ++i)
      {
        Tile_OLD* tileExtCornerFloor = m_WallSet.FloorExtCorner;
        Tile_OLD* tileIntCornerFloor = m_WallSet.FloorIntCorner;
        Tile_OLD* tileBorderFloor = m_WallSet.FloorBorder;
        Tile_OLD* tileFloor = m_WallSet.Floor;

        //Border.
        {
          BorderStruct borders;
          borders.border = tileBorderFloor->GetTileLoc();
          borders.extCorner = tileExtCornerFloor->GetTileLoc();
          borders.intCorner = tileIntCornerFloor->GetTileLoc();

          borders.thickness = m_WallSet.unitFloor / 2;

          {
            PolygonType::PtList const& extLine = m_Floor[i].Border();
            BrowseLine(extLine, borders, batcher);
          }

          for (unsigned int j = 0; j < m_Floor[i].Holes().size(); ++j)
          {
            BrowseLine(m_Floor[i].Holes()[j], borders, batcher);
          }
        }

        Vector<AABB2Di> boxes;
        Vector<PolygonType> center;
        m_Floor[i].Shrink(m_WallSet.unitFloor, center);
        for (unsigned int i = 0; i < center.size(); ++i)
        {
          Vector<AABB2Di> tempBoxes;
          center[i].GetBoxes(tempBoxes);
          boxes.insert(boxes.end(), tempBoxes.begin(), tempBoxes.end());
        }

        //*GetFieldCheckPtr<Vector3f>(&data,"Position") = Vector3f(0.0,0.0,0.0);

        Vector3f* globPos = (Vector3f*)malloc(6 * sizeof(Vector3f)*boxes.size());
        Vector2f* globTc = (Vector2f*)malloc(6 * sizeof(Vector3f)*boxes.size());

        Tile_OLD* tile = m_WallSet.Floor;

        for (unsigned int j = 0; j < boxes.size(); ++j)
        {
          AABB2Df dest(boxes[j]);
          AABB2Df texLoc(tile->GetTileLoc()->m_TexLoc);
          Vector2i sizei(boxes[j].m_Data[1] - boxes[j].m_Data[0]);
          Vector2f sizeTex(texLoc.m_Data[1] - texLoc.m_Data[0]);
          texLoc = AABB2Df(texLoc.MinX(), texLoc.MinY(), texLoc.MinX() + sizeTex.X() * sizei.X(), texLoc.MinY() + sizeTex.Y() * sizei.Y());

          //spriteArray->AddTextureDesc(tile->GetTileLoc()->m_Tex,dest,texLoc);
          batcher.AddSprite(this, tile->GetTileLoc()->m_Tex.get(), 1.0, 0, 0, dest, texLoc);
        }
      }

      batcher.Update();

      Vector<OGLSpriteBatcher::SpriteElement> elems;
      Vector<OGLSpriteBatcher::VertexChunk> chunks;
      OGLBuffer* buffer;
      OGLVAssembly assembly;
      batcher.StealSprites(elems, chunks, assembly, buffer);

      if (buffer && !chunks.empty())
      {
        Matrix4f identMatrix;
        identMatrix.MakeIdentity();

        auto geom = eXl_NEW GeometryInfo;
        geom->m_Assembly = assembly;
        geom->m_Vertices = buffer;
        geom->m_Command = OGLDraw::TriangleList;

        GfxComponent& comp = iGfxSys.CreateComponent(iMapHandle);
        comp.SetGeometry(geom);
        comp.SetTransform(identMatrix);
				
        unsigned int vertexStart = 0;
        for (auto& chunk : chunks)
        {
          OGLTexture const* tex = chunk.spriteDesc->GetTexture();
				
          auto mat = eXl_NEW SpriteMaterialInfo;
          mat->m_SpriteInfo.alphaMult = 1.0;
          mat->m_SpriteInfo.tint = Vector4f::ONE;
					mat->m_SpriteInfo.tcOffset = Vector2f::ZERO;
					mat->m_SpriteInfo.tcScaling = Vector2f::ONE;
          mat->m_Texture = tex;
          mat->SetupData();
				
				
          comp.AddDraw(mat, chunk.vertexCount, vertexStart);
          vertexStart += chunk.vertexCount;
        }

        iPhxSys.CreateComponent(iMapHandle, walls);

        return Err::Success;
      }
      return Err::Failure;
    }
  }
}