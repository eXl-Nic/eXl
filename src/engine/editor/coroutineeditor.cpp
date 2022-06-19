#include "coroutineeditor.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"
#include "luahighlighter.hpp"

#include "collectionmodel.hpp"

#include <core/type/typemanager.hpp>

#include <engine/game/commondef.hpp>
#include <engine/script/luacoroutine.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>

class QTextDocument;

namespace eXl
{

  ResourceEditorHandler& LuaCoroutineEditor::GetEditorHandler()
  {
    static EditorHandler_T<LuaCoroutine, LuaCoroutineEditor> s_Handler;
    return s_Handler;
  }

  struct LuaCoroutineEditor::Impl
  {
    Impl()
    {
    }

    void GenerateDefaultCoroutineSource();

    LuaCoroutineEditor* m_Editor;
    LuaCoroutine* m_Script;

    QTextEdit* m_ScriptSrc;
  };

  void LuaCoroutineEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  LuaCoroutineEditor::LuaCoroutineEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl)
  {
    m_Impl->m_Editor = this;
    m_Impl->m_Script = LuaCoroutine::DynamicCast(iDoc->GetResource());

    QVBoxLayout* layout = new QVBoxLayout(this);


    {
      QWidget* stepTimeWidget = new QWidget(this);
      QHBoxLayout* stepTimeLayout = new QHBoxLayout(stepTimeWidget);
      stepTimeWidget->setLayout(stepTimeLayout);
      QLabel* label = new QLabel("Default Step Time", stepTimeWidget);
      stepTimeLayout->addWidget(label);
      QDoubleSpinBox* stepTimeEntry = new QDoubleSpinBox(this);
      stepTimeEntry->setValue(m_Impl->m_Script->m_DefaultTickRate);
      stepTimeLayout->addWidget(stepTimeEntry);

      QObject::connect(stepTimeEntry, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](double iValue)
        {
          m_Impl->m_Script->m_DefaultTickRate = iValue;
          m_Impl->m_Editor->ModifyResource();
        });
      layout->addWidget(stepTimeWidget);
    }

    {
      QWidget* startPausedWidget = new QWidget(this);
      QHBoxLayout* startPausedLayout = new QHBoxLayout(startPausedWidget);
      startPausedWidget->setLayout(startPausedLayout);
      QLabel* label = new QLabel("Start Paused", startPausedWidget);
      startPausedLayout->addWidget(label);
      QCheckBox* startPausedEntry = new QCheckBox(this);
      startPausedEntry->setChecked(m_Impl->m_Script->m_DefaultStartPaused);
      startPausedLayout->addWidget(startPausedEntry);

      QObject::connect(startPausedEntry, &QCheckBox::stateChanged, [this, startPausedEntry](int)
        {
          m_Impl->m_Script->m_DefaultStartPaused = startPausedEntry->checkState() == Qt::Checked;
          m_Impl->m_Editor->ModifyResource();
        });
      layout->addWidget(startPausedWidget);
    }

    m_Impl->m_ScriptSrc = new QTextEdit(this);
    m_Impl->m_ScriptSrc->setTabStopWidth(m_Impl->m_ScriptSrc->tabStopWidth() / 2);
    new LuaHighlighter(m_Impl->m_ScriptSrc->document());
    m_Impl->m_ScriptSrc->setText(QString::fromUtf8(m_Impl->m_Script->m_Script.c_str()));

    layout->addWidget(m_Impl->m_ScriptSrc);

    QObject::connect(m_Impl->m_ScriptSrc, &QTextEdit::textChanged, [this]
      {
        m_Impl->m_Script->m_Script = m_Impl->m_ScriptSrc->toPlainText().toUtf8().data();
        m_Impl->m_Editor->ModifyResource();
      });

    setLayout(layout);

    if (m_Impl->m_Script->m_Script.empty())
    {
      m_Impl->GenerateDefaultCoroutineSource();
    }
  }

  void LuaCoroutineEditor::Impl::GenerateDefaultCoroutineSource()
  {
    String scriptObjName = m_Script->GetName();
    scriptObjName.append("_routine");

    String defaultScript;
    defaultScript.append("local module ");
    defaultScript.append(scriptObjName);
    defaultScript.append(" = {}\n\n");

    defaultScript.append("function ");
    defaultScript.append(scriptObjName);
    defaultScript.append(".Start(object)\n\treturn {}\nend\n\n");

    defaultScript.append("function ");
    defaultScript.append(scriptObjName);
    defaultScript.append(".Terminate(self, object)\n\nend\n\n");

    defaultScript.append("function ");
    defaultScript.append(scriptObjName);
    defaultScript.append(".Paused(self, object)\n\nend\n\n");

    defaultScript.append("function ");
    defaultScript.append(scriptObjName);
    defaultScript.append(".Resume(self, object)\n\nend\n\n");

    defaultScript.append("function ");
    defaultScript.append(scriptObjName);
    defaultScript.append(".Step(self, coroutine, object, elapsedTime)\n\nend\n\n");

    defaultScript.append("return ");
    defaultScript.append(scriptObjName);
    defaultScript.append("\n");

    m_Script->m_Script = std::move(defaultScript);
    m_ScriptSrc->setText(QString::fromUtf8(m_Script->m_Script.c_str()));
  }
}