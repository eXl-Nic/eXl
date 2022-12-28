#include "scriptwidget.hpp"

#include <editor/editorstate.hpp>

#include <lac/editor/EditorHighlighter.h>

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
    while (curParamPos < lastParentPos)
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

  lac::an::UserDefined BuildExlInformation(Vector<ResourceHandle<LuaFunctionLibrary>> const & iDeps)
  {
    World world(EditorState::BuildWorldConfig());
    world.AddSystem(std::make_unique<GameDatabase>(EditorState::GetProjectProperties()));
    LuaScriptSystem* luaScripts = world.AddSystem(std::make_unique<LuaScriptSystem>());
    for (auto dep : iDeps)
    {
      if (LuaFunctionLibrary const* lib = dep.GetOrLoad())
      {
        luaScripts->LoadScript(*lib);
      }
    }

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
            else if (memberName.find("__") != 0)
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

  lac::an::UserDefined BuildExlInformationForHandler(Vector<ResourceHandle<LuaFunctionLibrary>> const& iDeps, EventsManifest::FunctionsMap const& iFunctions)
  {
    lac::an::UserDefined baseInfo = BuildExlInformation(iDeps);

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

  ScriptEditorWidget::ScriptEditorWidget(QWidget* iParent)
    : lac::editor::LuaEditor(iParent)
  {
    highlighter()->useLuaRules();
    SetEngineFunctions();

    QFont font;
    font.setFamily("Consolas");
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPointSize(10);
    
    setFont(font);

    QFontMetrics metrics(font);
    setTabStopWidth(2 * metrics.width(' '));
  }

  void ScriptEditorWidget::SetEngineFunctions(Vector<ResourceHandle<LuaFunctionLibrary>> const& iDeps, EventsManifest::FunctionsMap const& iFunctions)
  {
    setUserDefined(BuildExlInformationForHandler(iDeps, iFunctions));
  }
}