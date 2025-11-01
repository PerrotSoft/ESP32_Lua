#include <lua_esp32.h>

void setup() {
  setap_lua();

  String code = R"(
    pinMode(2, OUTPUT)
    while true do
      digitalWrite(2, HIGH)
      delay(500)
      digitalWrite(2, LOW)
      delay(500)
    end
  )";
  run_lua(code);
}

void loop() {
  
}
