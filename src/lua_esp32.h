// esp32_lua_esp32.cpp
#include "lua/lua.hpp"
extern "C" {
  #include "lua/lua.h"
  #include "lua/lauxlib.h"
  #include "lua/lualib.h"
}
#include "lua_esp32_api.h"

// ---------- Глобальные переменные ----------
static lua_State* L = nullptr;

// ---------- Forward declarations ----------
void register_esp_lua_api(lua_State* L);

// ---------- Регистрация API ----------
void register_esp_lua_api(lua_State* L) {
  register_a_lu_esp32(L);
}
void add_lua_command(const String& name, lua_CFunction func) {
    lua_register(L, name.c_str(), func);
}
void add_lua_command_to_class(const String& name,const String& class_name, lua_CFunction func) {
    lua_register(L, name.c_str(), func);
    lua_setglobal(L, class_name.c_str());
}
// ---------- Инициализация Lua ----------
void setap_lua() {
  if (L) {
    lua_close(L);  // если Lua уже была запущена
    L = nullptr;
  }

  L = luaL_newstate();   // создаём Lua VM
  if (!L) {
    Serial.println("Failed to create Lua state!");
    return;
  }
  luaL_openlibs(L);

  // Регистрируем Arduino API
  register_esp_lua_api(L);

  // Отладочный вывод
  Serial.begin(115200);
  Serial.println("Lua initialized successfully!");
}

// ---------- Запуск кода ----------
void run_lua(String code) {
  if (!L) {
    Serial.println("Lua not initialized!");
    return;
  }

  int status = luaL_loadbuffer(L, code.c_str(), (size_t)code.length(), "user_code");
  if (status != LUA_OK) {
    const char* err = lua_tostring(L, -1);
    Serial.print("Lua load error: ");
    Serial.println(err ? err : "unknown");
    lua_pop(L, 1);
    return;
  }

  status = lua_pcall(L, 0, LUA_MULTRET, 0);
  if (status != LUA_OK) {
    const char* err = lua_tostring(L, -1);
    Serial.print("Lua runtime error: ");
    Serial.println(err ? err : "unknown");
    lua_pop(L, 1);
    return;
  }
}
