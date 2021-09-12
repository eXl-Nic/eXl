/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/map/map.hpp>

#include <math/mathtools.hpp>
#include <engine/game/commondef.hpp>
#include <engine/common/transforms.hpp>
#include <engine/map/maptiler.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/pathfinding/navmesh.hpp>
#include <engine/pathfinding/navigator.hpp>

namespace eXl
{
  MapResource::InstanceData MapResource::Instantiate(World& iWorld, Matrix4f const& iPos) const
  {
    InstanceData allObjects;

    Transforms* trans = iWorld.GetSystem<Transforms>();
    GameDatabase* database = iWorld.GetSystem<GameDatabase>();
    eXl_ASSERT_REPAIR_RET(trans != nullptr, allObjects);
    eXl_ASSERT_REPAIR_RET(database != nullptr, allObjects);

    GfxSystem* gfx = iWorld.GetSystem<GfxSystem>();

    Vector<MapTiler::Batcher> tilesByLayer;
    Vector<UnorderedMap<TilingGroup const*, MapTiler::Blocks>> blocksByLayer;
    UnorderedMap<TerrainTypeName, Vector<AABB2DPolygoni>> components;
    AABB2Di fullSize;

    for (auto const& terrainItem : m_Terrains)
    {
      Vector2i tilingSize = Vector2i::ONE * EngineCommon::s_WorldToPixel;
      TilingGroup const* group = terrainItem.m_TilingGroup.GetOrLoad();
      if (group != nullptr)
      {
        Tileset const* groupTileset = group->GetTileset().GetOrLoad();
        if (groupTileset != nullptr)
        {
          Tile const* tile = groupTileset->Find(group->m_DefaultTile);
          if (tile != nullptr)
          {
            tilingSize = tile->m_Size;
            for (auto const& terrainBlock : terrainItem.m_Blocks)
            {
              blocksByLayer.resize(terrainBlock.m_Layer + 1);
              auto& curLayer = blocksByLayer[terrainBlock.m_Layer];
              auto iter = curLayer.insert(std::make_pair(group, MapTiler::Blocks())).first;
              MapTiler::Blocks& block = iter->second;
              block.group = group;
              block.islands.push_back(terrainBlock.m_Shape);
              if (fullSize.Empty())
              {
                fullSize = block.islands.back().GetAABB();
              }
              else
              {
                fullSize.Absorb(block.islands.back().GetAABB());
              }
            }
          }
          else
          {
            LOG_ERROR << "Missing tile " << group->m_DefaultTile << " in tileset " << groupTileset->GetName();
          }
        }
        else
        {
          LOG_ERROR << "Could not load tileset " << group->GetTileset().GetUUID().ToString();
        }
      }
      else
      {
        LOG_ERROR << "Could not load tilegorup " << terrainItem.m_TilingGroup.GetUUID().ToString();
      }

      Vector2f worldScaling(tilingSize.X() / EngineCommon::s_WorldToPixel, tilingSize.Y() / EngineCommon::s_WorldToPixel);
      auto insertRes = components.insert(std::make_pair(terrainItem.m_Type, Vector<AABB2DPolygoni>()));
      auto& blocks = insertRes.first->second;
      for (auto const& terrainBlock : terrainItem.m_Blocks)
      {
        blocks.push_back(terrainBlock.m_Shape);
        blocks.back().ScaleComponents(tilingSize.X(), tilingSize.Y(), 1, 1);
      }
        //auto insertRes = physicLayer.insert(std::make_pair(terrainItem.m_Type, PhysicInitData()));
        //PhysicInitData& phData = insertRes.first->second;
        //if (insertRes.second)
        //{
        //  phData.SetCategory(terrainDesc.m_PhysicCategory, terrainDesc.m_PhysicFilter);
        //  phData.SetFlags(EngineCommon::s_BasePhFlags | PhysicFlags::Static);
        //}
        //for (auto const& terrainBlock : terrainItem.m_Blocks)
        //{
        //  Vector<AABB2Di> blockBoxes;
        //  terrainBlock.m_Shape.GetBoxes(blockBoxes);
        //
        //  for (unsigned int j = 0; j < blockBoxes.size(); ++j)
        //  {
        //    Vector2i origi(blockBoxes[j].m_Data[0] + blockBoxes[j].m_Data[1]);
        //    Vector2i sizei(blockBoxes[j].m_Data[1] - blockBoxes[j].m_Data[0]);
        //    Vector3f orig((float)origi.X() * 0.5f * worldScaling.X()
        //      , (float)origi.Y() * 0.5f * worldScaling.Y()
        //      , terrainDesc.m_Altitude + terrainDesc.m_Height * 0.5f);
        //    Vector3f size(sizei.X() * worldScaling.X(), sizei.Y() * worldScaling.Y(), terrainDesc.m_Height);
        //    phData.AddBox(size, orig + MathTools::GetPosition(iPos));
        //  }
        //}
        
    }

    if (!fullSize.Empty())
    {
      fullSize.m_Data[0] -= Vector2i::ONE;
      fullSize.m_Data[1] += Vector2i::ONE;
      if (gfx != nullptr)
      {
        for (uint32_t layerIdx = 0; layerIdx < blocksByLayer.size(); ++layerIdx)
        {
          MapTiler::Batcher batcher;
          auto const& layer = blocksByLayer[layerIdx];
          for (auto& blockEntry : layer)
          {
            MapTiler::ComputeGfxForBlock(batcher, fullSize, blockEntry.second);
          }
          ObjectHandle layerView = iWorld.CreateObject();
          allObjects.terrain.push_back(layerView);
          trans->AddTransform(layerView, &iPos);
          batcher.Finalize(*gfx, layerView, layerIdx);
        }
      }
    }

    for (auto const& tiles : m_Tiles)
    {
      Tileset const* tileset = tiles.m_Tileset.GetOrLoad();
      if (tileset != nullptr)
      {
        for (auto const& tile : tiles.m_Tiles)
        {
          Tile const* tileDesc = tileset->Find(tile.m_Name);
          if (tileDesc != nullptr)
          {
            Vector2f tilePos = MathTools::ToFVec(tile.m_Position) / EngineCommon::s_WorldToPixel;
            Vector2f phSize = MathTools::ToFVec(tileDesc->m_Size) / EngineCommon::s_WorldToPixel;

            auto insertRes = components.insert(std::make_pair(tiles.m_Type, Vector<AABB2DPolygoni>()));
            auto& blocks = insertRes.first->second;
            AABB2Di tileBox(tile.m_Position - tileDesc->m_Size / 2, tileDesc->m_Size);
            blocks.push_back(AABB2DPolygoni(tileBox));

            TerrainType terrainDesc = TerrainType::GetTerrainTypeFromName(tiles.m_Type);
            
            if (gfx != nullptr)
            {
              if (tileDesc->m_Frames.size() == 1)
              {
                MapTiler::TexGroup texGroup;
                texGroup.m_Name = tileDesc->m_ImageName;
                texGroup.m_Tileset = tileset;

                Vector2i imgSize = tileset->GetImageSize(tileDesc->m_ImageName);
                Vector2i tileOffset = tileDesc->m_Frames.size() > 0 ? tileDesc->m_Frames[0] : Vector2i::ZERO;
                Vector2f scale(float(tileDesc->m_Size.X()) / imgSize.X(), float(tileDesc->m_Size.Y()) / imgSize.Y());
                Vector2f offset(float(tileOffset.X()) / imgSize.X(), float(tileOffset.Y()) / imgSize.Y());

                texGroup.m_VtxOffset = offset;
                texGroup.m_VtxScaling = scale;
                texGroup.m_Tiling = scale;

                tilesByLayer.resize(tile.m_Layer + 1);

                Vector<float>& curGroup = tilesByLayer[tile.m_Layer].GetGroup(texGroup);

                float vtxData[][5] =
                {
                  {-0.5, -0.5, 0.0, 0.0, 1.0},
                  { 0.5, -0.5, 0.0, 1.0, 1.0},
                  {-0.5,  0.5, 0.0, 0.0, 0.0},
                  {-0.5,  0.5, 0.0, 0.0, 0.0},
                  { 0.5, -0.5, 0.0, 1.0, 1.0},
                  { 0.5,  0.5, 0.0, 1.0, 0.0},
                };
                for (auto const& vtx : vtxData)
                {
                  curGroup.push_back(phSize.X() * vtx[0] + tilePos.X());
                  curGroup.push_back(phSize.Y() * vtx[1] + tilePos.Y());
                  curGroup.push_back(vtx[2] + terrainDesc.m_Altitude);
                  curGroup.push_back(vtx[3]);
                  curGroup.push_back(vtx[4]);
                }
              }
              else
              {
                ObjectHandle animatedSprite = iWorld.CreateObject();
                allObjects.tiles.push_back(animatedSprite);
                Matrix4f objTrans = iPos * Matrix4f::FromPosition(MathTools::To3DVec(tilePos));
                trans->AddTransform(animatedSprite, &objTrans);
                GfxSpriteComponent& spriteComp = gfx->CreateSpriteComponent(animatedSprite);
                spriteComp.SetFlat(true);
                spriteComp.SetTileset(tileset);
                spriteComp.SetTileName(tile.m_Name);
                spriteComp.SetLayer(tile.m_Layer);
              }
            }
          }
          else
          {
            LOG_ERROR << "Could not find tile " << tile.m_Name << " in tileset " << tileset->GetName();
          }
        }
      }
      else
      {
        LOG_ERROR << "Could not load tileset " << tiles.m_Tileset.GetUUID().ToString();
      }
    }

    allObjects.objects.reserve(m_Objects.size());
    for (auto const& object : m_Objects)
    {
      ObjectCreationInfo creationInfo;
      creationInfo.m_DisplayName = object.m_Header.m_DisplayName;
      creationInfo.m_PersistentId = object.m_Header.m_ObjectId;

      ObjectHandle newObject = iWorld.CreateObject(std::move(creationInfo));
      allObjects.objects.push_back(newObject);
      if (trans != nullptr)
      {
        Matrix4f objTrans = iPos * Matrix4f::FromPosition(object.m_Header.m_Position);
        trans->AddTransform(newObject, &objTrans);
      }
    }

    for (uint32_t i = 0; i < m_Objects.size(); ++i)
    {
      auto const& objectDesc = m_Objects[i];
      ObjectHandle mapObject = allObjects.objects[i];
      Archetype const* objArchetype = objectDesc.m_Header.m_Archetype.GetOrLoad();
      if (objArchetype != nullptr)
      {
        objArchetype->Instantiate(mapObject, iWorld, &objectDesc.m_Data);
      }
      else
      {
        LOG_ERROR << "Archetype missing for object " << objectDesc.m_Header.m_DisplayName << ", id : " << objectDesc.m_Header.m_ObjectId;
      }

      auto const* carver = database->GetData<EngineCommon::TerrainCarver>(mapObject, EngineCommon::TerrainCarver::PropertyName());
      if (carver != nullptr)
      {
        Vector2i offset(carver->m_Shape.m_Offset.X() * EngineCommon::s_WorldToPixel, carver->m_Shape.m_Offset.Y() * EngineCommon::s_WorldToPixel);
        offset += MathTools::ToIVec(MathTools::As2DVec(objectDesc.m_Header.m_Position) * EngineCommon::s_WorldToPixel);
        Vector2i size;
        switch (carver->m_Shape.m_Type)
        {
        case EngineCommon::PhysicsShapeType::Box:
          size = Vector2i(carver->m_Shape.m_Dims.X(), carver->m_Shape.m_Dims.Y()) * EngineCommon::s_WorldToPixel;
          break;
        case EngineCommon::PhysicsShapeType::Sphere:
          size = Vector2i(carver->m_Shape.m_Dims.X(), carver->m_Shape.m_Dims.X()) * EngineCommon::s_WorldToPixel;
          break;
        }
        
        AABB2Di pixelBox(offset - size / 2, size);

        Vector<AABB2DPolygoni> diffResult;
        for (auto& entry : components)
        {
          if (entry.first == carver->m_TerrainType)
          {
            entry.second.push_back(AABB2DPolygoni(pixelBox));
            continue;
          }
          Vector<AABB2DPolygoni> newComponents;
          for (auto& poly : entry.second)
          {
            if (poly.GetAABB().Intersect(pixelBox))
            {
              diffResult.clear();
              poly.Difference(pixelBox, diffResult);

              if (diffResult.size() == 0)
              {
                poly.Clear();
              }
              if (diffResult.size() == 1)
              {
                poly = std::move(diffResult[0]);
              }
              if (diffResult.size() > 1)
              {
                poly.Clear();
                for (auto& newComps : diffResult)
                {
                  newComponents.push_back(std::move(newComps));
                }
              }
            }
          }
          for (int32_t i = 0; i<(int32_t)entry.second.size(); ++i)
          {
            auto& poly = entry.second[i];
            if (poly.Empty())
            {
              poly = std::move(entry.second.back());
              entry.second.pop_back();
              --i;
            }
          }
          for (auto& newComp : newComponents)
          {
            entry.second.push_back(std::move(newComp));
          }
        }
      }
    }

    for (auto& entry : components)
    {
      AABB2DPolygoni::Merge(entry.second);
    }

    if (PhysicsSystem* physics = iWorld.GetSystem<PhysicsSystem>())
    {
      for (auto const& entry : components)
      {
        TerrainType terrainDesc = TerrainType::GetTerrainTypeFromName(entry.first);
        if (terrainDesc.m_Height > Mathf::ZERO_TOLERANCE)
        {
          for (auto const& poly : entry.second)
          {
            Vector<AABB2Di> boxes;
            poly.GetBoxes(boxes);
            for (auto const& box : boxes)
            {
              ObjectHandle terrainPhysics = iWorld.CreateObject();
              allObjects.terrain.push_back(terrainPhysics);

              PhysicInitData phData;
              phData.SetCategory(terrainDesc.m_PhysicCategory, terrainDesc.m_PhysicFilter);
              phData.SetFlags(EngineCommon::s_BasePhFlags | PhysicFlags::Static);
              Vector2f phSize = MathTools::ToFVec(box.GetSize()) / EngineCommon::s_WorldToPixel;
              phData.AddBox(MathTools::To3DVec(phSize, terrainDesc.m_Height));
              
              Vector2f tilePos = MathTools::ToFVec(box.GetCenter()) / EngineCommon::s_WorldToPixel;
              Vector3f orig = MathTools::To3DVec(tilePos, terrainDesc.m_Altitude + terrainDesc.m_Height * 0.5f);
              Matrix4f boxPos = iPos;
              MathTools::GetPosition(boxPos) += orig;
              trans->AddTransform(terrainPhysics, &boxPos);
              physics->CreateComponent(terrainPhysics, phData);
            }
          }
        }
      }
    }

    if (gfx != nullptr)
    {
      for (uint32_t layerIdx = 0; layerIdx < tilesByLayer.size(); ++layerIdx)
      {
        auto& layer = tilesByLayer[layerIdx];
        ObjectHandle layerView = iWorld.CreateObject();
        allObjects.tiles.push_back(layerView);
        trans->AddTransform(layerView, &iPos);
        layer.Finalize(*gfx, layerView, layerIdx);
      }
    }

    if (NavigatorSystem* navSys = iWorld.GetSystem<NavigatorSystem>())
    {
      GameDataView<PhysicInitData>* view = GetPhysicsInitDataView(iWorld);

      Vector<AABB2Di> tileObstacles;

      auto extractNavMeshBox = [&tileObstacles, view, trans](ObjectHandle obj)
      {
        if (auto phData = view->Get(obj))
        {
          if (phData->GetFlags() & PhysicFlags::Static)
          {
            Matrix4f const& pos = trans->GetWorldTransform(obj);
            for (auto const& geom : phData->GetGeoms())
            {
              Vector2i pixelMin = MathTools::ToIVec(MathTools::As2DVec((MathTools::GetPosition(pos) + geom.position) * EngineCommon::s_WorldToPixel));
              Vector2i shapeSize;
              switch (geom.shape)
              {
              case GeomDef::Sphere:
              case GeomDef::Capsule:
                shapeSize = Vector2i(geom.geomData.X(), geom.geomData.X()) * EngineCommon::s_WorldToPixel;
                break;
              case GeomDef::Box:
                shapeSize = Vector2i(geom.geomData.X(), geom.geomData.Y()) * EngineCommon::s_WorldToPixel;
                break;
              }

              pixelMin -= shapeSize / 2;
              AABB2Di pixelBox(pixelMin, shapeSize);
              tileObstacles.push_back(pixelBox);
            }
          }
        }
      };

      for (auto obj : allObjects.terrain)
      {
        extractNavMeshBox(obj);
      }

      for (auto obj : allObjects.objects)
      {
        extractNavMeshBox(obj);
        if (auto phData = view->Get(obj))
        {
          if ((phData->GetFlags() & PhysicFlags::Static) == 0)
          {
            // Insert obstactle ?

          }
        }
      }

      Vector<AABB2DPolygoni> floor;
      auto iter = components.find(TerrainTypeName("Floor"));
      if (iter != components.end())
      {
        floor = std::move(iter->second);
      }

      Vector<AABB2DPolygoni> resVec;
      for (int32_t i = 0; i < (int32_t)floor.size(); ++i)
      {
        for (auto const& obstacle : tileObstacles)
        {
          floor[i].Difference(obstacle, resVec);
          if (resVec.size() == 0)
          {
            floor.erase(floor.begin() + i);
            --i;
            break;
          }
          else if (resVec.size() >= 1)
          {
            std::swap(floor[i], resVec[0]);
            for (uint32_t j = 1; j < resVec.size(); ++j)
            {
              floor.push_back(std::move(resVec[j]));
            }
          }
          resVec.clear();
        }
      }

      for (auto& floorPiece : floor)
      {
        floorPiece.Scale(1, EngineCommon::s_WorldToPixel);
      }

      allObjects.navMesh = std::make_unique<NavMesh>(NavMesh::MakeFromAABB2DPoly(floor));
      navSys->SetNavMesh(*allObjects.navMesh);
    }

    return allObjects;
  }
}