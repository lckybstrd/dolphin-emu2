#include "Core/Lua/LuaFunctions/LuaImportModule.h"

#include "Core/Scripting/LanguageDefinitions/NewLuaScriptContext.h"

#include <fmt/format.h>
#include <memory>
#include <unordered_map>
#include "Core/Lua/LuaHelperClasses/luaL_Reg_With_Version.h"
#include "Core/Lua/LuaVersionResolver.h"

namespace Lua::LuaImportModule
{
const char* class_name = "dolphin";

class ImportModuleClass
{
public:
  inline ImportModuleClass() {}
};

static std::unique_ptr<ImportModuleClass> import_module_class_pointer = nullptr;

ImportModuleClass* GetImportModuleClassInstance()
{
  if (import_module_class_pointer == nullptr)
    import_module_class_pointer = std::make_unique<ImportModuleClass>(ImportModuleClass());
  return import_module_class_pointer.get();
}

static std::string lua_version_from_global;
void InitLuaImportModule(lua_State* lua_state, const std::string& lua_api_version)
{
  lua_version_from_global = lua_api_version;
  ImportModuleClass** import_module_class_ptr_ptr =
      (ImportModuleClass**)lua_newuserdata(lua_state, sizeof(ImportModuleClass*));
  *import_module_class_ptr_ptr = GetImportModuleClassInstance();
  luaL_newmetatable(lua_state, "LuaImportMetaTable");
  lua_pushvalue(lua_state, -1);
  lua_setfield(lua_state, -2, "__index");

  std::array lua_import_module_functions_with_versions_attached = {
      luaL_Reg_With_Version({"importModule", "1.0", ImportModule}),
      luaL_Reg_With_Version({"import", "1.0", ImportAlt})};

  std::unordered_map<std::string, std::string> deprecated_functions_map;
  AddLatestFunctionsForVersion(lua_import_module_functions_with_versions_attached, lua_api_version,
                               deprecated_functions_map, lua_state);
  lua_setglobal(lua_state, class_name);
}

int ImportCommon(lua_State* lua_state, const char* func_name)
{
  if (lua_type(lua_state, 1) != LUA_TUSERDATA)
    luaL_error(lua_state, "Error: importModule must be called using the colon operator like this - "
                          "dolphin:importModule(\"BitAPI\")");
  std::string module_class = luaL_checkstring(lua_state, 2);
  std::string version_number = luaL_checkstring(lua_state, 3);
  Scripting::NewLuaScriptContext::ImportModule(lua_state, version_number, module_class);
  return 0;
}

int ImportModule(lua_State* lua_state)
{
  return ImportCommon(lua_state, "importModule");
}

int ImportAlt(lua_State* lua_state)
{
  return ImportCommon(lua_state, "import");
}

}  // namespace Lua::LuaImportModule
