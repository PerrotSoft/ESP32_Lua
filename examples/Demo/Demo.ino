#include <lua_esp32.h>

void setup() {
  setap_lua();

  String code = R"(
      -- GPIO
      pinMode(2, OUTPUT)
      digitalWrite(2, HIGH)
      delay(200)
      digitalWrite(2, LOW)

      -- analogRead (ESP32 ADC)
      local a = analogRead(36)
      Serial.print("ADC36=")
      Serial.println(a)

      -- analogWrite (ESP32 uses LEDC)
      analogWrite(13, 128) -- 50% duty
      delay(500)
      analogWrite(13, 0)

      -- millis / micros
      Serial.print("millis=")
      Serial.println(millis())
      Serial.print("micros=")
      Serial.println(micros())

      -- map / constrain / random
      local m = map(50,0,100,0,255)
      Serial.print("map(50)=")
      Serial.println(m)
      randomSeed(12345)
      Serial.println(random(0,100))

      -- Bit helpers
      local v = bitSet(0,3)
      Serial.println(v) -- 8
      -- Генерация тона на пине 15, 1kHz, 300ms
      tone(15, 1000, 300)
      delay(500)
      noTone(15)

  )";
  run_lua(code);
}

void loop() {
  
}
