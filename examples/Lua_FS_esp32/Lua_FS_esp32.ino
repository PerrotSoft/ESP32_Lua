#include <lua_esp32.h>

void setup() {
  setap_lua();

  String code = R"(
    -- Монтируем FS
    if FS.mount() then
      Serial.println("FS mounted!")

      -- Запись файла
      local f = FS.open("/flash.txt", "w")
      if f then
        FS.write(f, "Hello Lua FS\n")
        FS.close(f)
      end

      -- Чтение файла
      local fr = FS.open("/flash.txt", "r")
      if fr then
        local s = FS.read(fr, 128)
        Serial.print("From FS: ")
        Serial.println(s or "<nil>")
        FS.close(fr)
      end
    else
      Serial.println("FS mount failed")
    end
  )";
  run_lua(code);
}

void loop() {
  
}
