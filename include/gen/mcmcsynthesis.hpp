/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/gen_exp.hpp>

#include <core/containers.hpp>
#include <math/polygon.hpp>
#include <core/stream/serializer.hpp>

namespace eXl
{
  class Random;

  namespace MCMC2D
  {

    struct EXL_GEN_API PlacedElement
    {
      Vector2i     m_Pos;
      float        m_Angle;
      int          m_Element;
      int          m_ShapeNum;
      SERIALIZE_METHODS;
    };

    enum DirectionMethod
    {
      eCentroidal       = 0,
      eSeparatingPlanes = 1,
      eNearestPlane     = 2,
      eMirrorOther      = 3
    };

    struct EXL_GEN_API Element
    {
      inline Element()
        :m_Turn(0)
        ,m_AbsDensity(1.0)
        ,m_RelDensity(1.0)
        ,m_GridX(0)
        ,m_GridY(0)
        ,m_Layer(0)
        ,m_DirMethod(eCentroidal)
      {}

      Vector<Polygoni> m_Shapes;
      unsigned int m_Turn;
      float    m_AbsDensity;
      //float    m_AbsDensityBorder;
      float    m_RelDensity;
      unsigned int m_GridX;
      unsigned int m_GridY;
      unsigned int m_Layer;
      DirectionMethod m_DirMethod;

      SERIALIZE_METHODS;
    };

    struct ElementInter
    {
      float distance;
      Vector2d dir1;
      Vector2d dir2;
      int elem2;
      float coeff = 1.0;
    };

    struct SamplingCell
    {
      Vector2i position;
      float angle;
      unsigned int elem1;
      float weight;
      Vector<ElementInter> interactions;
      bool isPresence = false;
    };

    class EXL_GEN_API Debug
    {
    public:

      enum Op
      {
        Birth,
        Death
      };

      struct ElemInteraction
      {
        ElementInter m_Inter;
        double m_Result;
      };

      struct OpDesc
      {
        Op m_Op;
        PlacedElement m_Element;
        double m_FinalResult;
        bool m_Passed = false;

        Vector<ElemInteraction> m_Interaction;
      };

      virtual bool ForceOperation(OpDesc& oOp)
      {
        return false;
      }

      Vector<OpDesc> m_Ops;
    };

    struct EXL_GEN_API LearnExample
    {
      Polygoni m_Shape;
      Vector<PlacedElement> m_Elements;

      SERIALIZE_METHODS;
    };

    struct QuantileCullParams;

    struct EXL_GEN_API LearnParams
    {
      LearnParams(Random& iRandGen)
        : m_RandGen(iRandGen)
      {
      }

      Random& m_RandGen;
      bool m_Toroidal = true;
      float m_MaxDist_;
      float m_RegularisationCoeff = 0.01;
      unsigned int m_GridStep = 10;
      unsigned int m_Oversampling = 10;
      float m_Dispersion = 0.1;
      bool m_Resample = false;
      bool m_QuantileCull = true;
      float m_SigmaDist;
      float m_SigmaAngle = Mathf::Sqrt(1.0 - Mathf::Cos(Mathf::PI / 8.0));

      SERIALIZE_METHODS;
    };

    struct InputBuilder
    {
      static unsigned int ComputeNumElems(Vector<MCMC2D::Element> const& iElems, bool iToroidal);

      InputBuilder(Vector<MCMC2D::Element> const& iElems, bool iToroidal);

      unsigned int BuildOneHot(unsigned int i, int j, float otherAngle) const;

      //unsigned int const numInputs = 3;
      unsigned int const oneHotSize;
      Vector<Vector<std::pair<unsigned int, unsigned int>>> interMap;
      bool toroidal;
    };

    struct InteractionGeom
    {
      double dist;
      Vector2<double> dir1;
    };

    struct FullInteraction : InteractionGeom
    {
      static FullInteraction Make(InputBuilder& builder, SamplingCell const& iCell, ElementInter const& iInter);

      void Perturbate(eXl::Random& iRand, float iRadius, float iAngRange);

      unsigned int oneHotIdx;
      Vector2<double> dir2;
    };

    class EXL_GEN_API LearnedModel
    {
    public:

      struct DistParams
      {
        Err Stream(Streamer& streamer) const;
        Err Unstream(Unstreamer& streamer);

        float m_MaxDist;
        float m_EaseOutOffset;
        float m_EaseOutCoeff;
      };

      LearnedModel(Vector<DistParams> const& iDist, Vector<Element> const& iElements, bool iToroidal);

      ~LearnedModel();

      virtual float Sample(FullInteraction const&) = 0;

      virtual Vector<float> Sample(unsigned int oneHotIdx, Vector<InteractionGeom> const& interactions);

      virtual String const& GetModelName() const = 0;

