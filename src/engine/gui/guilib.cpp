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

    Vec2i Position::Get(Vec2i iSize) const
    {
      float frame[2] = {1, 1};

      Vec2i pos = Zero<Vec2i>();
      for (uint32_t i = 0; i < 2; ++i)
      {
        if (m_Anchors[i] == Anchor::High)
        {
          frame[i] *= -1;
          pos[i] = iSize[i];
        }
        else if (m_Anchors[i] == Anchor::Middle)
        {
          pos[i] = iSize[i] * 0.5;
        }
      }

      pos += Vec2((m_X.m_Scale * iSize.x + m_X.m_Offset) * frame[0], (m_Y.m_Scale * iSize.y + m_Y.m_Offset) * frame[1]);

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

    DlgDim DlgDim::ChildDlg(Size iChildSize) const
    {
      DlgDim childDim = *this;
      childDim.m_Size = iChildSize.Get(m_Size);

      return childDim;
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

      DlgDim dlgDim = iDlgDim.ChildDlg(m_Size);

      if (m_Children.empty())
      {
        return std::make_tuple(dlgDim, ObjectHandle());
      }

      ObjectHandle parentToUse = iCtx.parent;
      Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_AnchorPoint, 0.0));
      Transforms& trans = *iCtx.world.GetSystem<Transforms>();
      if (!m_IgnoreNode || !parentToUse.IsAssigned())
      {
        if (m_NeedScissor)
        {
          Vec2 scissor(Vec2(dlgDim.m_Size) * iCtx.win.m_WinScale);
        }

        m_Obj = iCtx.world.CreateObject();

        Transforms& trans = *iCtx.world.GetSystem<Transforms>();
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
          dimWithOrig.m_AnchorPoint += iDlgDim.m_AnchorPoint;
        }

        LayoutCtx childCtx = iCtx;
        childCtx.parent = parentToUse;

        DlgDim childDlgDim = child->Layout(dimWithOrig, childCtx);

        if (childDlgDim.m_Layer > maxLayer)
        {
          maxLayer = childDlgDim.m_Layer;
        }

        AABB2Di childBox = AABB2Di::FromMinAndSize(childDlgDim.m_AnchorPoint, childDlgDim.m_Size);
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
      dlgDim.m_AnchorPoint = box.m_Min;
      dlgDim.m_Size = box.GetSize();
      dlgDim.m_Anchors[0] = dlgDim.m_Anchors[1] = Anchor::Low;
      m_LayoutBox = box;

      if (m_Obj.IsAssigned())
      {
        Position offsetPos(Dim(0, 0), Dim(0, 0), iDlgDim.m_Anchors[0], iDlgDim.m_Anchors[1]);
        Vec2i dialogOffset = offsetPos.Get(dlgDim.m_Size);
        dlgDim.m_AnchorPoint -= dialogOffset;
        trans.UpdateTransform(m_Obj, translate(Identity<Mat4>(), position - Vec3(dialogOffset, 0)));
      }

      return std::make_tuple(dlgDim, m_Obj);
    }

    Dialog::LayoutCallback MakeStackLayout(Size iHalfOffset, bool iVertical)
    {
      return [&](Dialog& iDialog, DlgDim const& iDlgDim, LayoutCtx const& iCtx)
      {
        uint32_t maxLayer = iDlgDim.m_Layer;
        Vec2 origSize = iDialog.GetSize().Get(iDlgDim.m_Size);

        DlgDim dlgDim = iDlgDim.ChildDlg(iDialog.m_Size);
        dlgDim.m_Anchors[0] = dlgDim.m_Anchors[1] = Anchor::Low;

        Vec2i halfOffset = iHalfOffset.Get(iDlgDim.m_Size);

        Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_AnchorPoint, 0.0));
        ObjectHandle stackObj = iCtx.world.CreateObject();

        Transforms& trans = *iCtx.world.GetSystem<Transforms>();
        trans.AddTransform(stackObj, translate(Identity<Mat4>(), position));
        trans.Attach(stackObj, iCtx.parent);

        int minPosY;
        int minPosX;

        if (iVertical)
        {
          minPosY = iDlgDim.m_AnchorPoint.y;
        }
        else
        {
          minPosX = iDlgDim.m_AnchorPoint.x;
        }

        Vec3 curPos = iCtx.win.DimScale(Vec3(iDlgDim.m_AnchorPoint, 0.0));
        dlgDim.m_AnchorPoint += halfOffset;
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
            dlgDim.m_AnchorPoint += Vec2i(0.0, childDlgDim.m_Size.y + 2.0 * halfOffset.y);
          }
          else
          {
            dlgDim.m_AnchorPoint += Vec2i(childDlgDim.m_Size.x + 2.0 * halfOffset.x, 0.0);
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
        Position offsetPos(Dim(0, 0), Dim(0, 0), iDlgDim.m_Anchors[0], iDlgDim.m_Anchors[1]);
        Vec2i dialogOffset = offsetPos.Get(dlgDim.m_Size);
        dlgDim.m_AnchorPoint -= dialogOffset;

        trans.UpdateTransform(stackObj, translate(Identity<Mat4>(), position - Vec3(dialogOffset, 0)));
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
          return std::make_tuple(iDlgDim, ObjectHandle());
        }

        Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_AnchorPoint, 0.0));
        DlgDim dim = iDlgDim.ChildDlg(m_Size);

        Mat4 transform = translate(Identity<Mat4>(), position);

        if (m_StretchToSize)
        {
          Vec2 imgScale = Vec2(dim.m_Size) / (Vec2(tile->m_Size) * m_ImgDesc.m_Size);
          transform = scale(transform, Vec3(imgScale, 1));
        }
        else
        {
          dim.m_Size = tile->m_Size;
        }

        Position offsetPos(Dim(0, 0), Dim(0, 0), iDlgDim.m_Anchors[0], iDlgDim.m_Anchors[1]);
        Vec2i dialogOffset = offsetPos.Get(dim.m_Size);
        dim.m_AnchorPoint -= dialogOffset;
        transform[3] += Vec4(dim.m_Size / 2 - dialogOffset, 0, 0);

        m_LayoutBox = AABB2Di::FromCenterAndSize(Zero<Vec2i>(), dim.m_Size);

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
          return std::make_tuple(iDlgDim, ObjectHandle());
        }

        Vec3 position = iCtx.win.DimScale(Vec3(iDlgDim.m_AnchorPoint, 0.0));
        DlgDim dim = iDlgDim.ChildDlg(iDlg.m_Size);

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