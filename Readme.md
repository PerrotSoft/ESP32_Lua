# 🦜 **ESP32-Lua — живой мост между Lua и ESP32!**

> Версия: v0.9.0 | Автор: ParrotSoft
> Лозунг: *«Пусть твой ESP говорит на Lua!» 🐍⚡*

---

## 💬 Что такое Lua?

`Lua.h` и `esp32_lua_esp32.cpp` — это твоя “магическая связка”, превращающая ESP32 в мини-компьютер с интерпретатором Lua внутри!
📖 То есть ты можешь писать скрипты прямо в Lua — а они будут управлять GPIO, Serial, SD-картой, SPIFFS, и всем, что умеет Arduino!

---

## 🧩 Общая структура

| Файл                      | Что делает                                                            |
| ------------------------- | --------------------------------------------------------------------- |
| **`luax.h`**              | Реализует базовые функции Lua↔Arduino (GPIO, Serial, SD, FS, и т.п.)  |
| **`Lua.h`**            | Управляет интерпретатором Lua: создаёт, инициализирует, исполняет код |
| **`esp32_lua_esp32.cpp`** | Настраивает систему, регистрирует API и добавляет команды             |

---

## 🚀 Быстрый старт

```cpp
#include "lua/lua.hpp"
#include "Lua.h"

void setup() {
  Serial.begin(115200);
  setap_lua(); // Запускаем Lua! 🔥

  run_lua(R"(
    Serial.println("Привет, ESP32 на Lua!")
    pinMode(2, 1)
    while true do
      digitalWrite(2, 1)
      delay(500)
      digitalWrite(2, 0)
      delay(500)
    end
  )");
}

void loop() {}
```

👉 После загрузки прошивки ESP начнёт моргать встроенным светодиодом, а в Serial будет писаться Lua-лог!

---

## 🧠 Подробно о каждой функции (C++ API)

### 🧩 `setap_lua()`

> Создаёт и инициализирует виртуальную машину Lua на ESP32.

**Что делает шаг за шагом:**

1. Если Lua уже запущена — закрывает старую VM.
2. Создаёт новую VM (`luaL_newstate()`).
3. Подключает стандартные библиотеки Lua (`luaL_openlibs()`).
4. Регистрирует весь Arduino API (`register_esp_lua_api()`).
5. Пишет “Lua initialized successfully!” в Serial.

**Пример:**

```cpp
void setup() {
  Serial.begin(115200);
  setap_lua();
}
```

🧩 *Теперь ты можешь выполнять любые Lua-команды с помощью `run_lua()`!*

---

### 🧩 `register_esp_lua_api(lua_State* L)`

> Подключает к Lua всё, что связано с ESP32: GPIO, Serial, SPI, SD, FS и пр.

```cpp
void register_esp_lua_api(lua_State* L) {
  register_a_lu_esp32(L); // из luax.h — регистрирует всё Arduino API
  lua_register(L, "runlua", run_lua1); // добавляет Lua-функцию runlua()
}
```

📘 После этого в Lua появятся:

```lua
pinMode, digitalWrite, delay, Serial.print, SPI.begin, SD.open, FS.mount, ...
```

🎉 То есть ты можешь написать скрипт на Lua, который выглядит как Arduino-скетч!

---

### 🧩 `void run_lua(String code)`

> Выполняет Lua-код, переданный строкой.

```cpp
run_lua("Serial.println('Lua работает!')");
```

🔍 **Под капотом:**

* Загружает Lua-код в VM (`luaL_loadbuffer`).
* Проверяет ошибки синтаксиса.
* Выполняет (`lua_pcall`).
* В случае ошибки пишет её в Serial.

⚠️ Если Lua не инициализирована — пишет `Lua not initialized!`.

---

### 🧩 `void run_lua1(lua_State* L)`

> То же самое, но вызывается из самой Lua.

```lua
runlua("Serial.println('Привет из Lua внутри Lua!')")
```

🧠 То есть ты можешь запустить Lua-код прямо из Lua-кода.
Пример:

```lua
runlua("pinMode(2,1); digitalWrite(2,1); delay(500); digitalWrite(2,0)")
```

---

### 🧩 `void add_lua_command(const String& name, lua_CFunction func)`

> Добавляет новую глобальную команду в Lua прямо из C++!

Пример:

