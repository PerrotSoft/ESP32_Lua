#pragma once
#include <Arduino.h>
#include "lua/lua.hpp"
extern "C" {
  #include "lua/lualib.h"
  #include "lua/lauxlib.h"
}
#ifdef ARDUINO_ARCH_ESP32
#include <driver/ledc.h>
#endif
#include <SPI.h>
#include <SD.h>
#include <SPIFFS.h>

// ---------------------------
//  Internal helpers / state
// ---------------------------

// SPI CS support
static int spi_cs_pin = -1;
static bool spi_has_cs = false;
static SPIClass *spi_inst = &SPI; // use default VSPI/HSPI depending on init

// analogWrite via LEDC on ESP32
#define MAX_LEDC_CHANNELS 16
static int ledc_channel_of_pin[40]; // пины -> каналы
static bool ledc_channel_inited = false;
static int next_ledc_channel = 0;
static const int DEFAULT_LEDC_FREQ = 5000;
static const int DEFAULT_LEDC_RESOLUTION = 8; // 8 бит => duty 0..255

static void ensure_ledc_initialized() {
  if (!ledc_channel_inited) {
    for (int i=0;i<40;i++) ledc_channel_of_pin[i] = -1;
    next_ledc_channel = 0;
    ledc_channel_inited = true;
  }
}

#ifdef ARDUINO_ARCH_ESP32
int allocate_ledc_channel_for_pin(int pin) {
    ensure_ledc_initialized();
    if (pin < 0 || pin >= 40) return -1;
    if (ledc_channel_of_pin[pin] != -1) return ledc_channel_of_pin[pin];
    if (next_ledc_channel >= MAX_LEDC_CHANNELS) return -1;

    int ch = next_ledc_channel++;

    // Явная настройка канала
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)DEFAULT_LEDC_RESOLUTION,
        .timer_num = (ledc_timer_t)ch,
        .freq_hz = DEFAULT_LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {
        .gpio_num = pin,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = (ledc_channel_t)ch,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = (ledc_timer_t)ch,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ch_conf);

    ledc_channel_of_pin[pin] = ch;
    return ch;
}
#endif

// ---------------------------
//  Serial (class-like) -> Serial table in Lua
// ---------------------------

int lua_Serial_begin(lua_State* L) {
  int baud = luaL_checkinteger(L, 1);
  Serial.begin(baud);
  return 0;
}

int lua_Serial_print(lua_State* L) {
  const char* s = luaL_checkstring(L, 1);
  Serial.print(s);
  return 0;
}

int lua_Serial_println(lua_State* L) {
  const char* s = luaL_checkstring(L, 1);
  Serial.println(s);
  return 0;
}

int lua_Serial_write(lua_State* L) {
  size_t len;
  const char* s = luaL_checklstring(L, 1, &len);
  Serial.write((const uint8_t*)s, len);
  return 0;
}

int lua_Serial_available(lua_State* L) {
  lua_pushinteger(L, Serial.available());
  return 1;
}

int lua_Serial_read(lua_State* L) {
  if (Serial.available()) lua_pushinteger(L, Serial.read());
  else lua_pushnil(L);
  return 1;
}

int lua_Serial_flush(lua_State* L) {
  Serial.flush();
  return 0;
}

void register_Serial(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, lua_Serial_begin); lua_setfield(L, -2, "begin");
  lua_pushcfunction(L, lua_Serial_print); lua_setfield(L, -2, "print");
  lua_pushcfunction(L, lua_Serial_println); lua_setfield(L, -2, "println");
  lua_pushcfunction(L, lua_Serial_write); lua_setfield(L, -2, "write");
  lua_pushcfunction(L, lua_Serial_available); lua_setfield(L, -2, "available");
  lua_pushcfunction(L, lua_Serial_read); lua_setfield(L, -2, "read");
  lua_pushcfunction(L, lua_Serial_flush); lua_setfield(L, -2, "flush");
  lua_setglobal(L, "Serial");
}

// ---------------------------
//  Global Arduino-style functions
//  (pinMode, digitalWrite, digitalRead, analogRead, delay, millis, micros...)
// ---------------------------

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
  int v = digitalRead(pin);
  lua_pushinteger(L, v);
  return 1;
}

int lua_analogRead(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  int v = analogRead(pin);
  lua_pushinteger(L, v);
  return 1;
}

int lua_analogWrite(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  int val = luaL_checkinteger(L, 2);
  int ch = allocate_ledc_channel_for_pin(pin);
  if (ch < 0) {
    lua_pushnil(L);
    lua_pushstring(L, "No LEDC channels left");
    return 2;
  }
  int maxDuty = (1 << DEFAULT_LEDC_RESOLUTION) - 1;
  int duty = constrain(val, 0, maxDuty);
  ledcWrite(ch, duty);
  return 0;
}

