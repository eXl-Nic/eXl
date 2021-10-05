#pragma once

#include <QIcon>

namespace eXl
{
  namespace EditorIcons
  {
    QIcon GetPlayIcon();

    QIcon GetSelectionIcon();
    QIcon GetDrawToolIcon();
    QIcon GetFillToolIcon();
    QIcon GetMoveToolIcon();
    QIcon GetEraseToolIcon();
    QIcon GetMapToolIcon();

    // To force highlight on non-cooperating toolbars.
    QIcon GetHighlightedSelectionIcon();
    QIcon GetHighlightedDrawToolIcon();
    QIcon GetHighlightedFillToolIcon();
    QIcon GetHighlightedMoveToolIcon();
    QIcon GetHighlightedEraseToolIcon();
    QIcon GetHighlightedMapToolIcon();
  }
}