```cpp
add_lua_command("blink3", [](lua_State* L)->int {
  pinMode(2, 1);
  for (int i = 0; i < 3; i++) {
    digitalWrite(2, 1);
    delay(200);
    digitalWrite(2, 0);
    delay(200);
  }
  return 0;
});
```

Теперь в Lua можно написать:

```lua
blink3()
```

✨ И твой пин D2 мигнёт три раза! ✨

---

### 🧩 `void add_lua_command_to_class(...)`

> Добавляет функцию как метод “класса” (например, `System.reboot()`).

Пример:

```cpp
add_lua_command_to_class("reboot", "System", [](lua_State* L)->int {
  ESP.restart();
  return 0;
});
```

Теперь в Lua:

```lua
System.reboot()
```

📘 Поддерживает логику “объектных” модулей, например:

* `Network.connect()`
* `Audio.play()`
* `System.info()`

---

## 🧰 Доступные Lua-функции (из `luax.h`)

| Категория   | Примеры                                                               |
| ----------- | --------------------------------------------------------------------- |
| GPIO        | `pinMode`, `digitalWrite`, `digitalRead`, `analogRead`, `analogWrite` |
| Serial      | `Serial.begin`, `Serial.print`, `Serial.read`, `Serial.available`     |
| FS (SPIFFS) | `FS.mount`, `FS.open`, `FS.write`, `FS.read`, `FS.close`              |
| SD          | `SD.begin`, `SD.open`, `SD.read`, `SD.write`, `SD.close`              |
| SPI         | `SPI.begin`, `SPI.transfer`, `SPI.transferBytes`                      |
| Математика  | `map`, `constrain`, `abs`, `random`, `randomSeed`                     |
| Битовые     | `bitRead`, `bitSet`, `bitClear`, `bitWrite`                           |
| Звук        | `tone(pin,freq,duration)`, `noTone(pin)`                              |
| Системные   | `delay(ms)`, `delayMicroseconds(us)`, `millis()`, `micros()`          |

---

## 🎮 Примеры Lua-скриптов

### 💡 Моргание светодиодом

```lua
pinMode(2, 1)
while true do
  digitalWrite(2, 1)
  delay(500)
  digitalWrite(2, 0)
  delay(500)
end
```

---

### 🎵 Музыкальный “пик”

```lua
tone(25, 440, 200)
delay(100)
tone(25, 880, 200)
delay(100)
tone(25, 660, 400)
noTone(25)
```

---
### 💾 Работа с SPIFFS

```lua
FS.mount()
f = FS.open("/test.txt", "w")
FS.write(f, "Привет, мир!")
FS.close(f)
```

---

### 📟 Serial Debug

```lua
Serial.begin(115200)
Serial.println("Lua подключена!")
```

---

### 🧠 Генератор случайных чисел

```lua
randomSeed(micros())
for i = 1, 5 do
  Serial.println(random(1, 100))
  delay(200)
end
```

---

### 🕹️ Использование runlua внутри Lua

```lua
Serial.println("Сейчас моргнём!")
runlua("pinMode(2,1); digitalWrite(2,1); delay(1000); digitalWrite(2,0)")
```

---

## ⚖️ Сравнение с другими системами

| Характеристика       | **ESP32-Lua** 😎      | **NodeMCU (Lua RTOS)** 🧱 | **MicroPython** 🐍   |
| -------------------- | ------------------------ | ------------------------- | -------------------- |
| Язык исполнения      | Lua 5.4 (стандартный)    | Lua 5.1 (урезанный)       | Python 3.4 subset    |
| Архитектура          | Lua внутри C++           | Lua — ядро прошивки       | Python интерпретатор |
| Гибкость             | Можно добавлять свои API | Нельзя                    | Ограничено           |
| Скорость             | ⚡ Высокая (C++ + Lua)    | Средняя                   | Низкая               |
| Простота интеграции  | Очень лёгкая             | Требует прошивки          | Отдельный билд       |
| Использование памяти | 80–200 КБ                | 400+ КБ                   | 1+ МБ                |

---

## 🧾 Заключение

**ESP32-Lua** — это не просто “встраиваемый Lua”, а полноценная *скриптовая среда для твоей прошивки*!
🦜 Простая, весёлая и гибкая:

* пиши на Lua,
* управляй железом,
* добавляй свои команды,
* живи без прошивок и сборок каждый раз.

