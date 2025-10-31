#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <lua_esp32.h>
// ====== HW pins ======
#define TFT_CS   5
#define TFT_DC   21
#define TFT_RST  2
#define TOUCH_CS 15
#define TOUCH_IRQ 4

SPIClass SPI2(HSPI);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

void setup() {
  Serial.begin(115200);
  SPIFFS.begin(true);

  tft.begin();
  SPI2.begin(14, 12, 13);
  ts.begin(SPI2);
  ts.setRotation(1);
  tft.setRotation(1);

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  setap_lua(tft, ts);

  String code = R"(
    -- Lua: рисуем кнопку, читаем тач
    esp.draw_text(10, 10, "Hello from Lua!", 2)
    esp.draw_button(20, 40, 120, 40, "PRESS")
  )";
  run_lua(code);
}

void loop() {
  
}
