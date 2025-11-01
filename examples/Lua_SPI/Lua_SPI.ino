#include <lua_esp32.h>

void setup() {
  setap_lua();

  String code = R"(
      -- SPI на ESP32: SCLK=18, MISO=19, MOSI=23, CS=5
      SPI.begin(18,19,23,5)

      -- Передаем один байт
      local resp = SPI.transfer(0xAA)
      Serial.print("SPI returned: ")
      Serial.println(resp)

      -- Передаем массив байт
      local tx = {0x01,0x02,0x03,0x04}
      local rx = SPI.transferBytes(tx, #tx)
      Serial.print("SPI rx: ")
      for i=1,#rx do
        Serial.print(rx[i])
        Serial.print(" ")
      end
      Serial.println("")
  )";
  run_lua(code);
}

void loop() {
  
}
