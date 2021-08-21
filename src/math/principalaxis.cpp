/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/principalaxis.hpp>

#include <armadillo>

namespace eXl
{
  
  bool ComputePrincipalAxis::operator()(Polygoni const& iPoly)
  {
    return Compute(iPoly);
  }

  bool ComputePrincipalAxis::operator()(Polygonf const& iPoly)
  {
    return Compute(iPoly);
  }

  bool ComputePrincipalAxis::operator()(Polygond const& iPoly)
  {
    return Compute(iPoly);
  }

  bool ComputePrincipalAxis::operator()(Vector<Vector2f> const& iPoints)
  {
    return Compute(iPoints.data(), iPoints.size());
  }

  bool ComputePrincipalAxis::operator()(Vector<Vector2d> const& iPoints)
  {
    return Compute(iPoints.data(), iPoints.size());
  }

  template <typename Real>
  bool ComputePrincipalAxis::Compute(Polygon<Real> const& iPoly)
  {
    if (iPoly.Border().size() == 0)
    {
      return false;
    }
    uint32_t numPt = iPoly.Border().size();
    Vector2<Real> const* points = iPoly.Border().data();
    if (numPt > 1 && points[0] == points[numPt - 1])
    {
      --numPt;
    }
    
    return Compute(points, numPt);
  }

  template <typename Real>
  bool ComputePrincipalAxis::Compute(Vector2<Real> const* iPoints, uint32_t iNumPt)
  {
    if (iNumPt == 0)
    {
      return false;
    }
    arma::mat covarianceMatrix(2, 2, arma::fill::zeros);
    m_Center = Vector2d::ZERO;

    for (unsigned int i = 0; i < iNumPt; ++i)
    {
      Vector2<Real> const& curValue = iPoints[i];
      m_Center.X() += curValue.X();
      m_Center.Y() += curValue.Y();
    }

    m_Center.X() /= iNumPt;
    m_Center.Y() /= iNumPt;

    for (unsigned int i = 0; i < iNumPt; ++i)
    {
      Vector2<Real> const& curValue = iPoints[i];
      double centeredX = curValue.X() - m_Center.X();
      double centeredY = curValue.Y() - m_Center.Y();
      covarianceMatrix.at(0,0) += centeredX * centeredX;
      covarianceMatrix.at(1,1) += centeredY * centeredY;
      covarianceMatrix.at(0,1) += centeredX * centeredY;
    }

    covarianceMatrix.at(0,0) /= iNumPt;
    covarianceMatrix.at(1,1) /= iNumPt;
    covarianceMatrix.at(0,1) /= iNumPt;
    covarianceMatrix.at(1,0) = covarianceMatrix.at(0,1);

    arma::colvec eigval;
    arma::mat eigvect;
      
    bool res = arma::eig_sym(eigval, eigvect, covarianceMatrix);

    eXl_ASSERT_REPAIR_RET(res, false);

    unsigned int maxIdx = 0;
    double maxVal = eigval.at(0);
    if (eigval.at(1) > maxVal)
    {
      maxIdx = 1;
      maxVal = eigval.at(1);
    }

    m_PrimaryAxis.X() = eigvect.at(maxIdx, 0);
    m_PrimaryAxis.Y() = eigvect.at(maxIdx, 1);

    return true;
  }
}