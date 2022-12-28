#include "functionlibraryeditor.hpp"
#include <editor/editordef.hpp>
#include <editor/editorstate.hpp>
#include "scriptwidget.hpp"
#include "functionlibwidget.hpp"

#include <engine/game/commondef.hpp>
#include <engine/script/luafunctionlibrary.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QSplitter>

namespace eXl
{
  
  ResourceEditorHandler& LuaFunctionLibraryEditor::GetEditorHandler()
  {
    static EditorHandler_T<LuaFunctionLibrary, LuaFunctionLibraryEditor> s_Handler;
    return s_Handler;
  }

  struct LuaFunctionLibraryEditor::Impl
  {
    Impl()
    {
    }

    void GenerateDefaultLibrarySource();

    LuaFunctionLibraryEditor* m_Editor;
    LuaFunctionLibrary* m_Script;

    QComboBox* m_BehaviourSelector;
    ScriptEditorWidget* m_ScriptSrc;
    Vector<String> m_Behaviours;
  };

  void LuaFunctionLibraryEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  LuaFunctionLibraryEditor::LuaFunctionLibraryEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl)
  {
    m_Impl->m_Editor = this;
    m_Impl->m_Script = LuaFunctionLibrary::DynamicCast(iDoc->GetResource());

    QSplitter* rootSplitter = new QSplitter(this);

    LuaFunctionLibrarySelector* libSel = new LuaFunctionLibrarySelector(this, m_Impl->m_Script->m_Dependencies);
    rootSplitter->addWidget(libSel);

    QWidget* editorWidget = new QWidget(this);
    {
      QVBoxLayout* layout = new QVBoxLayout(editorWidget);
      m_Impl->m_ScriptSrc = new ScriptEditorWidget(editorWidget);
      
      m_Impl->m_ScriptSrc->setPlainText(QString::fromUtf8(m_Impl->m_Script->m_Script.c_str()));

      QObject::connect(libSel, &LuaFunctionLibrarySelector::onListChanged, [this, libSel]()
        {
          m_Impl->m_Script->m_Dependencies = libSel->GetList();
          m_Impl->m_ScriptSrc->SetEngineFunctions(m_Impl->m_Script->m_Dependencies);
          m_Impl->m_Editor->ModifyResource();
        });

      layout->addWidget(m_Impl->m_ScriptSrc);

      QObject::connect(m_Impl->m_ScriptSrc, &QPlainTextEdit::textChanged, [this]
        {
          m_Impl->m_Script->m_Script = m_Impl->m_ScriptSrc->toPlainText().toUtf8().data();
          m_Impl->m_Editor->ModifyResource();
        });
    }

    rootSplitter->addWidget(editorWidget);
    rootSplitter->setSizes(QList<int>({ 200, 1000 }));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(rootSplitter);
    setLayout(layout);

    if (m_Impl->m_Script->m_Script.empty())
    {
      m_Impl->GenerateDefaultLibrarySource();
    }
  }

  void LuaFunctionLibraryEditor::Impl::GenerateDefaultLibrarySource()
  {
    String scriptObjName = m_Script->GetName();
    scriptObjName.append("_functions");

    String defaultScript;
    defaultScript.append("local module ");
    defaultScript.append(scriptObjName);
    defaultScript.append(" = {}\n\n");

    defaultScript.append("return { \"namespace\"=");
    defaultScript.append(m_Script->GetName());
    defaultScript.append(", \"functions\"=");
    defaultScript.append(scriptObjName);
    defaultScript.append("}\n");

    m_Script->m_Script = std::move(defaultScript);
    m_ScriptSrc->setPlainText(QString::fromUtf8(m_Script->m_Script.c_str()));
  }
}