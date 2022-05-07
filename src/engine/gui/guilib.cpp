#include <engine/gui/guilib.hpp>
#include <engine/gfx/gfxguirendernode.hpp>
#include <engine/gui/fontresource.hpp>

namespace eXl
{
  namespace GUI
  {
    IMPLEMENT_RefC(Dialog);

    void WindowSetup::SetWindowSize(Vec2i iDim)
    {
      m_WinDimension = iDim;

      m_WinScale = float(iDim.x) / m_DesignDimension.x;
      float otherDim = float(m_DesignDimension.y) * m_WinScale;
      if (otherDim < iDim.y)
      {
        m_WinOffset = Vec2i(0.0, (iDim.y - otherDim) / (2 * m_WinScale));
      }
      else
      {
        m_WinScale = iDim.y / m_DesignDimension.y;
        otherDim = m_DesignDimension.x * m_WinScale;
        m_WinOffset = Vec2i((iDim.x - otherDim) / (2 * m_WinScale), 0.0);
      }
    }

    Vec2i Position::Get(Vec2i iSize, Vec2i iElementSize) const
    {
      Vec2 frames[] =
      { Vec2( 1, 0), Vec2(0,  1),
        Vec2( 1, 0), Vec2(0, -1),
        Vec2(-1, 0), Vec2(0, -1),
        Vec2( 1, 0), Vec2(0,  1),
        Vec2(-1, 0), Vec2(0,  1),
        Vec2( 1, 0), Vec2(0,  1),
      };

      Vec2 orig[] =
      { 
        iSize / 2,
        Vec2i(0, iSize.y),
        iSize,
        Vec2i(0, 0),
        Vec2i(iSize.x, 0),
        iSize / 2
      };

      Vec2 frameX = frames[m_Anchor * 2];
      Vec2 frameY = frames[m_Anchor * 2 + 1];

      Vec2i pos = orig[m_Anchor] + frameX * (m_X.m_Scale * iSize.x + m_X.m_Offset + iElementSize.x / 2) 
        + frameY * (m_Y.m_Scale * iSize.y + m_Y.m_Offset + iElementSize.y / 2);

      return pos;
    }

    Vec2i Size::Get(Vec2i iSize) const
    {
      Vec2i val = Vec2i(0, 0);

      if (m_Constraint == FreeDim || m_Constraint == SquareXDim)
      {
        val.x = Mathf::Abs(m_X.m_Scale) * iSize.x + m_X.m_Offset;
      }
      if (m_Constraint == FreeDim || m_Constraint == SquareYDim)
      {
        val.y = Mathf::Abs(m_Y.m_Scale) * iSize.y + m_Y.m_Offset;
      }
      if (m_Constraint == SquareXDim)
      {
        val.y = val.x;
      }
      if (m_Constraint == SquareYDim)
      {
        val.x = val.y;
      }
      
      return val;
    }

    uint32_t Dialog::NextLayer_Default(uint32_t iOrigLayer, uint32_t iChildLayer)
    {
      uint32_t nextLayer = iOrigLayer;
      if (iChildLayer > iOrigLayer)
      {
        nextLayer = iChildLayer + 1;
      }
      else
      {
        nextLayer = iOrigLayer + 1;
      }
      return nextLayer;
    }

    std::tuple<DlgDim, ObjectHandle> Dialog::Layout_Default(DlgDim const& iDlgDim, LayoutCtx const& iCtx)
    {
      uint32_t maxLayer = iDlgDim.m_Layer;

      DlgDim dlgDim = { AABB2Di::FromCenterAndSize(iDlgDim.m_Box.GetCenter(), m_Size.Get(iDlgDim.m_Box.GetSize())), iDlgDim.m_Layer };

      if (m_Children.empty())
      {
        return std::make_tuple(dlgDim, ObjectHandle());
      }

      ObjectHandle parentToUse = iCtx.parent;
      if (!m_IgnoreNode || !parentToUse.IsAssigned())
      {
        if (m_NeedScissor)
        {
          Vec2 scissor(Vec2(dlgDim.m_Box.GetSize()) * iCtx.win.m_WinScale);
        }

        Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_Box.GetCenter(), 0.0));
        m_Obj = iCtx.world.CreateObject();
        Transforms& trans = *iCtx.world.GetSystem<Transforms>();
        if (!parentToUse.IsAssigned() && iCtx.worldParent)
        {
          position -= Vec3(Vec2(dlgDim.m_Box.GetSize() /2), 0);
        }
        trans.AddTransform(m_Obj, translate(Identity<Mat4>(), position));
        trans.Attach(m_Obj, iCtx.parent);

