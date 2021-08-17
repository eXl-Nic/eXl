#ifndef OBJECTPTR_EDITOR_H
#define OBJECTPTR_EDITOR_H

#if 0

#include "../dynobjecteditor.hpp"

namespace Ui {
class ObjectPtr_Editor;
}

class ObjectPtr_Editor : public DynObjectEditor
{
  Q_OBJECT
  
public:

  static DynObjectEditor* Construct(QWidget *parent)
  {return new ObjectPtr_Editor(parent);}

  explicit ObjectPtr_Editor(QWidget *parent = 0);
  ~ObjectPtr_Editor();
  
signals:
  void editingFinished();

protected:

  void UpdateView();

private slots:
  void on_RttiList_currentIndexChanged(int index);

private:
  Ui::ObjectPtr_Editor *ui;
};

#endif

#endif // OBJECTPTR_EDITOR_H
