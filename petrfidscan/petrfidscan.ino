#include <U8g2lib.h>
#include <U8x8lib.h>

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8;

unsigned long displayOnTime = 0;
int displayOn = true;

void writeString(const char* str) {
  Serial.write(str);
  Serial.write("\n");
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, str);
  displayOnTime = millis();
  displayOn = true;
  u8x8.setPowerSave(0);
}

void checkDisplayOn() {
  if (displayOn) {
    unsigned long now = millis();
    if ((now - displayOnTime) > 30000) {
      displayOn = false;
      u8x8.setPowerSave(1);
    }
  }
}

void mycb(char* id, char* raw) {
  writeString(id);
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  petid_set_callback(mycb);

  u8x8.begin();
  writeString("READY");
}

void loop() {
  checkDisplayOn();

  if (Serial1.available()) {
    unsigned char inByte = Serial1.read();
    pettag_data_put(inByte);
  }
}
