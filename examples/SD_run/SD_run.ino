#include <lua_esp32.h>  // твоя библиотека
#include <SD.h>         // стандартная библиотека Arduino
#include <SPI.h>        // требуется для SD

#define SD_CS_PIN 5     // замени на свой пин CS

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Initializing SD card...");

  // Инициализация SD-карты
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (true);
  }
  Serial.println("SD card initialized.");

  // Пробуем открыть файл
  File file = SD.open("/main.lua");
  if (!file) {
    Serial.println("Failed to open /main.lua");
    while (true);
  }

  // Считываем содержимое файла
  String code = "";
  while (file.available()) {
    code += (char)file.read();
  }
  file.close();

  Serial.println("Running Lua script from SD...");
  Serial.println("-----------------------------");
  Serial.println(code);
  Serial.println("-----------------------------");

  // Инициализация твоего Lua-движка
  setap_lua();

  // Выполнение Lua-кода
  run_lua(code);
}

void loop() {
  // Можно ничего не делать — всё в Lua
}
