#ifndef ENUM_EDITOR_H
#define ENUM_EDITOR_H

#include "../dynobjecteditor.hpp"

namespace Ui {
class Enum_Editor;
}

class Enum_Editor : public DynObjectEditor
{
  Q_OBJECT
  
public:

  static DynObjectEditor* Construct(QWidget *parent)
  {return new Enum_Editor(parent);}

  explicit Enum_Editor(QWidget *parent = 0);
  ~Enum_Editor();

protected:

  void UpdateView();

private slots:
  void on_EnumNames_currentIndexChanged(int index);

private:
  Ui::Enum_Editor *ui;
};

#endif // ENUM_EDITOR_H
