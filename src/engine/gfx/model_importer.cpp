/*
Copyright 2022 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/model_importer.hpp>
#include <engine/gfx/model.hpp>

#include <ogl/renderer/oglbuffer.hpp>
#include <ogl/oglmeshalgo.hpp>

#include <core/image/image.hpp>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

namespace eXl
{
  namespace
  {
    unsigned int const ppsteps =
      aiProcess_CalcTangentSpace | // calculate tangents and bitangents if possible
      aiProcess_JoinIdenticalVertices | // join identical vertices/ optimize indexing
      aiProcess_ValidateDataStructure | // perform a full validation of the loader's output
      aiProcess_ImproveCacheLocality | // improve the cache locality of the output vertices
      aiProcess_RemoveRedundantMaterials | // remove redundant materials
      aiProcess_FindDegenerates | // remove degenerated polygons from the import
      aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
      aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
      aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
      aiProcess_FindInstances | // search for instanced meshes and remove them by references to one master
      aiProcess_LimitBoneWeights | // limit bone weights to 4 per vertex
      aiProcess_OptimizeMeshes | // join small meshes, if possible;
      aiProcess_SplitByBoneCount | // split meshes with too many bones.
      aiProcess_GenSmoothNormals | // generate smooth normal vectors if not existing
      aiProcess_SplitLargeMeshes | // split large, unrenderable meshes into submeshes
      aiProcess_Triangulate | // triangulate polygons with more than 3 edges
      aiProcess_SortByPType | // make 'clean' meshes which consist of a single typ of primitives
      0;

  }

  /**
  * Informations about a given mesh
  */
  struct MeshInfo
  {
    uint32_t numVtx = 0;
    uint32_t numIdx = 0;

    bool hasNormals = false;
    bool hasUV = false;
    bool hasTangentSpace = false;

    void Combine(MeshInfo const& iInfos)
    {
      hasNormals = iInfos.hasNormals;
      hasUV = iInfos.hasUV;
      hasTangentSpace = iInfos.hasTangentSpace;

      numVtx += iInfos.numVtx;
      numIdx += iInfos.numIdx;
    }

    uint32_t VertexSizeInFloats()
    {
      uint32_t vertexSize = 3;
      if (hasNormals)
      {
        vertexSize += 3;
      }
      if (hasUV)
      {
        vertexSize += 2;
      }
      if (hasTangentSpace)
      {
        vertexSize += 6;
      }

      return vertexSize;
    }
  };

  // Gather information about mesh data
  MeshInfo GetMeshInfos(aiMesh const& iMesh)
  {
    if ((iMesh.mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0)
    {
      return MeshInfo();
    }

    MeshInfo info;

    info.hasNormals |= iMesh.HasNormals();
    info.hasUV |= iMesh.HasTextureCoords(0);
    info.hasTangentSpace |= false & iMesh.HasTangentsAndBitangents();

    info.numVtx = iMesh.mNumVertices;
    info.numIdx = iMesh.mNumFaces * 3;

    return info;
  }

  // Transfer mesh data into the appropriate buffer.
  // Compute the mesh's AABB as well.
  Box3D ReadMesh(aiMesh const& iMesh, MeshInfo const& iInfo
    , uint32_t iVertexSize, uint32_t iBaseIdx
    , float* oVtxData, uint32_t* oIdxData, Mat4 iTransform)
  {
    aiVector3D const nullVec(0, 0, 0);
    if ((iMesh.mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0)
    {
      return Box3D();
    }

    Box3D meshBox;

    for (uint32_t i = 0; i < iMesh.mNumVertices; ++i)
    {
      //memcpy(oVtxData, iMesh.mVertices + i, sizeof(aiVector3D));
      *reinterpret_cast<Vec3*>(oVtxData) = iTransform * Vec4(*reinterpret_cast<Vec3*>(iMesh.mVertices + i), 1);
      meshBox = meshBox.Merge(reinterpret_cast<Vec3 const&>(*reinterpret_cast<Vec3*>(oVtxData)));

      oVtxData += 3;
      if (iInfo.hasNormals)
      {
        if (iMesh.HasNormals())
        {
          //memcpy(oVtxData, iMesh.mNormals + i, sizeof(aiVector3D));
          *reinterpret_cast<Vec3*>(oVtxData) = normalize(iTransform * Vec4(*reinterpret_cast<Vec3*>(iMesh.mNormals + i), 0));
        }
        else
        {
          memcpy(oVtxData, &nullVec, sizeof(aiVector3D));
        }
        oVtxData += 3;
      }

      if (iInfo.hasUV)
      {
        if (iMesh.HasTextureCoords(0))
        {
          memcpy(oVtxData, iMesh.mTextureCoords[0] + i, sizeof(aiVector2D));
        }
        else
        {
          memcpy(oVtxData, &nullVec, sizeof(aiVector2D));
        }
        oVtxData += 2;
      }

      if (iInfo.hasTangentSpace)
      {
        if (iMesh.HasTangentsAndBitangents())
        {
          //memcpy(oVtxData, iMesh.mTangents + i, sizeof(aiVector3D));
          //memcpy(oVtxData + 3, iMesh.mBitangents + i, sizeof(aiVector3D));
          *reinterpret_cast<Vec3*>(oVtxData) = normalize(iTransform * Vec4(*reinterpret_cast<Vec3*>(iMesh.mTangents + i), 0));
          *reinterpret_cast<Vec3*>(oVtxData + 3) = normalize(iTransform * Vec4(*reinterpret_cast<Vec3*>(iMesh.mBitangents + i), 0));
        }
        else
        {
          memcpy(oVtxData, &nullVec, sizeof(aiVector3D));
          memcpy(oVtxData, &nullVec, sizeof(aiVector3D));
        }

        oVtxData += 6;
      }
    }

    for (uint32_t i = 0; i < iMesh.mNumFaces; ++i)
    {
      aiFace const& curFace = iMesh.mFaces[i];

      oIdxData[0] = curFace.mIndices[0] + iBaseIdx;
      oIdxData[1] = curFace.mIndices[1] + iBaseIdx;
      oIdxData[2] = curFace.mIndices[2] + iBaseIdx;

      oIdxData += 3;
    }

    return meshBox;
  }

  // Print a material's properties (DEBUG)
  void PrintMatProperties(aiMaterial const& iMat)
  {
    auto printProperties = [](auto const* iDataPtr, aiMaterialProperty const& iProp, const char* iFormat)
    {
      uint32_t const numValues = iProp.mDataLength / sizeof(iDataPtr[0]);
      printf("(");
      for (uint32_t i = 0; i < numValues; ++i, ++iDataPtr)
      {
        printf(iFormat, *iDataPtr);
        if (i != numValues - 1)
        {
          printf(", ");
        }
      }
      printf(")\n");
    };

    for (uint32_t propIdx = 0; propIdx < iMat.mNumProperties; ++propIdx)
    {
      aiMaterialProperty const* prop = iMat.mProperties[propIdx];
      printf("Property %i : %s -> ", propIdx, prop->mKey.C_Str());
      switch (prop->mType)
      {
      case aiPTI_Float:
        printProperties(reinterpret_cast<float const*>(prop->mData), *prop, "%f");
        break;
      case aiPTI_Double:
        printProperties(reinterpret_cast<double const*>(prop->mData), *prop, "%f");
        break;
      case aiPTI_Integer:
        printProperties(reinterpret_cast<float const*>(prop->mData), *prop, "%i");
        break;
      case aiPTI_String:
      {
        aiString string;
        string.length = static_cast<unsigned int>(*reinterpret_cast<uint32_t*>(prop->mData));
        memcpy(string.data, prop->mData + 4, string.length + 1);
        printf("\"%s\"\n", string.C_Str());
      }
      break;
      case aiPTI_Buffer:
        printf("(binary buffer)\n");
        break;
      }
    }
  }
  struct SceneImportData
  {
    Vector<uint32_t> meshesVtxOffsets;
    Vector<uint32_t> meshesIdxOffsets;
    Vector<uint32_t> meshesIdxSize;
    Vector<uint32_t> meshesMaterial;
    Vector<Box3D> meshesBox;

    MeshInfo meshInfo;
  };

  ImporterContext::~ImporterContext() = default;

  void ComputeMeshSizeFromTree(SceneImportData& ioMeshes, aiScene const& iScene, aiNode const& iNode)
  {
    for (uint32_t i = 0; i < iNode.mNumMeshes; ++i)
    {
      aiMesh const* curMesh = iScene.mMeshes[iNode.mMeshes[i]];

      MeshInfo curInfo = GetMeshInfos(*curMesh);

      if (curInfo.numVtx == 0)
      {
        continue;
      }

      ioMeshes.meshesVtxOffsets.push_back(ioMeshes.meshInfo.numVtx);
      ioMeshes.meshesIdxOffsets.push_back(ioMeshes.meshInfo.numIdx);
      ioMeshes.meshesIdxSize.push_back(curInfo.numIdx);
      ioMeshes.meshesMaterial.push_back(curMesh->mMaterialIndex);

      ioMeshes.meshInfo.Combine(curInfo);
    }

    for (uint32_t i = 0; i < iNode.mNumChildren; ++i)
    {
      ComputeMeshSizeFromTree(ioMeshes, iScene, *iNode.mChildren[i]);
    }
  }

  void ComputeGeometryFromTree(SceneImportData& ioMeshes,
    aiScene const& iScene,
    aiNode const& iNode,
    Mat4 const& iParentMat,
    Vector<float>& vertexData,
    Vector<uint32_t>& indices,
    size_t const iVertexSize,
    uint32_t& ioMeshCounter,
    uint32_t& baseIdx,
    Box3D& oSceneBox)
  {
    Mat4 curMat = iParentMat * transpose(*reinterpret_cast<Mat4 const*>(&iNode.mTransformation));

    for (uint32_t i = 0; i < iNode.mNumMeshes; ++i)
    {
      aiMesh const* curMesh = iScene.mMeshes[iNode.mMeshes[i]];

      // Skip empty meshes.
      MeshInfo curInfo = GetMeshInfos(*curMesh);
      if (curInfo.numVtx == 0)
      {
        continue;
      }

      float* outData = vertexData.data() + iVertexSize * ioMeshes.meshesVtxOffsets[ioMeshCounter];
      uint32_t* outIdx = indices.data() + ioMeshes.meshesIdxOffsets[ioMeshCounter];

      Box3D meshBox = ReadMesh(*curMesh, ioMeshes.meshInfo, iVertexSize, baseIdx, outData, outIdx, curMat);
      oSceneBox = oSceneBox.Merge(meshBox);

      baseIdx += curMesh->mNumVertices;
      ioMeshCounter++;
    }

    for (uint32_t i = 0; i < iNode.mNumChildren; ++i)
    {
      ComputeGeometryFromTree(ioMeshes, iScene, *iNode.mChildren[i], curMat, vertexData, indices, iVertexSize, ioMeshCounter, baseIdx, oSceneBox);
    }
  }

  void ComputeSceneFromTree(SceneImportData const& iMeshes,
    aiScene const& iScene,
    aiNode const& iNode,
    Mat4 const& iParentMat,
    IntrusivePtr<Geometry> const& iGeom,
    Vector<IntrusivePtr<Material const>> const& iMaterials,
    Scene& oScene
    )
  {
    Mat4 curMat = iParentMat * transpose(*reinterpret_cast<Mat4 const*>(&iNode.mTransformation));

    if(iNode.mNumMeshes > 0)
    {
      Box3D modelBox;
      for (uint32_t i = 0; i < iNode.mNumMeshes; ++i)
      {
        uint32_t meshIdx = iNode.mMeshes[i];
        if (iMeshes.meshesIdxSize[meshIdx] != 0)
        {
          modelBox = modelBox.Merge(iMeshes.meshesBox[meshIdx]);
          oScene.m_SceneBox = oScene.m_SceneBox.Merge(curMat * iMeshes.meshesBox[meshIdx]);
        }
      }

      auto builder = Model::Create(iGeom, modelBox);
      for (uint32_t i = 0; i < iNode.mNumMeshes; ++i)
      {
        uint32_t meshIdx = iNode.mMeshes[i];
        if (iMeshes.meshesIdxSize[meshIdx] != 0)
        {
          builder.AddPart(iMaterials[iMeshes.meshesMaterial[meshIdx]], iMeshes.meshesIdxSize[meshIdx], iMeshes.meshesIdxOffsets[meshIdx]);
        }
      }

      oScene.m_Models.push_back(builder.End());
      oScene.m_Transforms.push_back(curMat);
    }
    for (uint32_t i = 0; i < iNode.mNumChildren; ++i)
    {
      ComputeSceneFromTree(iMeshes, iScene, *iNode.mChildren[i], curMat, iGeom, iMaterials, oScene);
    }
  }

  Scene ImportScene(ImporterContext const& iCtx, String const& iPath)
  {
    aiScene const* scene = aiImportFile(iPath.c_str(), ppsteps);

    if (scene == nullptr)
    {
      return Scene();
    }

    // Gather informations about all the scene's meshes.
    SceneImportData mergedMeshesInfo;
    if (iCtx.bakeTransforms)
    {
      ComputeMeshSizeFromTree(mergedMeshesInfo, *scene, *scene->mRootNode);
    }
    else
    {
      for (uint32_t meshNum = 0; meshNum < scene->mNumMeshes; ++meshNum)
      {
        aiMesh const* curMesh = scene->mMeshes[meshNum];

        MeshInfo curInfo = GetMeshInfos(*curMesh);

        mergedMeshesInfo.meshesVtxOffsets.push_back(mergedMeshesInfo.meshInfo.numVtx);
        mergedMeshesInfo.meshesIdxOffsets.push_back(mergedMeshesInfo.meshInfo.numIdx);
        mergedMeshesInfo.meshesIdxSize.push_back(curInfo.numIdx);
        mergedMeshesInfo.meshesMaterial.push_back(curMesh->mMaterialIndex);

        mergedMeshesInfo.meshInfo.Combine(curInfo);
      }
    }
    // Allocate buffers to hold all the meshes information.
    uint32_t vertexSize = mergedMeshesInfo.meshInfo.VertexSizeInFloats();
    Vector<float> vertexData(vertexSize * mergedMeshesInfo.meshInfo.numVtx);
    Vector<uint32_t> indices(mergedMeshesInfo.meshInfo.numIdx);

    Box3D modelBox;
    uint32_t baseIdx = 0;

    if (iCtx.bakeTransforms)
    {
      uint32_t meshCounter = 0;
      ComputeGeometryFromTree(mergedMeshesInfo, *scene, *scene->mRootNode, iCtx.importTransform, vertexData, indices, vertexSize, meshCounter, baseIdx, modelBox);
    }
    else
    {
      // Read individual meshes, all merged into the same buffer.
      for (uint32_t meshNum = 0; meshNum < scene->mNumMeshes; ++meshNum)
      {
        aiMesh const* curMesh = scene->mMeshes[meshNum];

        // Skip empty meshes.
        MeshInfo curInfo = GetMeshInfos(*curMesh);
        if (curInfo.numVtx == 0)
        {
          mergedMeshesInfo.meshesBox.push_back(Box3D());
          continue;
        }

        float* outData = vertexData.data() + vertexSize * mergedMeshesInfo.meshesVtxOffsets[meshNum];
        uint32_t* outIdx = indices.data() + mergedMeshesInfo.meshesIdxOffsets[meshNum];

        Box3D meshBox = ReadMesh(*curMesh, mergedMeshesInfo.meshInfo, vertexSize, baseIdx, outData, outIdx, Identity<Mat4>());
        mergedMeshesInfo.meshesBox.push_back(meshBox);

        baseIdx += curMesh->mNumVertices;
      }
    }

    // Create vertex and index buffers.
    IntrusivePtr<OGLBuffer> vtxBuffer(OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, vertexData));
    IntrusivePtr<OGLBuffer> idxBuffer(OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER, indices));

    uint32_t vertexSizeInBytes = vertexSize * sizeof(float);
    uint32_t bufferOffset = 0;

    // Create the model's geometry.
    IntrusivePtr<Geometry> modelGeom = MakeRefCounted<Geometry>();
    modelGeom->m_Assembly.AddAttrib(vtxBuffer, OGLBaseAlgo::GetPosAttrib(), 3, vertexSizeInBytes, bufferOffset);
    bufferOffset += 3 * sizeof(float);
    if (mergedMeshesInfo.meshInfo.hasNormals)
    {
      modelGeom->m_Assembly.AddAttrib(vtxBuffer, OGLMeshAlgo::GetNormalAttrib(), 3, vertexSizeInBytes, bufferOffset);
      bufferOffset += 3 * sizeof(float);
    }
    if (mergedMeshesInfo.meshInfo.hasUV)
    {
      modelGeom->m_Assembly.AddAttrib(vtxBuffer, OGLBaseAlgo::GetTexCoordAttrib(), 2, vertexSizeInBytes, bufferOffset);
      bufferOffset += 2 * sizeof(float);
    }
    if (mergedMeshesInfo.meshInfo.hasTangentSpace && 0)
    {
      modelGeom->m_Assembly.AddAttrib(vtxBuffer, "tangent", 3, vertexSizeInBytes, bufferOffset);
      modelGeom->m_Assembly.AddAttrib(vtxBuffer, "bitangent", 3, vertexSizeInBytes, bufferOffset + 3*sizeof(float));
      bufferOffset += 6 * sizeof(float);
    }

    modelGeom->m_Assembly.m_IBuffer = idxBuffer;

    if (iCtx.keepShadowCopy)
    {
      modelGeom->m_ShadowCopy = std::make_unique<GeometryData>();
      modelGeom->m_ShadowCopy->m_VertexData = std::move(vertexData);
      modelGeom->m_ShadowCopy->m_Indices = std::move(indices);
    }

    // Gather scene's materials.
    Vector<IntrusivePtr<Material const>> materials;
    for (uint32_t matIdx = 0; matIdx < scene->mNumMaterials; ++matIdx)
    {
      aiMaterial const* curMat = scene->mMaterials[matIdx];

      String name(curMat->GetName().C_Str());
      auto iter = iCtx.materialMapping.find(name);
      if (iter != iCtx.materialMapping.end())
      {
        materials.push_back(iter->second);
      }
      else
      {
        IntrusivePtr<Material const> meshMat = iCtx.baseMaterial;
        aiColor4D meshColor;
        if (curMat->Get(AI_MATKEY_COLOR_DIFFUSE, meshColor) == aiReturn_SUCCESS
          && meshColor != aiColor4D(1, 1, 1, 1))
        {
          //auto newMat = iCtx.baseMaterial->Duplicate();
          //meshMat = newMat;
          //newMat->SetShaderData("baseColor", reinterpret_cast<Vec4 const&>(meshColor));
          meshMat = iCtx.baseMaterial;
        }
        
        materials.push_back(meshMat);
      }

#ifdef PRINT_MATERIAL_PROPERTIES
      PrintMatProperties(*curMat);
#endif
    }
    if (iCtx.bakeTransforms)
    {
      // Create the final model, the combination of all the meshes and their material.
      auto builder = Model::Create(std::move(modelGeom), modelBox);

      for (uint32_t i = 0; i < mergedMeshesInfo.meshesIdxOffsets.size(); ++i)
      {
        if (mergedMeshesInfo.meshesIdxSize[i] != 0)
        {
          builder.AddPart(materials[mergedMeshesInfo.meshesMaterial[i]], mergedMeshesInfo.meshesIdxSize[i], mergedMeshesInfo.meshesIdxOffsets[i]);
        }
      }
     
      Scene mergedScene;
      mergedScene.m_Models.push_back(builder.End());
      mergedScene.m_Transforms.push_back(Identity<Mat4>());
      mergedScene.m_SceneBox = modelBox;
        
      return mergedScene;
    }
    else
    {
      Scene splitScene;
      ComputeSceneFromTree(mergedMeshesInfo, *scene, *scene->mRootNode, iCtx.importTransform, modelGeom, materials, splitScene);

      return splitScene;
    }
  }
};