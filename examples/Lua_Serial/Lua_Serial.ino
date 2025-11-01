#include <lua_esp32.h>

void setup() {
  setap_lua();

  String code = R"(
    -- Инициализация порта
    Serial:begin(115200)
    Serial:println("=== Lua Serial Demo for ESP32 ===")
    Serial:println("Введите что-нибудь в Serial Monitor...")

    -- Бесконечный цикл чтения/ответа
    while true do
      -- Проверяем, есть ли данные в буфере
      if Serial:available() > 0 then
        -- Считываем строку
        local input = Serial:readString()
        
        -- Очищаем символы новой строки
        input = string.gsub(input, "[\r\n]", "")

        -- Отправляем ответ
        Serial:print("Вы ввели: ")
        Serial:println(input)

        -- Реакция на команды
        if input == "led on" then
          pinMode(2, OUTPUT)
          digitalWrite(2, HIGH)
          Serial:println("LED включен.")
        elseif input == "led off" then
          pinMode(2, OUTPUT)
          digitalWrite(2, LOW)
          Serial:println("LED выключен.")
        elseif input == "exit" then
          Serial:println("Выход из программы.")
          break
        else
          Serial:println("Неизвестная команда.")
        end
      end
      
      delay(100)
    end

    Serial:println("Программа завершена.")
  )";
  run_lua(code);
}

void loop() {
  
}
