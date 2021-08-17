#pragma once

#include <QWidget>
#include <QBoxLayout>

class AspectRatioWidget : public QWidget
{
  Q_OBJECT
public:
  AspectRatioWidget(QWidget *widget, float width, float height, QWidget *parent = 0);
  void resizeEvent(QResizeEvent *event);
  void SetAspectRatio(float w, float h);

private:
  QBoxLayout *layout;
  float arWidth; // aspect ratio width
  float arHeight; // aspect ratio height
};
