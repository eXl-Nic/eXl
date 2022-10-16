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

#include <lac/editor/LuaEditor.h>
#include <lac/editor/EditorHighlighter.h>

#include <core/lua/luamanager.hpp>
#include <core/lua/luabind/detail/class_registry.hpp>
#include <core/lua/luabind/class_info.hpp>
#include <engine/common/world.hpp>
#include <engine/common/gamedatabase.hpp>
#include <engine/game/commondef.hpp>
#include <engine/script/luascriptsystem.hpp>


namespace eXl
{
  void CleanupCPPDecorators(std::string& signature)
  {
    size_t amperPos = signature.find("&");
    while (amperPos != String::npos)
    {
      signature.replace(amperPos, 1, "");
      amperPos = signature.find("&");
    }
    size_t starPos = signature.find("*");
    while (starPos != String::npos)
    {
      signature.replace(starPos, 1, "");
      starPos = signature.find("*");
    }
    size_t constPos = signature.find("const");
    while (constPos != String::npos)
    {
      signature.replace(constPos, 5, "");
      constPos = signature.find("const");
    }
    size_t voidPos = signature.find("void");
    while (voidPos != String::npos)
    {
      signature.replace(voidPos, 4, "");
      voidPos = signature.find("void");
    }
    while (std::isspace(*signature.begin()) != 0)
    {
      signature = signature.substr(1);
    }
  }

  size_t TrimWhFw(std::string const& iStr, size_t iPos, size_t iEnd)
  {
    while (iPos < iEnd && std::isspace(iStr[iPos + 1]) != 0)
    {
      iPos++;
    }
    return iPos;
  }

  size_t TrimWhBw(std::string const& iStr, size_t iPos, size_t iBegin)
  {
    while (iPos > iBegin && std::isspace(iStr[iPos - 1]) != 0)
    {
      iPos--;
    }
    return iPos;
  }

  void NameArgs(std::string& signature)
  {
    auto parentPos = signature.find('(');
    auto curComaPos = signature.find(',');
    auto lastParentPos = signature.find(')');

    size_t paramEnd = curComaPos;
    if (curComaPos == String::npos)
    {
      curComaPos = lastParentPos;
      paramEnd = lastParentPos;
    }
    uint32_t arg = 0;
    size_t curParamPos = parentPos + 1;
    curParamPos = TrimWhFw(signature, curParamPos, paramEnd);
    while(curParamPos < lastParentPos)
    {
      paramEnd = TrimWhBw(signature, paramEnd - 1, curParamPos) + 1;

      String argName = StringUtil::Format(" arg_%i", arg);
      signature.insert(paramEnd, argName);
      ++arg;
      curComaPos += argName.size();
      lastParentPos += argName.size();

      curParamPos = TrimWhFw(signature, curComaPos + 1, lastParentPos);
      curComaPos = signature.find(',', curParamPos + 1);
      paramEnd = curComaPos;
      if (curComaPos == String::npos)
      {
        curComaPos = lastParentPos;
        paramEnd = lastParentPos;
      }
    }
  }