int lua_delay(lua_State* L) {
  int ms = luaL_checkinteger(L, 1);
  delay(ms);
  return 0;
}

int lua_delayMicroseconds(lua_State* L) {
  int us = luaL_checkinteger(L, 1);
  delayMicroseconds(us);
  return 0;
}

int lua_millis(lua_State* L) {
  lua_pushinteger(L, millis());
  return 1;
}

int lua_micros(lua_State* L) {
  lua_pushinteger(L, micros());
  return 1;
}

// ---------------------------
//  Utility functions: map, constrain, abs, min, max, random
// ---------------------------

int lua_map(lua_State* L) {
  long x = luaL_checkinteger(L, 1);
  long in_min = luaL_checkinteger(L, 2);
  long in_max = luaL_checkinteger(L, 3);
  long out_min = luaL_checkinteger(L, 4);
  long out_max = luaL_checkinteger(L, 5);
  long r = map(x, in_min, in_max, out_min, out_max);
  lua_pushinteger(L, r);
  return 1;
}

int lua_constrain(lua_State* L) {
  long x = luaL_checkinteger(L, 1);
  long a = luaL_checkinteger(L, 2);
  long b = luaL_checkinteger(L, 3);
  long r = constrain(x, a, b);
  lua_pushinteger(L, r);
  return 1;
}

int lua_abs(lua_State* L) {
  long x = luaL_checkinteger(L, 1);
  lua_pushinteger(L, abs(x));
  return 1;
}

int lua_min(lua_State* L) {
  long a = luaL_checkinteger(L, 1);
  long b = luaL_checkinteger(L, 2);
  lua_pushinteger(L, min(a,b));
  return 1;
}

int lua_max(lua_State* L) {
  long a = luaL_checkinteger(L, 1);
  long b = luaL_checkinteger(L, 2);
  lua_pushinteger(L, max(a,b));
  return 1;
}

int lua_randomSeed(lua_State* L) {
  long seed = luaL_checkinteger(L, 1);
  randomSeed((uint32_t)seed);
  return 0;
}

int lua_random(lua_State* L) {
  int nargs = lua_gettop(L);
  if (nargs == 0) {
    lua_pushinteger(L, random());
  } else if (nargs == 1) {
    int maxv = luaL_checkinteger(L, 1);
    lua_pushinteger(L, random(maxv));
  } else {
    int minv = luaL_checkinteger(L, 1);
    int maxv = luaL_checkinteger(L, 2);
    lua_pushinteger(L, random(minv, maxv));
  }
  return 1;
}

// ---------------------------
//  Bit helpers
// ---------------------------

int lua_bitRead(lua_State* L) {
  unsigned long value = luaL_checkinteger(L, 1);
  int bit = luaL_checkinteger(L, 2);
  lua_pushinteger(L, (value >> bit) & 1);
  return 1;
}
int lua_bitWrite(lua_State* L) {
  unsigned long value = luaL_checkinteger(L, 1);
  int bit = luaL_checkinteger(L, 2);
  int bitvalue = luaL_checkinteger(L, 3);
  if (bitvalue) value |= (1UL << bit);
  else value &= ~(1UL << bit);
  lua_pushinteger(L, value);
  return 1;
}
int lua_bitSet(lua_State* L) {
  unsigned long value = luaL_checkinteger(L, 1);
  int bit = luaL_checkinteger(L, 2);
  value |= (1UL << bit);
  lua_pushinteger(L, value);
  return 1;
}
int lua_bitClear(lua_State* L) {
  unsigned long value = luaL_checkinteger(L, 1);
  int bit = luaL_checkinteger(L, 2);
  value &= ~(1UL << bit);
  lua_pushinteger(L, value);
  return 1;
}

// ---------------------------
//  Tone / noTone (simple blocking impl)
// ---------------------------

int lua_tone(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  int freq = luaL_checkinteger(L, 2);
  int duration = 0;
  if (lua_gettop(L) >= 3) duration = luaL_checkinteger(L, 3);

  int ch = allocate_ledc_channel_for_pin(pin);
  if (ch < 0) {
    lua_pushboolean(L, false);
    return 1;
  }
  // ESP32: ledcWriteTone sets frequency on channel
  ledcWriteTone(ch, freq);
  if (duration > 0) {
    delay(duration);
    ledcWriteTone(ch, 0); // stop
  }
  lua_pushboolean(L, true);
  return 1;
}

