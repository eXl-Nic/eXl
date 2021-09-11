#include "editoricons.hpp"
#include <core/corelib.hpp>
#include <core/path.hpp>

namespace eXl
{
  namespace EditorIcons
  {
    namespace
    {
      struct IconsStore
      {
        IconsStore()
        {
          Path appPath(GetAppPath().begin(), GetAppPath().end());
          appPath = appPath.parent_path();
          appPath /= "editor_rsc";
          appPath /= "editor_icons.png";
          
          if (m_IconAtlas.load(appPath.string().c_str()))
          {
            const int32_t standardSize = 32;

            QRect rect(0, 0, standardSize, standardSize);
            QPoint selectionPos(0, 2);
            QPoint drawToolPos(0, 1);
            QPoint fillToolPos(1, 1);
            QPoint moveToolPos(1, 3);
            QPoint eraseToolPos(2, 1);
            QPoint mapToolPos(0, 4);

            QPixmap zone;

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(selectionPos * standardSize)));
            m_SelectionIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(drawToolPos * standardSize)));
            m_DrawToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(fillToolPos * standardSize)));
            m_FillToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(moveToolPos * standardSize)));
            m_MoveToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(eraseToolPos * standardSize)));
            m_EraseToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(mapToolPos * standardSize)));
            m_MapToolIcon.addPixmap(zone);

            QPoint translation(8, 0);

            selectionPos += translation;
            drawToolPos += translation;
            fillToolPos += translation;
            moveToolPos += translation;
            eraseToolPos += translation;
            mapToolPos += translation;

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(selectionPos * standardSize)));
            m_SelectionIcon.addPixmap(zone, QIcon::Active);
            m_SelectionIcon.addPixmap(zone, QIcon::Selected);
            m_HSelectionIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(drawToolPos * standardSize)));
            m_DrawToolIcon.addPixmap(zone, QIcon::Active);
            m_DrawToolIcon.addPixmap(zone, QIcon::Selected);
            m_HDrawToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(fillToolPos * standardSize)));
            m_FillToolIcon.addPixmap(zone, QIcon::Active);
            m_FillToolIcon.addPixmap(zone, QIcon::Selected);
            m_HFillToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(moveToolPos * standardSize)));
            m_MoveToolIcon.addPixmap(zone, QIcon::Active);
            m_MoveToolIcon.addPixmap(zone, QIcon::Selected);
            m_HMoveToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(eraseToolPos * standardSize)));
            m_EraseToolIcon.addPixmap(zone, QIcon::Active);
            m_EraseToolIcon.addPixmap(zone, QIcon::Selected);
            m_HEraseToolIcon.addPixmap(zone);

            zone.convertFromImage(m_IconAtlas.copy(rect.translated(mapToolPos * standardSize)));
            m_MapToolIcon.addPixmap(zone, QIcon::Active);
            m_MapToolIcon.addPixmap(zone, QIcon::Selected);
            m_HMapToolIcon.addPixmap(zone);

          }
        }
        QImage m_IconAtlas;
        QIcon m_SelectionIcon;
        QIcon m_DrawToolIcon;
        QIcon m_FillToolIcon;
        QIcon m_MoveToolIcon;
        QIcon m_EraseToolIcon;
        QIcon m_MapToolIcon;

        QIcon m_HSelectionIcon;
        QIcon m_HDrawToolIcon;
        QIcon m_HFillToolIcon;
        QIcon m_HMoveToolIcon;
        QIcon m_HEraseToolIcon;
        QIcon m_HMapToolIcon;
      };

      IconsStore const& GetStore()
      {
        static IconsStore s_Store;
        return s_Store;
      }
    }

    QIcon GetSelectionIcon()
    {
      return GetStore().m_SelectionIcon;
    }

    QIcon GetDrawToolIcon()
    {
      return GetStore().m_DrawToolIcon;
    }

    QIcon GetFillToolIcon()
    {
      return GetStore().m_FillToolIcon;
    }

    QIcon GetMoveToolIcon()
    {
      return GetStore().m_MoveToolIcon;
    }

    QIcon GetEraseToolIcon()
    {
      return GetStore().m_EraseToolIcon;
    }

    QIcon GetMapToolIcon()
    {
      return GetStore().m_MapToolIcon;
    }

    QIcon GetHighlightedSelectionIcon()
    {
      return GetStore().m_HSelectionIcon;
    }

    QIcon GetHighlightedDrawToolIcon()
    {
      return GetStore().m_HDrawToolIcon;
    }

    QIcon GetHighlightedFillToolIcon()
    {
      return GetStore().m_HFillToolIcon;
    }

    QIcon GetHighlightedMoveToolIcon()
    {
      return GetStore().m_HMoveToolIcon;
    }

    QIcon GetHighlightedEraseToolIcon()
    {
      return GetStore().m_HEraseToolIcon;
    }

    QIcon GetHighlightedMapToolIcon()
    {
      return GetStore().m_MapToolIcon;
    }
  }
}