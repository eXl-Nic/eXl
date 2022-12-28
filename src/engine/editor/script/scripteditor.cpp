#include "scripteditor.hpp"
#include <editor/editordef.hpp>
#include <editor/editorstate.hpp>
#include "scriptwidget.hpp"
#include "functionlibwidget.hpp"

#include <editor/collectionmodel.hpp>

#include <core/type/typemanager.hpp>

#include <engine/game/commondef.hpp>
#include <engine/script/luaeventhandler.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QSplitter>

#include <core/lua/luamanager.hpp>
#include <core/lua/luabind/detail/class_registry.hpp>
#include <core/lua/luabind/class_info.hpp>
#include <engine/common/world.hpp>
#include <engine/common/gamedatabase.hpp>
#include <engine/game/commondef.hpp>
#include <engine/script/luascriptsystem.hpp>


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
    Vector<String> m_Behaviours;
    ScriptEditorWidget* m_ScriptSrc;
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

    QSplitter* rootSplitter = new QSplitter(this);
    
    LuaFunctionLibrarySelector* libSel = new LuaFunctionLibrarySelector(this, m_Impl->m_Script->m_Dependencies);
    rootSplitter->addWidget(libSel);

    QWidget* editorWidget = new QWidget(this);
    {
      QVBoxLayout* layout = new QVBoxLayout(editorWidget);

      m_Impl->m_BehaviourSelector = new QComboBox(this);

      EventsManifest const& manifest = EditorState::BuildWorldConfig().m_Events;
      m_Impl->m_BehaviourSelector->addItem("<empty>");
      for (auto const& entry : manifest.m_Interfaces)
      {
        EventsManifest const& manifest = EngineCommon::GetBaseEvents();
        m_Impl->m_Behaviours.push_back(entry.first);
        m_Impl->m_BehaviourSelector->addItem(QString::fromUtf8(entry.first.c_str()));
      }

      QObject::connect(m_Impl->m_BehaviourSelector, (void (QComboBox::*)(int)) & QComboBox::currentIndexChanged,
        [this](int iIndex)
        {
          EventsManifest const& manifest = EngineCommon::GetBaseEvents();
          if (iIndex == 0)
          {
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
            auto iter = manifest.m_Interfaces.find(itfName);
            m_Impl->m_ScriptSrc->SetEngineFunctions(m_Impl->m_Script->m_Dependencies, iter->second);
          }
        });

      layout->addWidget(m_Impl->m_BehaviourSelector);
      m_Impl->m_ScriptSrc = new ScriptEditorWidget(this);
      layout->addWidget(m_Impl->m_ScriptSrc);
      m_Impl->m_ScriptSrc->setPlainText(QString::fromUtf8(m_Impl->m_Script->m_Script.c_str()));

      QObject::connect(libSel, &LuaFunctionLibrarySelector::onListChanged, [this, libSel]()
        {
          EventsManifest const& manifest = EngineCommon::GetBaseEvents();
          auto iter = manifest.m_Interfaces.find(m_Impl->m_Script->m_InterfaceName);
          
          m_Impl->m_Script->m_Dependencies = libSel->GetList();
          m_Impl->m_ScriptSrc->SetEngineFunctions(m_Impl->m_Script->m_Dependencies, iter != manifest.m_Interfaces.end() ? iter->second : eXl::EventsManifest::FunctionsMap());
          m_Impl->m_Editor->ModifyResource();
        });

      auto iter = manifest.m_Interfaces.find(m_Impl->m_Script->m_InterfaceName);
      if (iter != manifest.m_Interfaces.end())
      {
        auto iterIdx = std::find(m_Impl->m_Behaviours.begin(), m_Impl->m_Behaviours.end(), m_Impl->m_Script->m_InterfaceName);
        uint32_t index = iterIdx - m_Impl->m_Behaviours.begin();
        m_Impl->m_BehaviourSelector->setCurrentIndex(index + 1);
        m_Impl->m_ScriptSrc->SetEngineFunctions(m_Impl->m_Script->m_Dependencies, iter->second);
      }

      QObject::connect(m_Impl->m_ScriptSrc, &QPlainTextEdit::textChanged, [this]
        {
          m_Impl->m_Script->m_Script = m_Impl->m_ScriptSrc->toPlainText().toUtf8().data();
          m_Impl->m_Editor->ModifyResource();
        });

      rootSplitter->addWidget(editorWidget);
    }
    rootSplitter->setSizes(QList<int>({ 200, 1000 }));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(rootSplitter);
    setLayout(layout);
  }

  void LuaScriptEditor::Impl::GenerateDefaultBehaviourSource()
  {
    EventsManifest const& manifest = EngineCommon::GetBaseEvents();
    auto iter = manifest.m_Interfaces.find(m_Script->m_InterfaceName);
    if (iter == manifest.m_Interfaces.end())
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
    defaultScript.append("Init(object)\n\nend\n\n");

    for (auto const funDesc : iter->second)
    {
      defaultScript.append("function ");
      defaultScript.append(funDesc.first);
      defaultScript.append("(self, object");

      for (uint32_t i = 0; i<funDesc.second.GetArgs().size(); ++i)
      {
        Type const* type = funDesc.second.GetArgs()[i];
        defaultScript.append(", ");
        defaultScript.append(type->GetDisplayName());
        defaultScript.append("_arg");
        defaultScript.append(StringUtil::FromInt(i));
      }
      defaultScript.append(")\n");
      if (funDesc.second.GetRetType() != nullptr)
      {
        defaultScript.append("--return ");
        defaultScript.append(funDesc.second.GetRetType()->GetDisplayName());
        defaultScript.append("()\n");
      }
      defaultScript.append("end\n\n");
    }

    defaultScript.append("local handler = {}\n");

    for (auto const funDesc : iter->second)
    {
      defaultScript.append("handler[\"");
      defaultScript.append(funDesc.first);
      defaultScript.append("\"]=");
      defaultScript.append(funDesc.first);
      defaultScript.append("\n");
    }

    defaultScript.append("return handler");

    m_Script->m_Script = std::move(defaultScript);
    m_ScriptSrc->setPlainText(QString::fromUtf8(m_Script->m_Script.c_str()));
  }
}