        parentToUse = m_Obj;
      }
      AABB2Di box;
      for (uint32_t i = 0; i < m_Children.size(); ++i)
      {
        Dialog* child = m_Children[i].get();
        if (child == nullptr)
        {
          continue;
        }
        DlgDim dimWithOrig = GetChildOrigin(i, dlgDim);
        if (m_IgnoreNode)
        {
          dimWithOrig.m_Box.Translate(iDlgDim.m_Box.GetCenter());
        }

        LayoutCtx childCtx = iCtx;
        childCtx.parent = parentToUse;

        DlgDim childDlgDim = child->Layout(dimWithOrig, childCtx);

        if (childDlgDim.m_Layer > maxLayer)
        {
          maxLayer = childDlgDim.m_Layer;
        }

        AABB2Di childBox(childDlgDim.m_Box);
        childBox.Translate(dimWithOrig.m_Box.GetCenter());
        if (i == 0)
        {
          box = childBox;
        }
        else
        {
          box.Absorb(childBox);
        }

        dlgDim.m_Layer = NextLayer(dlgDim.m_Layer, childDlgDim.m_Layer);
      }
      dlgDim.m_Box = box;
      return std::make_tuple(dlgDim, m_Obj);
    }

    Dialog::LayoutCallback MakeStackLayout(Size iHalfOffset, bool iVertical)
    {
      return [&](Dialog& iDialog, DlgDim const& iDlgDim, LayoutCtx const& iCtx)
      {
        uint32_t maxLayer = iDlgDim.m_Layer;
        Vec2 origSize = iDialog.GetSize().Get(iDlgDim.m_Box.GetSize());

        DlgDim dlgDim{ AABB2Di::FromCenterAndSize(iDlgDim.m_Box.GetCenter(), origSize), iDlgDim.m_Layer };

        Vec2i halfOffset = iHalfOffset.Get(iDlgDim.m_Box.GetSize());

        Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_Box.GetCenter(), 0.0));
        ObjectHandle stackObj = iCtx.world.CreateObject();
        Transforms& trans = *iCtx.world.GetSystem<Transforms>();
        trans.AddTransform(stackObj, translate(Identity<Mat4>(), position));
        trans.Attach(stackObj, iCtx.parent);

        int minPosY;
        int minPosX;

        if (iVertical)
        {
          minPosY = iDlgDim.m_Box.m_Min.y;
        }
        else
        {
          minPosX = iDlgDim.m_Box.m_Min.x;
        }

        Vec3 curPos = iCtx.win.DimScale(Vec3(iDlgDim.m_Box.GetCenter(), 0.0));
        dlgDim.m_Box.Translate(halfOffset);
        if (iVertical)
        {
          dlgDim.m_HardY = false;
        }
        else
        {
          dlgDim.m_HardX = false;
        }
        for (uint32_t i = 0; i < iDialog.GetChildren().size(); ++i)
        {
          Dialog* child = iDialog.GetChildren()[i].get();
          if (child == nullptr)
          {
            continue;
          }

          LayoutCtx childCtx = iCtx;
          childCtx.parent = stackObj;

          DlgDim dimWithOrig = iDialog.GetChildOrigin(i, dlgDim);
          DlgDim childDlgDim = child->Layout(dimWithOrig, childCtx);

          if (childDlgDim.m_Layer > maxLayer)
          {
            maxLayer = childDlgDim.m_Layer;
          }
          if (iVertical)
          {
            dlgDim.m_Box.Translate(Vec2i(0.0, childDlgDim.m_Box.GetSize().y + 2.0 * halfOffset.y));
          }
          else
          {
            dlgDim.m_Box.Translate(Vec2i(childDlgDim.m_Box.GetSize().x + 2.0 * halfOffset.x, 0.0));
          }
        }

        //if (iVertical)
        //{
        //  int32_t maxPosY = dlgDim.m_Box.GetCenter().y + halfOffset.y;
        //  if (maxPosY > origSize.y)
        //  {
        //    maxPosY = minPosY;
        //    minPosY = minPosY + origSize.y - maxPosY;
        //  }
        //  else
        //  {
        //    maxPosY = minPosY;
        //  }
        //
        //  dlgDim.m_Size = Vec2i(origSize.y, maxPosY);
        //}
        //else
        //{
        //  int32_t maxPosX = dlgDim.m_Orig.x + halfOffset.x;
        //  if (maxPosX > origSize.x)
        //  {
        //    maxPosX = minPosX;
        //    minPosX = minPosX + origSize.x - maxPosX;
        //  }
        //  else
        //  {
        //    maxPosX = minPosX;
        //  }
        //
        //  dlgDim.m_Size = Vec2i(origSize.x, maxPosX);
        //}
        return std::make_tuple(dlgDim, stackObj);
      };
    }

    Image::Image(Size const& iSize)
      : Dialog(iSize)
    {
      m_Layout = [this](Dialog& iDlg, DlgDim const& iDlgDim, LayoutCtx const& iCtx)
      {
        Tileset const* tileset = m_ImgDesc.m_Tileset.GetOrLoad();
        Tile const* tile = tileset ? tileset->Find(m_ImgDesc.m_TileName) : nullptr;
        if (tile == nullptr)
        {
          return std::make_tuple(DlgDim{ AABB2Di::FromCenterAndSize(iDlgDim.m_Box.GetCenter(), Zero<Vec2i>()) , iDlgDim.m_Layer }, ObjectHandle());
        }

        Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_Box.GetCenter(), 0.0));
        DlgDim dim{ AABB2Di::FromCenterAndSize(iDlgDim.m_Box.GetCenter(), tile->m_Size), iDlgDim.m_Layer };

        Mat4 transform = translate(Identity<Mat4>(), position);

        if (m_StretchToSize)
        {
          dim.m_Box = AABB2Di::FromCenterAndSize(iDlgDim.m_Box.GetCenter(), m_Size.Get(iDlgDim.m_Box.GetSize()));
          Vec2 imgScale = Vec2(dim.m_Box.GetSize()) / (Vec2(tile->m_Size) * m_ImgDesc.m_Size);
          transform = scale(transform, Vec3(imgScale, 1));
        }

        m_Obj = iCtx.world.CreateObject();
        Transforms& trans = *iCtx.world.GetSystem<Transforms>();

        trans.AddTransform(m_Obj, transform);
        trans.Attach(m_Obj, iCtx.parent);

        GfxSpriteComponent::Desc desc = m_ImgDesc;
        desc.m_Layer = iDlgDim.m_Layer;
        
        iCtx.render.AddSprite(m_Obj, desc, iCtx.worldParent);

        return std::make_tuple(dim, m_Obj);
      };
    }

    Text::~Text() = default;

    Text::Text(Size const& iSize)
      : Dialog(iSize)
    {
      m_Layout = [this](Dialog& iDlg, DlgDim const& iDlgDim, LayoutCtx const& iCtx)
      {
        FontResource const* font = m_Font.GetOrLoad();
        if (font == nullptr)
        {
          return std::make_tuple(DlgDim{ AABB2Di::FromCenterAndSize(iDlgDim.m_Box.GetCenter(), Zero<Vec2i>()), iDlgDim.m_Layer }, ObjectHandle());
        }

        Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_Box.GetCenter(), 0.0));
        DlgDim dim{ iDlgDim.m_Box, iDlgDim.m_Layer };

        Mat4 transform = translate(Identity<Mat4>(), position);

        m_Obj = iCtx.world.CreateObject();
        Transforms& trans = *iCtx.world.GetSystem<Transforms>();
        trans.AddTransform(m_Obj, transform);
        trans.Attach(m_Obj, iCtx.parent);

        iCtx.render.AddText(m_Obj, m_Text, m_Size, font, iDlgDim.m_Layer, iCtx.worldParent);

        return std::make_tuple(dim, m_Obj);
      };
    }
  }
}