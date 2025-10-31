// esp32_lua_esp32.cpp
#include "lua/lua.hpp" // допускается, но ниже подключаю C API заголовки
extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#include <SPI.h>
#include <SPIFFS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// ---------- Глобальные переменные (один экземпляр в прошивке) ----------
static Adafruit_ILI9341* g_tft = nullptr;
static XPT2046_Touchscreen* g_ts = nullptr;
static lua_State* L = nullptr;

// ---------- Forward declarations ----------
void setap_lua(Adafruit_ILI9341 &tft, XPT2046_Touchscreen &ts);
void run_lua(String code);

// ---------- Утилиты ----------
static void ensure_lua_initialized();

// ---------- Lua-C API функции (extern "C" not required but safe) ----------
static int l_draw_text(lua_State* L) {
    // esp.draw_text(x, y, text, size)
    int nargs = lua_gettop(L);
    if (nargs < 3) {
        lua_pushstring(L, "draw_text expects (x,y,text[,size])");
        lua_error(L);
        return 0;
    }
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    const char* txt = luaL_checkstring(L, 3);
    int size = 1;
    if (nargs >= 4) size = (int)luaL_checkinteger(L, 4);

    if (!g_tft) return 0;
    g_tft->setTextSize(size);
    g_tft->setCursor(x, y);
    g_tft->setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    g_tft->print(txt);
    return 0;
}

static int l_draw_button(lua_State* L) {
    // esp.draw_button(x,y,w,h,label)
    int nargs = lua_gettop(L);
    if (nargs < 5) {
        lua_pushstring(L, "draw_button expects (x,y,w,h,label)");
        lua_error(L);
        return 0;
    }
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    const char* label = luaL_checkstring(L, 5);

    if (!g_tft) return 0;
    // background
    g_tft->fillRoundRect(x, y, w, h, 6, ILI9341_DARKGREY);
    // border
    g_tft->drawRoundRect(x, y, w, h, 6, ILI9341_WHITE);
    // label - center
    int16_t tbx = x + 4;
    int16_t tby = y + (h/2) - 6;
    g_tft->setTextSize(2);
    g_tft->setTextWrap(false);
    g_tft->setCursor(tbx, tby);
    g_tft->setTextColor(ILI9341_WHITE, ILI9341_DARKGREY);
    g_tft->print(label);
    return 0;
}

static int l_pin_mode(lua_State* L) {
    // esp.pin_mode(pin, mode) -- mode: "INPUT", "OUTPUT", "INPUT_PULLUP"
    int pin = (int)luaL_checkinteger(L, 1);
    const char* mode = luaL_checkstring(L, 2);
    if (strcmp(mode, "OUTPUT") == 0) pinMode(pin, OUTPUT);
    else if (strcmp(mode, "INPUT") == 0) pinMode(pin, INPUT);
    else if (strcmp(mode, "INPUT_PULLUP") == 0) pinMode(pin, INPUT_PULLUP);
    else {
        lua_pushstring(L, "invalid pin mode");
        lua_error(L);
    }
    return 0;
}

static int l_digital_write(lua_State* L) {
    // esp.digital_write(pin, value)
    int pin = (int)luaL_checkinteger(L, 1);
    int val = lua_toboolean(L, 2) ? HIGH : LOW;
    digitalWrite(pin, val);
    return 0;
}

static int l_digital_read(lua_State* L) {
    // v = esp.digital_read(pin)
    int pin = (int)luaL_checkinteger(L, 1);
    int v = digitalRead(pin);
    lua_pushinteger(L, v);
    return 1;
}

static int l_analog_read(lua_State* L) {
    // v = esp.analog_read(pin)
    int pin = (int)luaL_checkinteger(L, 1);
    int v = analogRead(pin);
    lua_pushinteger(L, v);
    return 1;
}

static int l_touch_read(lua_State* L) {
    // x,y,z,pressed = esp.touch_read()
    if (!g_ts) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushboolean(L, 0);
        return 4;
    }
    TS_Point p = g_ts->getPoint(); // обычно TS_Point { x, y, z }
    // Некоторые реализации требуют mapRawToScreen; тут простая выдача значений
    int x = p.x;
    int y = p.y;
    int z = p.z;
    bool pressed = (p.z > 0);
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    lua_pushinteger(L, z);
    lua_pushboolean(L, pressed);
    return 4;
}

static int l_draw_image_raw(lua_State* L) {
    // esp.draw_image_raw(path, x, y, w, h)
    // Ожидается RAW RGB565 (uint16_t LE) с длиной w*h*2
    const char* path = luaL_checkstring(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int w = (int)luaL_checkinteger(L, 4);
    int h = (int)luaL_checkinteger(L, 5);

    if (!g_tft) return 0;

    if (!SPIFFS.begin(true)) {
        lua_pushstring(L, "SPIFFS not mounted");
        lua_error(L);
        return 0;
    }
    File f = SPIFFS.open(path, "r");
    if (!f) {
        lua_pushfstring(L, "file not found: %s", path);
        lua_error(L);
        return 0;
    }
    size_t expected = (size_t)w * (size_t)h * 2;
    if (f.size() < expected) {
        // попробуем читать столько, сколько есть
    }

    // Буфер небольшими кусками
    const size_t CHUNK_PIXELS = 128; // 128 pixels per chunk => 256 bytes
    uint8_t buf[CHUNK_PIXELS * 2];
    int px = 0;
    for (int row = 0; row < h; ++row) {
        int col = 0;
        while (col < w) {
            int toread = min((int)CHUNK_PIXELS, w - col);
            size_t bytes = toread * 2;
            size_t r = f.read(buf, bytes);
            if (r == 0) break;
            // draw these pixels
            // Adafruit_ILI9341 supports drawRGBBitmap but expects 16-bit array in MCUs endianness
            // We'll write pixel by pixel
            for (int i = 0; i < (int)r; i += 2) {
                uint16_t pix = buf[i] | (buf[i+1] << 8);
                int drawx = x + col;
                int drawy = y + row;
                g_tft->drawPixel(drawx, drawy, pix);
                col++;
            }
        }
    }
    f.close();
    return 0;
}

// ---------- Регистрация библиотеки в Lua ----------
static void register_esp_api(lua_State* L) {
    static const luaL_Reg esp_funcs[] = {
        {"draw_text", l_draw_text},
        {"draw_button", l_draw_button},
        {"draw_image_raw", l_draw_image_raw},
        {"touch_read", l_touch_read},
        {"pin_mode", l_pin_mode},
        {"digital_write", l_digital_write},
        {"digital_read", l_digital_read},
        {"analog_read", l_analog_read},
        {NULL, NULL}
    };

    lua_newtable(L);               // create table esp
    luaL_setfuncs(L, esp_funcs, 0);
    lua_setglobal(L, "esp");       // global.esp = { ... }
}

// ---------- Инициализация Lua (один раз) ----------
static void ensure_lua_initialized() {
    if (L) return;
    L = luaL_newstate();
    luaL_openlibs(L);
    register_esp_api(L);
}

// ---------- Пользовательские внешние функции ----------
void setap_lua(Adafruit_ILI9341 &tft, XPT2046_Touchscreen &ts){
    // Сохраняем указатели на дисплей и тач
    g_tft = &tft;
    g_ts = &ts;

    // Инициализация SPIFFS (если не инициализирован)
    if (!SPIFFS.begin(true)) {
        // Если не удалось - можно попытаться позже
    }

    // Инициализируем Lua-стейт (глобально)
    ensure_lua_initialized();
}

void run_lua(String code){
    ensure_lua_initialized();
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
