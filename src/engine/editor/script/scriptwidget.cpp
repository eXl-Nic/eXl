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

  void TypeNameToTypeInfo(KString iName, lac::an::TypeInfo& oInfo)
  {
    if (iName == "float")
    {
      oInfo = lac::an::Type::number;
    }
    else if (iName == "String")
    {
      oInfo = lac::an::Type::string;
    }
    else if (iName.find("int") == 0)
    {
      oInfo = lac::an::Type::number;
    }
    else if (iName == "bool")
    {
      oInfo = lac::an::Type::boolean;
    }
    else
    {
      oInfo = lac::an::Type::userdata;
      oInfo.name = iName;
    }
  }

  String LuaTypeName(Type const* iType) {
    String luaTypeName = iType->GetName();
    size_t found = luaTypeName.find(":");
    while (found != String::npos) {
      luaTypeName[found] = '_';
      found = luaTypeName.find(":", found);
    }
    lac::an::TypeInfo info;
    TypeNameToTypeInfo(luaTypeName, info);
    if (info.type == lac::an::Type::userdata) {
      return luaTypeName;
    }
    else {
      switch (info.type) {
      case lac::an::Type::number:
        return "number";
        break;
      case lac::an::Type::string:
        return "string";
        break;
      case lac::an::Type::boolean:
        return "boolean";
        break;
      default:
        return "unknown";
      }
    }
  }

  void RegisterLuabindFunction( lua_State* luaState, 
    const ArrayType * iArrayType, 
    const luabind::object& iFunObj, 
    KString iName, 
    KString iFullTypeName,
    KString iTypeName, 
    lac::an::UserDefined::TypeMap& oParentNS, 
    lac::an::UserDefined::TypeMap& oMap )
  {
    auto const& functionObj = *luabind::touserdata<luabind::detail::function_object*>(std::get<1>(luabind::getupvalue(iFunObj, 1)));
    luabind::detail::stack_pop pop(luaState, 1);
    functionObj->format_signature(luaState, iName.data());
    std::string signature = luabind::to_string(luabind::object(luabind::from_stack(luaState, -1)));

    KString const ctorPrefix = "void __init(luabind::argument const&";
    bool const isCtor = iName == "__init";

    if (isCtor)
    {
      if (signature.find(ctorPrefix) == 0)
      {
        size_t toReplace = ctorPrefix.size();
        if (signature[toReplace] == ',')
        {
          ++toReplace;
        }
        signature.replace(0, toReplace, String(iFullTypeName) + " function(");
        CleanupCPPDecorators(signature);
        NameArgs(signature);
        lac::an::TypeInfo ti(signature);
        if (ti.type == lac::an::Type::error)
        {
          LOG_ERROR << "Could not parse signature for " << iName << " : " << signature;
        }
        oParentNS.insert(std::make_pair(iTypeName, ti));
      }
    }
    else if (iName.find("__") != 0)
    {
      auto namePos = signature.find(String(iName) + "(");
      if (namePos != std::string::npos)
      {
        if (iArrayType != nullptr && iName == "Elements")
        {
          lac::an::TypeInfo ti;
          ti.type = lac::an::Type::function;
          ti.function.isMethod = true;
          lac::an::TypeInfo result;
          TypeName const& elementType = iArrayType->GetElementType()->GetName();
          ti.function.results.resize(1);
          ti.function.results[0].type = lac::an::Type::array;
          ti.function.results[0].name = elementType;

          oMap.insert(std::make_pair(iName, ti));
        }
        else
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

            if (signature.substr(parentPos, firstParamEnd - parentPos).find(iFullTypeName) == 0)
            {
              isMethod = true;
            }
            else if (signature.substr(parentPos, firstParamEnd - parentPos).find("luabind::argument") == 0) {
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
            signature.replace(namePos, iName.size(), "method");
          }
          else
          {
            signature.replace(namePos, iName.size(), "function");
          }
          CleanupCPPDecorators(signature);
          NameArgs(signature);
          lac::an::TypeInfo ti(signature);
          if (ti.type == lac::an::Type::error)
          {
            LOG_ERROR << "Could not parse signature for " << iName << " : " << signature;
          }
          else
          {
            oMap.insert(std::make_pair(iName, ti));
          }
          //LOG_INFO << memberName << "->" << signature;
        }
      }
    }
  }

  void SetupGameDataTable(
    World& iWorld,
    std::map<std::string, lac::an::TypeInfo>& oMap) 
  {
    std::map<std::string, lac::an::TypeInfo> gameDataTable;
    
    String objHandleType = "eXl__ObjectHandle";
    auto const& propNames = iWorld.GetConfig().m_Properties.GetProperties();
    for (auto const& name : propNames) 
    {
      if (Type const* propType = iWorld.GetConfig().m_Properties.GetTypeFromName(name) ) 
      {
        String luaTypeName = LuaTypeName( propType );
        auto& propSlot = gameDataTable[name.c_str()];
        propSlot.type = lac::an::Type::table;
        propSlot.name = name.c_str();
        propSlot.members["Get"] = lac::an::TypeInfo( luaTypeName + " function(" + objHandleType + " object)" );
        propSlot.members["Set"] = lac::an::TypeInfo("function("+ objHandleType + " object, " + luaTypeName + " property)");
        propSlot.members["Modify"] = lac::an::TypeInfo(luaTypeName + " function(" + objHandleType + " object)");
        lac::an::TypeInfo iterRes(objHandleType + ", " + luaTypeName + " function()");
        lac::an::TypeInfo& iterType = propSlot.members["Iter"];
        iterType.name = "Iter";
        iterType.type = lac::an::Type::function;
        iterType.function.isMethod = false;
        iterType.function.results.push_back(iterRes);

        propSlot.members["IterModify"] = iterType;
        propSlot.members["IterModify"].name = "IterModify";
      }
    }
    oMap["GameData"].members = std::move(gameDataTable);
    oMap["GameData"].type = lac::an::Type::table;
  }

  void ExploreLuabindTable(lua_State* luaState,
    lac::an::UserDefined& oTypes,
    const luabind::object& iTable,
    KString iName,
    std::map<std::string, lac::an::TypeInfo>& oMap) {

    for (luabind::iterator iterEntry(iTable); iterEntry != luabind::iterator(); ++iterEntry)
    {
      luabind::object curModuleEntry(*iterEntry);
      curModuleEntry.push(luaState);
      luabind::detail::stack_pop pop(luaState, 1);
      if (luabind::detail::is_class_rep(luaState, -1))
      {
        luabind::detail::class_rep* classInfo = static_cast<luabind::detail::class_rep*>(lua_touserdata(luaState, -1));
        luabind::type_id type = classInfo->type();
        Type const* exlType = type.get_id();

        TupleType const* tuple = exlType->IsTuple();
        ArrayType const* arrayType = ArrayType::DynamicCast(exlType);

        // Only get unscoped type name
        String typeName = exlType->GetDisplayName(0xFFFF);
        

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
                  TypeNameToTypeInfo(nT, newType.members[memberName]);
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

                TypeNameToTypeInfo(type, newType.members[memberName]);
              }
            }
          }
          else
          {
            if (!luabind::detail::is_luabind_function(luaState, -1))
            {
              continue;
            }
            luabind::object funObj(luabind::from_stack(luaState, -1));
            RegisterLuabindFunction(luaState, arrayType, funObj, memberName, newType.name, typeName, oMap, newType.members);

          }
        }
        oTypes.addType( newType );
      }
      else
      {
        auto key = iterEntry.key();
        auto memberName = luabind::to_string(key);

        if (luabind::detail::is_luabind_function(luaState, -1))
        {
          luabind::object member(*iterEntry);
          member.push(luaState);
          luabind::detail::stack_pop pop(luaState, 1);
          luabind::object funObj(luabind::from_stack(luaState, -1));
          std::map< std::string, lac::an::TypeInfo > dummyMap;
          RegisterLuabindFunction(luaState, nullptr, funObj, memberName, "", "", dummyMap, oMap);
        }
        else
        {
          luabind::object member(*iterEntry);
          member.push(luaState);
          luabind::detail::stack_pop pop(luaState, 1);
          if (lua_istable(luaState, -1)) {
            lac::an::TypeInfo table;
            table.name = memberName;
            table.type = lac::an::Type::table;
            if (memberName != "GameData") 
            {
              ExploreLuabindTable(luaState, oTypes, member, memberName, table.members);
              oMap[memberName] = std::move(table);
            }
          }
          else if (lua_iscfunction(luaState, -1)) 
          {
            lac::an::TypeInfo info("function()");
            info.name = memberName;
            oMap[memberName] = std::move(info);
          }
        }
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
    eXlModule.name = "eXlModule";
    eXlModule.type = lac::an::Type::table;

    LuaWorld& luaWorld = luaScripts->GetLuaWorld();
    {
      LuaStateHandle stateHandle = luaWorld.GetState();
      lua_State* luaState = stateHandle.GetState();

      auto* classReg = luabind::detail::class_registry::get_registry(luaState);
      luabind::object _G = luabind::globals(luaState);
      luabind::object exlModuleTable = _G["eXl"];
      ExploreLuabindTable( luaState, ud, exlModuleTable, "eXl", eXlModule.members );

      luabind::object exlProjectTable = _G["eXlProject"];
      ExploreLuabindTable(luaState, ud, exlProjectTable, "eXlProject", eXlModule.members);

      SetupGameDataTable(world, eXlModule.members);
    }
    ud.addType(std::move(eXlModule));
    ud.addVariable("eXl", "eXlModule");
    return ud;
  }

  lac::an::UserDefined BuildExlInformationForHandler(Vector<ResourceHandle<LuaFunctionLibrary>> const& iDeps, EventsManifest::FunctionsMap const& iFunctions)
  {
    lac::an::UserDefined baseInfo = BuildExlInformation(iDeps);

    for (auto const& entryPoint : iFunctions)
    {
      String funcArgs("function(table self, eXl__ObjectHandle obj,");

      for (uint32_t i = 0; i < entryPoint.second.GetArgs().size(); ++i)
      {
        Type const* type = entryPoint.second.GetArgs()[i];
        funcArgs += ' ';
        funcArgs += LuaTypeName(type);
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