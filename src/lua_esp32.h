// esp32_lua_esp32.cpp
#include "lua/lua.hpp" // допускается, но ниже подключаю C API заголовки
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}
#include <SPI.h>
#include "lua_esp32_api.h"
// ---------- Глобальные переменные (один экземпляр в прошивке) ----------
static lua_State* L = nullptr;

// ---------- Forward declarations ----------
void run_lua(String code);
// ---------- Инициализация Lua (один раз) ----------
void register_esp_lua_api(lua_State* L) {
  register_serial(L);
  register_gpio(L);
}
// ---------- Пользовательские внешние функции ----------
void setap_lua(){
    register_esp_lua_api(L);
    // Инициализируем Lua-стейт (глобально)S
}

void run_lua(String code){
    if (!L) return;
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
