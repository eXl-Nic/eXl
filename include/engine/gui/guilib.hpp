#pragma once

#include <math/math.hpp>

#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <engine/gfx/gfxcomponent.hpp>

namespace eXl
{
  class GfxGUIRenderNode;
  class FontResource;

  namespace GUI
  {
    struct WindowSetup
    {
      Vec2i m_DesignDimension = Vec2i(0, 0);
      bool m_MajorDimX = true;

      Vec2i m_WinDimension = Vec2(0, 0);
      float m_WinScale = 0.0;
      Vec2i m_WinOffset = Vec2(0, 0);

      void SetWindowSize(Vec2i iDim);

      Vec3 DimScale(Vec3 iSize) const
      {
        return Vec3(iSize.x * m_WinScale, iSize.y * m_WinScale, iSize.z);
      }

      Vec3 DimPosition(Vec3 iPos) const
      {
        return Vec3(iPos.x * m_WinScale + m_WinOffset.x * m_WinScale, iPos.y * m_WinScale + m_WinOffset.y * m_WinScale, iPos.z);
      }
    };

    struct Dim
    {
      // Value relative to the parent size
      float m_Scale;
      // Absolute value. 
      float m_Offset;
      Dim(float iScale, float iOffset)
      {
        m_Offset = iOffset;
        m_Scale = Mathf::Clamp(iScale, -1.f, 1.f);
      }
    };

    struct Position
    {
      enum Anchor
      {
        None = 0,
        UpperLeft = 1,
        UpperRight = 2,
        LowerLeft = 3,
        LowerRight = 4,
        Center = 5,
      };

      Dim m_X;
      Dim m_Y;
      Anchor m_Anchor;

      Position(Dim iX, Dim iY, Anchor iAnchor = None)
        : m_X(iX), m_Y(iY), m_Anchor(iAnchor)
      {
      }

      Vec2i Get(Vec2i iSize, Vec2i iElementSize) const;
    };

    struct Size
    {
      enum AspectConstraint
      {
        FreeDim = 1,
        SquareXDim = 2,
        SquareYDim = 3,
      };

      Dim m_X;
      Dim m_Y;
      AspectConstraint m_Constraint;

      Size(Dim iX, Dim iY, AspectConstraint iConst = FreeDim)
        : m_X(iX), m_Y(iY), m_Constraint(iConst)
      {
      }

      static Size Full()
      {
        return Size(Dim(1.0, 0.0), Dim(1.0, 0.0), FreeDim);
      }

      static Size SquareX(Dim iDim)
      {
        return Size(iDim, Dim(0.0, 0.0), SquareXDim);
      }

      static Size SquareY(Dim iDim)
      {
        return Size(iDim, Dim(0.0, 0.0), SquareXDim);
      }

      Vec2i Get(Vec2i iSize) const;
    };

    // A laid out dialog size.
    struct DlgDim
    {
      AABB2Di m_Box;
      uint32_t m_Layer;
      bool m_HardX = true;
      bool m_HardY = true;
    };

    struct LayoutCtx
    {
      World& world;
      GfxGUIRenderNode& render;
      WindowSetup const& win;
      ObjectHandle parent;
      Optional<ObjectHandle> worldParent;
    };

    class Dialog : public HeapObject
    {
      DECLARE_RefC;

    protected:

      ObjectHandle m_Obj;
      World* m_World = nullptr;
      Dialog* m_Parent = nullptr;
      AABB2Di m_LayoutBox;
      bool m_Pickable = false;

      void SetParent(Dialog& iChild)
      {
        iChild.m_Parent = this;
      }

    public:

      Size m_Size;
      Vector<IntrusivePtr<Dialog>> m_Children;
      bool m_IgnoreNode = false;
      bool m_NeedScissor = false;
      bool m_WorldSpace = false;

      std::function<Vec2i(uint32_t iChildIdx, DlgDim iDlgDim)> m_GetChildOrigin;
      std::function<uint32_t(uint32_t iOrigLayer, uint32_t iChildLayer)> m_NextLayer;
      std::function<void()> m_Pick;
      using LayoutCallback = std::function<std::tuple<DlgDim, ObjectHandle>(Dialog&, DlgDim const&, LayoutCtx const&)>;
      LayoutCallback m_Layout;

      Dialog(Size const& iSize)
        : m_Size(iSize)
      {
      }

