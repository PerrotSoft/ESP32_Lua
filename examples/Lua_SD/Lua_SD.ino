#include <lua_esp32.h>

void setup() {
  setap_lua();

  String code = R"(
    -- SD на CS 5 (можно изменить)
    local SD_CS = 5
    SPI.begin(18, 19, 23, SD_CS)

    if SD.begin(SD_CS) then
      Serial.println("SD mounted!")

      -- Запись файла
      local f = SD.open("/test.txt", "w")
      if f then
        SD.write(f, "Hello from Lua on SD!\n")
        SD.close(f)
      end

      -- Чтение файла
      local fr = SD.open("/test.txt", "r")
      if fr then
        local s = SD.read(fr, 128) -- читаем до 128 байт
        Serial.print("From SD: ")
        Serial.println(s or "<nil>")
        SD.close(fr)
      end
    else
      Serial.println("SD mount failed")
    end
  )";
  run_lua(code);
}

void loop() {
  
}
