#include "principalaxis.hpp"

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

  template <typename Real>
  bool ComputePrincipalAxis::Compute(Polygon<Real> const& iPoly)
  {
    arma::mat covarianceMatrix(2, 2, arma::fill::zeros);
    Vector2d mean = Vector2d::ZERO;

    Vector<Vector2<Real> > const& border = iPoly.Border();
    if (border.size() > 0)
    {
      unsigned int numPts = border.size();

      Vector2<Real> initValue = border[0];
      mean.X() += initValue.X();
      mean.Y() += initValue.Y();
      for (unsigned int i = 1; i < numPts; ++i)
      {
        Vector2<Real> curValue = border[i];
        if (curValue == initValue)
        {
          --numPts;
          break;
        }
        mean.X() += curValue.X();
        mean.Y() += curValue.Y();
      }

      mean.X() /= numPts;
      mean.Y() /= numPts;

      for (unsigned int i = 0; i < numPts; ++i)
      {
        Vector2<Real> curValue = border[i];
        double centeredX = curValue.X() - mean.X();
        double centeredY = curValue.Y() - mean.Y();
        covarianceMatrix.at(0,0) += centeredX * centeredX;
        covarianceMatrix.at(1,1) += centeredY * centeredY;
        covarianceMatrix.at(0,1) += centeredX * centeredY;
      }

      covarianceMatrix.at(0,0) /= numPts;
      covarianceMatrix.at(1,1) /= numPts;
      covarianceMatrix.at(0,1) /= numPts;
      covarianceMatrix.at(1,0) = covarianceMatrix.at(0,1);

      arma::colvec eigval;
      arma::mat eigvect;
      
      bool res = arma::eig_sym(eigval, eigvect, covarianceMatrix);

      eXl_ASSERT_MSG(res, EXL_TEXT("Could not get eigenvect for PrincipalAxis"));

      if (res)
      {
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
    return false;
  }
}