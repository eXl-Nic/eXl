#include "functionlibraryeditor.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"
#include "luahighlighter.hpp"

#include <engine/game/commondef.hpp>
#include <engine/script/luafunctionlibrary.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextEdit>

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
    QTextEdit* m_ScriptSrc;
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

    QVBoxLayout* layout = new QVBoxLayout(this);

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
    m_ScriptSrc->setText(QString::fromUtf8(m_Script->m_Script.c_str()));
  }
}