int lua_noTone(lua_State* L) {
  int pin = luaL_checkinteger(L, 1);
  if (pin >=0 && pin < (int)sizeof(ledc_channel_of_pin)/sizeof(ledc_channel_of_pin[0])) {
    int ch = ledc_channel_of_pin[pin];
    if (ch >= 0) ledcWriteTone(ch, 0);
  }
  return 0;
}

// ---------------------------
//  SPI (class-like object but with CS support)
//    SPI.begin(sclk, miso, mosi, cs)
//    SPI.transfer(byte) -> uses CS if set
// ---------------------------

int lua_SPI_begin(lua_State* L) {
  int sclk = luaL_checkinteger(L, 1);
  int miso = luaL_checkinteger(L, 2);
  int mosi = luaL_checkinteger(L, 3);
  // optional CS as 4th arg
  if (lua_gettop(L) >= 4) {
    spi_cs_pin = luaL_checkinteger(L, 4);
    spi_has_cs = true;
    pinMode(spi_cs_pin, OUTPUT);
    digitalWrite(spi_cs_pin, HIGH);
  } else {
    spi_has_cs = false;
    spi_cs_pin = -1;
  }
  // begin SPI; on ESP32 SPI.begin(SCLK, MISO, MOSI, SS)
  SPI.begin(sclk, miso, mosi, spi_cs_pin >=0 ? spi_cs_pin : -1);
  return 0;
}

int lua_SPI_transfer(lua_State* L) {
  int b = luaL_checkinteger(L, 1);
  uint8_t res;
  if (spi_has_cs && spi_cs_pin >= 0) digitalWrite(spi_cs_pin, LOW);
  res = SPI.transfer((uint8_t)b);
  if (spi_has_cs && spi_cs_pin >= 0) digitalWrite(spi_cs_pin, HIGH);
  lua_pushinteger(L, res);
  return 1;
}

int lua_SPI_transferBytes(lua_State* L) {
  // transferBytes(tx_string_or_nil, len) or (tx_table, len)
  size_t txlen;
  const char* tx = nullptr;
  if (lua_isstring(L,1)) {
    tx = luaL_checklstring(L, 1, &txlen);
  } else if (lua_islightuserdata(L,1)) {
    tx = (const char*)lua_touserdata(L,1);
    txlen = luaL_checkinteger(L, 2);
  } else {
    lua_pushnil(L);
    lua_pushstring(L, "transferBytes: unsupported arg");
    return 2;
  }
  // allocate rx buffer
  uint8_t* rx = (uint8_t*)malloc(txlen);
  if (!rx) {
    lua_pushnil(L);
    lua_pushstring(L, "transferBytes: oom");
    return 2;
  }
  if (spi_has_cs && spi_cs_pin >= 0) digitalWrite(spi_cs_pin, LOW);
  SPI.transferBytes((uint8_t*)tx, rx, txlen);
  if (spi_has_cs && spi_cs_pin >= 0) digitalWrite(spi_cs_pin, HIGH);

  lua_pushlstring(L, (const char*)rx, txlen);
  free(rx);
  return 1;
}

void register_SPI(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, lua_SPI_begin); lua_setfield(L, -2, "begin"); // SPI.begin(sclk, miso, mosi, cs?)
  lua_pushcfunction(L, lua_SPI_transfer); lua_setfield(L, -2, "transfer");
  lua_pushcfunction(L, lua_SPI_transferBytes); lua_setfield(L, -2, "transferBytes");
  lua_setglobal(L, "SPI");
}

// ---------------------------
//  SD class wrapper (SD.begin, SD.open, SD.read, SD.write, SD.close)
// ---------------------------

int lua_SD_begin(lua_State* L) {
  int cs = luaL_checkinteger(L, 1);
  bool ok = SD.begin(cs);
  lua_pushboolean(L, ok);
  return 1;
}

int lua_SD_open(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* mode = luaL_optstring(L, 2, "r");
  File f = SD.open(path, strcmp(mode, "r")==0 ? FILE_READ : FILE_WRITE);
  if (!f) {
    lua_pushnil(L);
    return 1;
  }
  File* pf = new File(f);
  lua_pushlightuserdata(L, pf);
  return 1;
}

int lua_SD_read(lua_State* L) {
  File* pf = (File*)lua_touserdata(L, 1);
  size_t n = luaL_checkinteger(L, 2);
  String s;
  if (pf && pf->available()) {
    char* buf = (char*)malloc(n+1);
    size_t r = pf->readBytes(buf, n);
    buf[r] = 0;
    lua_pushlstring(L, buf, r);
    free(buf);
    return 1;
  }
  lua_pushnil(L);
  return 1;
}

