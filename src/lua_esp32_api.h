#include "lua/lua.hpp"
extern "C" {
#include "lua/lualib.h"
#include "lua/lauxlib.h"
}

#include <Arduino.h>

int lua_serial_begin(lua_State* L) {
  // self = userdata (Serial)
  // 2-й аргумент — скорость
  int baud = luaL_checkinteger(L, 2);
  Serial.begin(baud);
  return 0;
}

int lua_serial_print(lua_State* L) {
  const char* str = luaL_checkstring(L, 2);
  Serial.print(str);
  return 0;
}

int lua_serial_println(lua_State* L) {
  const char* str = luaL_checkstring(L, 2);
  Serial.println(str);
  return 0;
}

int lua_serial_write(lua_State* L) {
  const char* str = luaL_checkstring(L, 2);
  Serial.write(str);
  return 0;
}

int lua_serial_read(lua_State* L) {
  if (Serial.available()) {
    int ch = Serial.read();
    lua_pushinteger(L, ch);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int lua_serial_available(lua_State* L) {
  lua_pushinteger(L, Serial.available());
  return 1;
}


int lua_serial_flush(lua_State* L) {
  Serial.flush();
  return 0;
}

// ------------------------------
//      GPIO BINDINGS
// ------------------------------

int lua_pinMode(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  int mode = luaL_checkinteger(L, 2);
  pinMode(pin, mode);
  return 0;
}

int lua_digitalWrite(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  int val = luaL_checkinteger(L, 2);
  digitalWrite(pin, val);
  return 0;
}

int lua_digitalRead(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  lua_pushinteger(L, digitalRead(pin));
  return 1;
}

int lua_analogRead(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  lua_pushinteger(L, analogRead(pin));
  return 1;
}

int lua_analogWrite(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  int val = luaL_checkinteger(L, 2);
  analogWrite(pin, val);
  return 0;
}

int lua_delay(lua_State* L) {
  int ms = luaL_checkinteger(L, 1);
  delay(ms);
  return 0;
}

int lua_millis(lua_State* L) {
  lua_pushinteger(L, millis());
  return 1;
}

// ------------------------------
//      REGISTRATION
// ------------------------------
void register_serial(lua_State* L) {
  // Создаём таблицу методов
  luaL_newmetatable(L, "SerialMeta");

  lua_pushcfunction(L, lua_serial_begin);
  lua_setfield(L, -2, "begin");

  lua_pushcfunction(L, lua_serial_print);
  lua_setfield(L, -2, "print");

  lua_pushcfunction(L, lua_serial_println);
  lua_setfield(L, -2, "println");

  lua_pushcfunction(L, lua_serial_write);
  lua_setfield(L, -2, "write");

  lua_pushcfunction(L, lua_serial_read);
  lua_setfield(L, -2, "read");

  lua_pushcfunction(L, lua_serial_available);
  lua_setfield(L, -2, "available");

  lua_pushcfunction(L, lua_serial_flush);
  lua_setfield(L, -2, "flush");

  // Устанавливаем метатаблицу для объекта
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");

  // Создаём глобальный объект Serial
  lua_newuserdata(L, 0);
  luaL_setmetatable(L, "SerialMeta");
  lua_setglobal(L, "Serial");
}

void register_gpio(lua_State* L) {
  lua_register(L, "pinMode", lua_pinMode);
  lua_register(L, "digitalWrite", lua_digitalWrite);
  lua_register(L, "digitalRead", lua_digitalRead);
  lua_register(L, "analogRead", lua_analogRead);
  lua_register(L, "analogWrite", lua_analogWrite);
  lua_register(L, "delay", lua_delay);
  lua_register(L, "millis", lua_millis);

  // Добавим удобные константы
  lua_pushinteger(L, OUTPUT);
  lua_setglobal(L, "OUTPUT");

  lua_pushinteger(L, INPUT);
  lua_setglobal(L, "INPUT");

  lua_pushinteger(L, INPUT_PULLUP);
  lua_setglobal(L, "INPUT_PULLUP");

  lua_pushinteger(L, HIGH);
  lua_setglobal(L, "HIGH");

  lua_pushinteger(L, LOW);
  lua_setglobal(L, "LOW");
}