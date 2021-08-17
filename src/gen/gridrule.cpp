#include <gen/gridrule.hpp>

namespace eXl
{

  MultiGrid::Iterator GridRule::MoveIter(MultiGrid::Iterator const& iIter, Vector2i const& iOffset, MultiGrid::Direction iBaseDir)
  {
    MultiGrid::Iterator curBox = iIter;
    for(unsigned int dim = 0; dim<2; ++dim)
    {
      if(iOffset.m_Data[dim] != 0)
      {
        MultiGrid::Direction dir = (MultiGrid::Direction)(2 * dim + (iOffset.m_Data[dim] > 0 ? 1 : 0));
        dir = LocalToWorld(dir, iBaseDir);
        unsigned int numIter = Mathi::Abs(iOffset.m_Data[dim]);
        for(unsigned int k = 0; k<numIter; ++k)
          curBox.Move(dir);
      }
    }
    return curBox;
  }

}