  lac::an::UserDefined BuildExlInformation()
  {
    World world(EditorState::BuildWorldConfig());
    world.AddSystem(std::make_unique<GameDatabase>(EditorState::GetProjectProperties()));
    LuaScriptSystem* luaScripts = world.AddSystem(std::make_unique<LuaScriptSystem>());

    lac::an::UserDefined ud;

    lac::an::TypeInfo eXlModule;
    eXlModule.name = "eXl";
    eXlModule.type = lac::an::Type::table;

    LuaWorld& luaWorld = luaScripts->GetLuaWorld();
    {
      LuaStateHandle stateHandle = luaWorld.GetState();
      lua_State* luaState = stateHandle.GetState();
      auto* classReg = luabind::detail::class_registry::get_registry(luaState);
      
      for (auto const& entry : classReg->get_classes())
      {
        auto* classInfo = entry.second;

        luabind::type_id type = classInfo->type();
        
        TupleType const* tuple = type.get_id()->IsTuple();

        lac::an::TypeInfo newType;
      
        newType.name = classInfo->name();
        newType.type = lac::an::Type::table;

        luabind::detail::stack_pop pop(luaState, 1);
        classInfo->get_table(luaState);
        luabind::table classTable(luabind::from_stack(luaState, -1));
        for (luabind::iterator iter(classTable); iter != luabind::iterator(); ++iter)
        {
          if (luabind::type(*iter) != LUA_TFUNCTION)
            continue;

          luabind::object member(*iter);
          member.push(luaState);
          luabind::detail::stack_pop pop(luaState, 1);

          auto key = iter.key();
          auto memberName = luabind::to_string(key);
          if (lua_tocfunction(luaState, -1) == &luabind::detail::property_tag)
          {
            if (tuple)
            {
              if (Type const* fieldType = tuple->GetFieldDetails(TypeFieldName(luabind::to_string(key))))
              {
                KString nT = fieldType->GetLuaNativeType();
                if (nT.empty())
                {
                  newType.members[memberName] = lac::an::Type::userdata;
                  newType.members[memberName].name = fieldType->GetName();
                }
                else
                {
                  if (nT == "number")
                  {
                    newType.members[memberName] = lac::an::Type::number;
                  }
                  else if (nT == "string")
                  {
                    newType.members[memberName] = lac::an::Type::string;
                  }
                  else if (nT == "int")
                  {
                    newType.members[memberName] = lac::an::Type::number;
                  }
                  else if (nT == "bool")
                  {
                    newType.members[memberName] = lac::an::Type::boolean;
                  }
                }
              }
            }
            else
            {
              luabind::object propFun(luabind::from_stack(luaState, -1));
              luabind::object getterFun = std::get<1>(luabind::getupvalue(propFun, 1));
              getterFun.push(luaState);
              eXl_ASSERT(luabind::detail::is_luabind_function(luaState, -1));
              luabind::object getterFunObj = std::get<1>(luabind::getupvalue(getterFun, 1));
              auto const& functionObj = *luabind::touserdata<luabind::detail::function_object*>(getterFunObj);
              
              functionObj->format_signature(luaState, memberName.c_str());
              std::string signature = luabind::to_string(luabind::object(luabind::from_stack(luaState, -1)));

              auto namePos = signature.find(memberName + "(");
              if (namePos != std::string::npos)
              {
                String type(signature.substr(0, namePos));
                if (type == "float")
                {
                  newType.members[memberName] = lac::an::Type::number;
                }
                else if (type == "String")
                {
                  newType.members[memberName] = lac::an::Type::string;
                }
                else if (type.find("int") == 0)
                {
                  newType.members[memberName] = lac::an::Type::number;
                }
                else if (type == "bool")
                {
                  newType.members[memberName] = lac::an::Type::boolean;
                }
                else
                {
                  newType.members[memberName] = lac::an::Type::userdata;
                  newType.members[memberName].name = type;
                }
              }
            }
          }
          else
          {
              if (!luabind::detail::is_luabind_function(luaState, -1)) 
              {
                continue;
              }

              auto const& functionObj = *luabind::touserdata<luabind::detail::function_object*>(std::get<1>(luabind::getupvalue(member, 1)));
              luabind::detail::stack_pop pop(luaState, 1);
              functionObj->format_signature(luaState, memberName.c_str());
              std::string signature = luabind::to_string(luabind::object(luabind::from_stack(luaState, -1)));

              KString const ctorPrefix = "void __init(luabind::argument const&";
              bool const isCtor = memberName == "__init";

              if (isCtor)
              {
                if (signature.find(ctorPrefix) == 0)
                {
                  size_t toReplace = ctorPrefix.size();
                  if (signature[toReplace] == ',')
                  {
                    ++toReplace;
                  }
                  signature.replace(0, toReplace, newType.name + " function(");
                  CleanupCPPDecorators(signature);
                  NameArgs(signature);
                  lac::an::TypeInfo ti(signature);
                  if (ti.type == lac::an::Type::error)
                  {
                    LOG_ERROR << "Could not parse signature for " << memberName << " : " << signature;
                  }
                  eXlModule.members.insert(std::make_pair(newType.name, ti));
                }
              }
              else if(memberName.find("__") != 0)
              {
                auto namePos = signature.find(memberName + "(");
                if (namePos != std::string::npos)
                {
                  bool isMethod = false;
                  auto parentPos = signature.find('(');
                  auto firstComaPos = signature.find(',');
                  auto lastParentPos = signature.find(')');
                  size_t firstParamEnd;
                  if (parentPos != std::string::npos
                    && (firstComaPos != std::string::npos || lastParentPos != std::string::npos)
                    && parentPos < firstComaPos)
                  {
                    ++parentPos;
                    firstParamEnd = firstComaPos != std::string::npos ? firstComaPos : lastParentPos;

                    parentPos = TrimWhFw(signature, parentPos, firstParamEnd);
                    firstParamEnd = TrimWhBw(signature, firstParamEnd, parentPos);

                    if (signature.substr(parentPos, firstParamEnd - parentPos).find(newType.name) == 0)
                    {
                      isMethod = true;
                    }
                  }
                  if (isMethod)
                  {
                    size_t replaceEnd = firstParamEnd + 1;
                    if (firstParamEnd == lastParentPos)
                    {
                      --replaceEnd;
                    }
                    signature.replace(signature.begin() + parentPos, signature.begin() + replaceEnd, "");
                    signature.replace(namePos, memberName.size(), "method");
                  }
                  else
                  {
                    signature.replace(namePos, memberName.size(), "function");
                  }
                }
                CleanupCPPDecorators(signature);
                NameArgs(signature);
                lac::an::TypeInfo ti(signature);
                if (ti.type == lac::an::Type::error)
                {
                  LOG_ERROR << "Could not parse signature for " << memberName << " : " << signature;
                }

                newType.members.insert(std::make_pair(memberName, ti));
                //LOG_INFO << memberName << "->" << signature;
              }
          }
        }
        ud.addType(newType);
      }

      eXlModule.members.insert(std::make_pair("GetWorld", "World function()"));
    }
    ud.addType(std::move(eXlModule));
    return ud;
  }

