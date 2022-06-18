#include "scripteditor.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"
#include "luahighlighter.hpp"

#include "collectionmodel.hpp"

#include <core/type/typemanager.hpp>

#include <engine/game/commondef.hpp>
#include <engine/script/luaeventhandler.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QTextCharFormat>
#include <QTextEdit>

class QTextDocument;

namespace eXl
{
  
  ResourceEditorHandler& LuaScriptEditor::GetEditorHandler()
  {
    static EditorHandler_T<LuaEventHandler, LuaScriptEditor> s_Handler;
    return s_Handler;
  }

  struct LuaScriptEditor::Impl
  {
    Impl()
    {
    }

    void GenerateDefaultBehaviourSource();

    LuaScriptEditor* m_Editor;
    LuaEventHandler* m_Script;

    QComboBox* m_BehaviourSelector;
    QTextEdit* m_ScriptSrc;
    Vector<String> m_Behaviours;
  };

  void LuaScriptEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  LuaScriptEditor::LuaScriptEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl)
  {
    m_Impl->m_Editor = this;
    m_Impl->m_Script = LuaEventHandler::DynamicCast(iDoc->GetResource());

    QVBoxLayout* layout = new QVBoxLayout(this);

    m_Impl->m_BehaviourSelector = new QComboBox(this);
    EventsManifest const& manifest = EngineCommon::GetBaseEvents();
    m_Impl->m_BehaviourSelector->addItem("<empty>");
    for (auto const& entry : manifest.m_Interfaces)
    {
      m_Impl->m_Behaviours.push_back(entry.first);      
      m_Impl->m_BehaviourSelector->addItem(QString::fromUtf8(entry.first.c_str()));
    }

    auto iter = manifest.m_Interfaces.find(m_Impl->m_Script->m_InterfaceName);
    if (iter != manifest.m_Interfaces.end())
    {
      auto iter = std::find(m_Impl->m_Behaviours.begin(), m_Impl->m_Behaviours.end(), m_Impl->m_Script->m_InterfaceName);
      uint32_t index = iter - m_Impl->m_Behaviours.begin();
      m_Impl->m_BehaviourSelector->setCurrentIndex(index + 1);
    }

    QObject::connect(m_Impl->m_BehaviourSelector, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged,
      [this](int iIndex)
    { 
      if (iIndex == 0)
      {
        EventsManifest const& manifest = EngineCommon::GetBaseEvents();
        auto iter = manifest.m_Interfaces.find(m_Impl->m_Script->m_InterfaceName);
        if (iter == manifest.m_Interfaces.end())
        {
          return;
        }
        m_Impl->m_Script->m_InterfaceName = "";
        m_Impl->m_Editor->ModifyResource();
      }
      else
      {
        String itfName = m_Impl->m_Behaviours[iIndex - 1];
        if (itfName == m_Impl->m_Script->m_InterfaceName)
        {
          return;
        }
        m_Impl->m_Script->m_InterfaceName = itfName;
        m_Impl->m_Editor->ModifyResource();

        if (m_Impl->m_Script->m_Script.empty())
        {
          m_Impl->GenerateDefaultBehaviourSource();
        }
      }
    });

    layout->addWidget(m_Impl->m_BehaviourSelector); 

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
  }

  void LuaScriptEditor::Impl::GenerateDefaultBehaviourSource()
  {
    EventsManifest const& manifest = EngineCommon::GetBaseEvents();
    auto iter = manifest.m_Interfaces.find(m_Script->m_InterfaceName);
    if (iter != manifest.m_Interfaces.end())
    {
      return;
    }
    String scriptObjName = m_Script->GetName();
    scriptObjName.append("_");
    scriptObjName.append(iter->first);
    scriptObjName.append("_script");
    
    String defaultScript;
    defaultScript.append("local module ");
    defaultScript.append(scriptObjName);
    defaultScript.append(" = {}\n\n");
    
    defaultScript.append("function ");
    defaultScript.append(scriptObjName);
    defaultScript.append(".Init(object)\n\nend\n\n");

    for (auto const funDesc : iter->second)
    {
      defaultScript.append("function ");
      defaultScript.append(scriptObjName);
      defaultScript.append(".");
      defaultScript.append(funDesc.first);
      defaultScript.append("(self, object");

      for (uint32_t i = 0; i<funDesc.second.GetArgs().size(); ++i)
      {
        Type const* type = funDesc.second.GetArgs()[i];
        defaultScript.append(", ");
        defaultScript.append(type->GetName());
        defaultScript.append("_arg");
        defaultScript.append(StringUtil::FromInt(i));
      }
      defaultScript.append(")\n");
      if (funDesc.second.GetRetType() != nullptr)
      {
        defaultScript.append("--return ");
        defaultScript.append(funDesc.second.GetRetType()->GetName());
        defaultScript.append("()\n");
      }
      defaultScript.append("end\n\n");
    }
    defaultScript.append("return ");
    defaultScript.append(scriptObjName);
    defaultScript.append("\n");

    m_Script->m_Script = std::move(defaultScript);
    m_ScriptSrc->setText(QString::fromUtf8(m_Script->m_Script.c_str()));
  }
}