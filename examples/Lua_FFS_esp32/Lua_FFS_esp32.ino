#include <lua_esp32.h>

void setup() {
  setap_lua();

  String code = R"(
    -- Монтируем FFS
    if FFFFFS.mount() then
      Serial.println("FFFS mounted!")

      -- Запись файла
      local f = FFFFFS.open("/flash.txt", "w")
      if f then
        FFFFFS.write(f, "Hello Lua FFS\n")
        FFFFFS.close(f)
      end

      -- Чтение файла
      local fr = FFFFFS.open("/flash.txt", "r")
      if fr then
        local s = FFFFFS.read(fr, 128)
        Serial.print("From FFS: ")
        Serial.println(s or "<nil>")
        FFFFFS.close(fr)
      end
    else
      Serial.println("FFFS mount failed")
    end
  )";
  run_lua(code);
}

void loop() {
  
}