      virtual Err Stream(Streamer& streamer) const = 0;
      virtual Err Unstream(Unstreamer& streamer) = 0;

      virtual bool CanSuggestPosition() const
      { return false; }

      virtual bool SuggestPosition(Random& iRand, InputBuilder& iBuilder, PlacedElement const& iElem, PlacedElement& ioSuggestion) const
      { return false; }

      Vector<Element> const& GetElements() { return m_Elements; }
      Vector<DistParams>& GetDist() { return m_DistParams; }

      float GetMaxDist(unsigned int iOneHotIdx) const;

      bool IsToroidal() const { return m_Toroidal; }

    protected:

      void WriteCommon(Streamer& streamer) const;
      void ReadCommon(Unstreamer& streamer);

      Vector<DistParams> m_DistParams;
      Vector<Element> m_Elements;
      std::unique_ptr<QuantileCullParams> m_Mask;
      bool m_Toroidal;
    };

    struct QuantileCullParams : public LearnedModel
    {
      QuantileCullParams(Vector<Element> const& iElements, bool iToroidal, float iMaxDist, double iSigmaDist, double iSigmaAngle);

      float Sample(MCMC2D::FullInteraction const& iInter) override;

      void ComputeDistribution(unsigned int iIdx, double iDist, Vector2d const& iDir, double& oConstant, double& oAlpha);

      QuantileCullParams* Clone() const;

      String const& GetModelName() const override;

      Err Stream(Streamer& streamer) const override;

      Err Unstream(Unstreamer& streamer) override;

      double m_SigmaDist;
      double m_SigmaAngle;

      bool m_DebugOutput = false;
      bool m_DebugDisplayPos = false;

      Vector<Vector<Vector3d>> m_Vectors;
    };

    float GetMaxDist_Common(unsigned int iOneHotIdx, Vector<LearnedModel::DistParams> const& iDists);

    class Learner
    {
    public:

      Learner(Vector<Element> const& iElements, LearnParams& iParams)
        : m_Elements(iElements)
        , m_Params(iParams)
        , m_Builder(iElements, iParams.m_Toroidal)
      {}

      ~Learner()
      {}

      virtual String const& GetModelName() const = 0;

      virtual LearnedModel* UnstreamModel(Unstreamer& streamer) const = 0;

      virtual void Learn(LearnedModel*& ioModel, LearnExample const& iExamples, unsigned int const numIter, Debug* iDebug = nullptr) = 0;

      virtual void Refine(LearnedModel& ioModel, unsigned int const numIter) = 0;

      virtual LearnParams const& GetParams() const { return m_Params; }

    protected:

      float GetMaxDist(unsigned int iOneHotIdx) const;

      Vector<Element> const& GetElements() { return m_Elements; }

      Vector<SamplingCell>& GetCells() { return m_Cells; }

      InputBuilder& GetBuilder() { return m_Builder; }

      void BuildCells(LearnExample const& iExamples);

      Vector<SamplingCell> m_Cells;
      InputBuilder m_Builder;
      Vector<LearnedModel::DistParams> m_DistParams;

      LearnParams& m_Params;
      Vector<Element> const& m_Elements;
      std::unique_ptr<QuantileCullParams> m_Mask;
    };

    struct EXL_GEN_API RunParams
    {
      Polygoni m_Shape;
      unsigned int m_NumIter;
      Vector<PlacedElement> m_Placed;
      Vector<PlacedElement> m_Static;

      float m_Temperature = 1.0;

      SERIALIZE_METHODS;
    };

    EXL_GEN_API void Run(Random& iRand, RunParams& iParams, LearnedModel* model, Debug* iDebug = nullptr);

#ifdef EXL_IMAGESTREAMER_ENABLED
    EXL_GEN_API void DrawDbgImg(String const& iPath, LearnedModel* model);
#endif

    template<typename Index>
    struct QueryCache
    {
      Polygoni            m_Poly1;
      Polygoni            m_Poly2;
      Vector<Polygoni>    m_Poly3;
      Vector<Polygoni>    m_Walls;
      Index               m_Index;
      Vector<typename Index::value_type> m_RetVal;
    };

    template <typename Handler, typename Index, typename ObjIdentity>
    bool Query(Vector<Element> const& iElements, QueryCache<Index>& iCache, Handler& iHandler, ObjIdentity const& iIdent, AABB2Di const& iQueryBox, PlacedElement const& iCurElem);

    template <typename Handler, typename Index, typename ObjIdentity>
    bool Query(Vector<Element> const& iElements, QueryCache<Index>& iCache, Handler& iHandler, ObjIdentity const& iIdent, AABB2Di const& iQueryBox, PlacedElement const& iCurElem, bool iToroidal, AABB2Di const& iSceneBox);

  };
}