      ~Dialog()
      {
        if (m_World)
        {
          m_World->DeleteObject(m_Obj);
        }
      }

      Size const& GetSize() const
      {
        return m_Size;
      }

      Vector<IntrusivePtr<Dialog>> const& GetChildren() const
      {
        return m_Children;
      }

      ObjectHandle GetObject() const { return m_Obj; }
      AABB2Di const& GetLayoutBox() const { return m_LayoutBox; }
      bool IsPickable() const { return m_Pickable || m_Pick; }

      DlgDim GetChildOrigin(uint32_t iChildIdx, DlgDim iDlgDim) const
      {
        DlgDim childDim = iDlgDim;
        childDim.m_Box = AABB2Di::FromCenterAndSize(m_GetChildOrigin ? m_GetChildOrigin(iChildIdx, iDlgDim) : Zero<Vec2i>(), iDlgDim.m_Box.GetSize());
        return childDim;
      }

      //void OnChildMove(iItem, iPos, iDeltaPos)
      //{
      //  if (m_Parent)
      //  {
      //    m_Parent->OnChildMove(iItem, iPos, iDeltaPos);
      //  }
      //}

      uint32_t NextLayer(uint32_t iOrigLayer, uint32_t iChildLayer)
      {
        return m_NextLayer ? m_NextLayer(iOrigLayer, iChildLayer) : NextLayer_Default(iOrigLayer, iChildLayer);
      }

      DlgDim Layout(DlgDim const& iDlgDim, LayoutCtx const& iCtx)
      {
        m_World = &iCtx.world;
        if (m_Obj.IsAssigned())
        {
          iCtx.world.DeleteObject(m_Obj);
          m_Obj = ObjectHandle();
        }

        auto [actualDim, object] = m_Layout ? m_Layout(*this, iDlgDim, iCtx)
          : Layout_Default(iDlgDim, iCtx);

        m_LayoutBox = actualDim.m_Box;
        m_Obj = object;

        m_Pickable = !(!m_Pick);

        for (auto child : m_Children)
        {
          m_Pickable |= child->m_Pickable;
        }

        return actualDim;
      }

      void StackLayout(LayoutCallback iLayout)
      {
        if (!m_Layout)
        {
          m_Layout = std::move(iLayout);
          return ;
        }

        LayoutCallback saveLayout = std::move(m_Layout);
        m_Layout = [prevLayout = std::move(saveLayout), curLayout = std::move(iLayout)]
        (Dialog& iDlg, DlgDim const& iDlgDim, LayoutCtx const& iCtx)
        {
          auto res = prevLayout(iDlg, iDlgDim, iCtx);
          return curLayout(iDlg, iDlgDim, iCtx);
        };
      }

    protected:

      uint32_t NextLayer_Default(uint32_t iOrigLayer, uint32_t iChildLayer);
      std::tuple<DlgDim, ObjectHandle> Layout_Default(DlgDim const& iDlgDim, LayoutCtx const& iCtx);
    };

    class Container : public Dialog
    {
    public:

      Container(Size const& iSize)
        : Dialog(iSize)
      {
        m_NextLayer = [](uint32_t iOrigLayer, uint32_t)
        {
          return iOrigLayer;
        };
        m_GetChildOrigin = [this](uint32_t iChildIdx, DlgDim iDlgDim)
        {
          return m_Positions[iChildIdx].Get(iDlgDim.m_Box.GetSize(), m_Children[iChildIdx]->GetSize().Get(iDlgDim.m_Box.GetSize()));
        };
      }

      void AddDialog(IntrusivePtr<Dialog> iDialog, Position iPos)
      {
        if (iDialog)
        {
          SetParent(*iDialog);
          m_Children.push_back(std::move(iDialog));
          m_Positions.push_back(iPos);
        }
      }

    protected:
      Vector<Position> m_Positions;
    };

    Dialog::LayoutCallback MakeStackLayout(Size iHalfOffset, bool iVertical);

    class Image : public Dialog
    {
    public:

      Image(Size const& iSize);

      GfxSpriteComponent::Desc m_ImgDesc;
      bool m_StretchToSize = false;
    };

    class Text : public Dialog
    {
    public:

      Text(Size const& iSize);
      ~Text();

      ResourceHandle<FontResource> m_Font;
      String m_Text;
      uint32_t m_Size = 16;
    };
  }
}