int lua_SD_write(lua_State* L) {
  File* pf = (File*)lua_touserdata(L, 1);
  size_t len;
  const char* data = luaL_checklstring(L, 2, &len);
  if (!pf) {
    lua_pushinteger(L, 0);
    return 1;
  }
  size_t w = pf->write((const uint8_t*)data, len);
  lua_pushinteger(L, w);
  return 1;
}

int lua_SD_close(lua_State* L) {
  File* pf = (File*)lua_touserdata(L, 1);
  if (pf) {
    pf->close();
    delete pf;
  }
  return 0;
}

void register_SD(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, lua_SD_begin); lua_setfield(L, -2, "begin");
  lua_pushcfunction(L, lua_SD_open); lua_setfield(L, -2, "open");
  lua_pushcfunction(L, lua_SD_read); lua_setfield(L, -2, "read");
  lua_pushcfunction(L, lua_SD_write); lua_setfield(L, -2, "write");
  lua_pushcfunction(L, lua_SD_close); lua_setfield(L, -2, "close");
  lua_setglobal(L, "SD");
}

// ---------------------------
//  SPIFFS / FS wrapper
// ---------------------------

int lua_FS_begin(lua_State* L) {
  bool ok = SPIFFS.begin(true);
  lua_pushboolean(L, ok);
  return 1;
}

int lua_FS_open(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  const char* mode = luaL_optstring(L, 2, "r");
  File f = SPIFFS.open(path, strcmp(mode, "r")==0 ? FILE_READ : FILE_WRITE);
  if (!f) { lua_pushnil(L); return 1; }
  File* pf = new File(f);
  lua_pushlightuserdata(L, pf);
  return 1;
}

int lua_FS_read(lua_State* L) {
  File* pf = (File*)lua_touserdata(L, 1);
  size_t n = luaL_checkinteger(L, 2);
  if (pf && pf->available()) {
    char* buf = (char*)malloc(n+1);
    size_t r = pf->readBytes(buf, n);
    buf[r] = 0;
    lua_pushlstring(L, buf, r);
    free(buf);
    return 1;
  }
  lua_pushnil(L);
  return 1;
}

int lua_FS_write(lua_State* L) {
  File* pf = (File*)lua_touserdata(L, 1);
  size_t len;
  const char* data = luaL_checklstring(L, 2, &len);
  if (!pf) { lua_pushinteger(L, 0); return 1; }
  size_t w = pf->write((const uint8_t*)data, len);
  lua_pushinteger(L, w);
  return 1;
}

int lua_FS_close(lua_State* L) {
  File* pf = (File*)lua_touserdata(L, 1);
  if (pf) { pf->close(); delete pf; }
  return 0;
}

void register_FS(lua_State* L) {
  lua_newtable(L);
  lua_pushcfunction(L, lua_FS_begin); lua_setfield(L, -2, "mount");
  lua_pushcfunction(L, lua_FS_open); lua_setfield(L, -2, "open");
  lua_pushcfunction(L, lua_FS_read); lua_setfield(L, -2, "read");
  lua_pushcfunction(L, lua_FS_write); lua_setfield(L, -2, "write");
  lua_pushcfunction(L, lua_FS_close); lua_setfield(L, -2, "close");
  lua_setglobal(L, "FS");
}

// ---------------------------
//  Register everything
// ---------------------------

extern "C" void register_a_lu_esp32(lua_State* L) {
  register_Serial(L);

  // global Arduino functions
  lua_register(L, "pinMode", lua_pinMode);
  lua_register(L, "digitalWrite", lua_digitalWrite);
  lua_register(L, "digitalRead", lua_digitalRead);
  lua_register(L, "analogRead", lua_analogRead);
  lua_register(L, "analogWrite", lua_analogWrite);
  lua_register(L, "delay", lua_delay);
  lua_register(L, "delayMicroseconds", lua_delayMicroseconds);
  lua_register(L, "millis", lua_millis);
  lua_register(L, "micros", lua_micros);

  // utils
  lua_register(L, "map", lua_map);
  lua_register(L, "constrain", lua_constrain);
  lua_register(L, "abs", lua_abs);
  lua_register(L, "min", lua_min);
  lua_register(L, "max", lua_max);
  lua_register(L, "randomSeed", lua_randomSeed);
  lua_register(L, "random", lua_random);

  // bit helpers
  lua_register(L, "bitRead", lua_bitRead);
  lua_register(L, "bitWrite", lua_bitWrite);
  lua_register(L, "bitSet", lua_bitSet);
  lua_register(L, "bitClear", lua_bitClear);

  // tone
  lua_register(L, "tone", lua_tone);
  lua_register(L, "noTone", lua_noTone);

  // SPI class-like
  register_SPI(L);

  // SD class
  register_SD(L);

  // FS (SPIFFS)
  register_FS(L);
}