  lac::an::UserDefined BuildExlInformationForHandler(EventsManifest::FunctionsMap const& iFunctions)
  {
    lac::an::UserDefined baseInfo = BuildExlInformation();

    for (auto const& entryPoint : iFunctions)
    {
      String funcArgs("function(table self, ObjectHandle obj,");

      for (uint32_t i = 0; i < entryPoint.second.GetArgs().size(); ++i)
      {
        Type const* type = entryPoint.second.GetArgs()[i];
        funcArgs += ' ';
        funcArgs += type->GetDisplayName();
        funcArgs += ' ';
        funcArgs += "arg";
        funcArgs += StringUtil::FromInt(i);
        funcArgs += ',';
      }
      funcArgs.back() = ')';

      baseInfo.addScriptInput(entryPoint.first, funcArgs.c_str());
    }

    return baseInfo;
  }
  
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
    lac::editor::LuaEditor* m_ScriptSrc;
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

    QObject::connect(m_Impl->m_BehaviourSelector, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged,
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
        m_Impl->m_ScriptSrc->setUserDefined(BuildExlInformationForHandler(iter->second));
      }
    });

    layout->addWidget(m_Impl->m_BehaviourSelector); 

    //m_Impl->m_ScriptSrc = new QTextEdit(this);
    //m_Impl->m_ScriptSrc->setTabStopWidth(m_Impl->m_ScriptSrc->tabStopWidth() / 2);
    //new LuaHighlighter(m_Impl->m_ScriptSrc->document());
    //m_Impl->m_ScriptSrc->setText(QString::fromUtf8(m_Impl->m_Script->m_Script.c_str()));
    //
    //layout->addWidget(m_Impl->m_ScriptSrc);
    //
    //QObject::connect(m_Impl->m_ScriptSrc, &QTextEdit::textChanged, [this]
    //{
    //  m_Impl->m_Script->m_Script = m_Impl->m_ScriptSrc->toPlainText().toUtf8().data();
    //  m_Impl->m_Editor->ModifyResource();
    //});

    m_Impl->m_ScriptSrc = new lac::editor::LuaEditor(this);
    layout->addWidget(m_Impl->m_ScriptSrc);
    m_Impl->m_ScriptSrc->highlighter()->useLuaRules();
    m_Impl->m_ScriptSrc->setPlainText(QString::fromUtf8(m_Impl->m_Script->m_Script.c_str()));

    auto iter = manifest.m_Interfaces.find(m_Impl->m_Script->m_InterfaceName);
    if (iter != manifest.m_Interfaces.end())
    {
      auto iterIdx = std::find(m_Impl->m_Behaviours.begin(), m_Impl->m_Behaviours.end(), m_Impl->m_Script->m_InterfaceName);
      uint32_t index = iterIdx - m_Impl->m_Behaviours.begin();
      m_Impl->m_BehaviourSelector->setCurrentIndex(index + 1);
      m_Impl->m_ScriptSrc->setUserDefined(BuildExlInformationForHandler(iter->second));
    }

    QObject::connect(m_Impl->m_ScriptSrc, &QPlainTextEdit::textChanged, [this]
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