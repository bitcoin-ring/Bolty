#ifndef GUI_H
#define GUI_H

#include "Button2.h"
#include "ESPAsyncWebSrv.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <qrcode.h>

/*#define TFT_MOSI            19
  #define TFT_SCLK            18
  #define TFT_CS              5
  #define TFT_DC              16
  #define TFT_RST             23
*/
#define TFT_BL 4 // Display backlight control pin

#define ADC_EN 14 // ADC_EN is the ADC detection enable port
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

#define DEFAULT_TEXTSIZE 0

#define BTNMODE_IDLE 0
#define BTNMODE_BRIGHTNESS 1
#define BTNMODE_APP 2

#define APPWHITE TFT_WHITE
#define APPBLACK TFT_BLACK
#define APPGREEN TFT_DARKGREEN
#define APPYELLOW TFT_YELLOW
#define APPORANGE TFT_ORANGE
#define APPRED TFT_RED

struct varcollection {
  uint8_t app;
  byte tft_brightness;
  uint8_t buttonmode;
  byte evbuttons[2];
  byte appbuttons[2];
};

varcollection sharedvars = {
    .app = 0,
    .tft_brightness = 30,
    .buttonmode = BTNMODE_IDLE,
    .evbuttons = {0, 0},
};

// ST7789-based displays, we will use this call
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
int vref = 1100;

void enablebrightnessctrl() { sharedvars.buttonmode = BTNMODE_BRIGHTNESS; }

uint16_t fromrgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void displayText(int col, int row, String txt) {
  tft.fillScreen(APPWHITE);
  tft.setCursor(col * 10, row * 10);
  tft.print(txt);
}

int displayTextCentered(int y, String txt) {
  // center the amount as its dynamic
  int16_t w = tft.textWidth(txt);
  int16_t h = tft.fontHeight();
  // tft.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  // tft.getTextBounds("A|W", 0, 120, &x1, &y1, &w2, &h2);
  tft.setCursor(tft.width() / 2 - w / 2, y);
  tft.print(txt);
  return (y + h) * 1.01;
}

int displayTextLeft(int y, String txt) {
  // center the text
  int16_t x1, y1;
  int16_t w = tft.textWidth(txt);
  int16_t h = tft.fontHeight();
  tft.setCursor(3, y);
  tft.print(txt);
  return (y + h) * 1.01;
}

uint16_t v = 0;
static uint64_t timeStamp = 0;

void draw_battery(bool force_update = false) {
  if ((!force_update) && (millis() - timeStamp < 10000)) {
    return;
  }
  v = analogRead(ADC_PIN);
  timeStamp = millis();
  float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  String voltage = "Voltage :" + String(battery_voltage) + "V";
  Serial.println(voltage);
  float emptyvoltage = 3.2;
  if (battery_voltage < emptyvoltage) {
    emptyvoltage = battery_voltage;
  }
  float fullvoltage = 4.2 - emptyvoltage;
  int percentage = int(100. / fullvoltage * (battery_voltage - emptyvoltage));
  if (percentage > 100) {
    percentage = 100;
  }
  uint16_t batcolor = APPGREEN;
  uint16_t txtcolor = APPWHITE;

  if (percentage < 80) {
    batcolor = APPYELLOW;
    txtcolor = APPBLACK;
  }
  if (percentage < 60) {
    batcolor = APPORANGE;
    txtcolor = APPBLACK;
  }
  if (percentage < 30) {
    batcolor = APPRED;
    txtcolor = APPWHITE;
  }
  Serial.println(voltage);
  int posx = 205;
  int posy = 15;
  int w = 30;
  int h = 10;
  uint8_t ws = int((w) / 100. * percentage);
  tft.fillRoundRect(posx, posy - h, w, h, 2, fromrgb(0x9d, 0x9f, 0x92));
  tft.fillRoundRect(posx, posy - h, ws, h, 2, batcolor);
  tft.drawRoundRect(posx, posy - h, w, h, 2, APPBLACK);
  h = 8;
  if (battery_voltage > 4.3) {
    tft.setFreeFont();
    tft.setTextColor(txtcolor);
    tft.setCursor(posx + 6, posy - 8);
    tft.print("PWR");
  } else {
    tft.setFreeFont();
    tft.setTextColor(txtcolor);
    tft.setCursor(posx + 3, posy - 8);
    tft.print(String(battery_voltage));
  }
}

void draw_wifi(bool wifi_enabled) {
  // wifi status
  int w = 30;
  int h = 10;
  int posx = 5;
  int posy = 15;
  tft.setFreeFont();
  if (wifi_enabled) {
    tft.fillRoundRect(posx, posy - h, w, h, 2, APPGREEN);
    tft.setTextColor(APPWHITE);
  } else {
    tft.fillRoundRect(posx, posy - h, w, h, 2, APPWHITE);
    tft.setTextColor(fromrgb(0xad, 0xaf, 0xa2));
  }
  tft.drawRoundRect(posx, posy - h, w, h, 2, APPBLACK);
  tft.setCursor(posx + 3, posy - 8);
  tft.print("WiFi");
}
void screen_wait() {
  tft.fillScreen(APPWHITE);
  tft.setFreeFont(&FreeSans9pt7b);
  displayTextCentered(75, "Please wait...");
}

void button_loop() {
  sharedvars.evbuttons[0] = 0;
  sharedvars.evbuttons[1] = 0;
  btn1.loop();
  btn2.loop();
}

/* this function will be invoked when additionalTask was created */
void handlebuttonevents(void *data) {
  varcollection *sharedp = (varcollection *)data;
  /* loop forever */
  while (true) {
    switch (sharedp->buttonmode) {
    /*case BTNMODE_BRIGHTNESS:
      {
      //Serial.println("BTNMODE_BRIGHTNESS");
      if ((sharedp->evbuttons[0] == 1) && (sharedp->evbuttons[1] == 0)){
        sharedp->tft_brightness = constrain(sharedp->tft_brightness + 20,
      1,255); ledcWrite(0, sharedp->tft_brightness); // 0-15, 0-255 (with 8 bit
      resolution); 0=totally dark;255=totally shiny
      }
      else if ((sharedp->evbuttons[1] == 1) && (sharedp->evbuttons[0] == 0)){
        sharedp->tft_brightness = constrain(sharedp->tft_brightness - 20,
      1,255); ledcWrite(0, sharedp->tft_brightness); // 0-15, 0-255 (with 8 bit
      resolution); 0=totally dark;255=totally shiny
      }
      break;
      }*/
    case BTNMODE_APP: {
      // Serial.println("BTNMODE_APP");
      if ((sharedp->evbuttons[0] == 1) && (sharedp->evbuttons[1] == 0)) {
      } else if ((sharedp->evbuttons[1] == 1) && (sharedp->evbuttons[0] == 0)) {
      } else if ((sharedp->evbuttons[1] == 2) && (sharedp->evbuttons[0] == 2)) {
        break;
      }
      break;
    }
    }
    button_loop();
    delay(30);
  }
}

void drawscreen(void *data) {
  while (true) {
    draw_battery();
    delay(1000);
  }
};

bool displayQR(String input) {
  // This code from https://github.com/arcbtc/koopa/blob/master/main.ino

  // auto detect best qr code size
  int qrSize = 10;
  int ec_lvl = 0;
  int const sizes[18][4] = {
      /* https://github.com/ricmoo/QRCode */
      /* 1 */ {17, 14, 11, 7},
      /* 2 */ {32, 26, 20, 14},
      /* 3 */ {53, 42, 32, 24},
      /* 4 */ {78, 62, 46, 34},
      /* 5 */ {106, 84, 60, 44},
      /* 6 */ {134, 106, 74, 58},
      /* 7 */ {154, 122, 86, 64},
      /* 8 */ {192, 152, 108, 84},
      /* 9 */ {230, 180, 130, 98},   // OPN:0 LND:0 good
      /* 10 */ {271, 213, 151, 119}, // BTP:0 OPN:1 good
      /* 11 */ {321, 251, 177, 137},
      /* 12 */ {367, 287, 203, 155}, // BTP:1 bad
      /* 13 */ {425, 331, 241, 177}, // BTP:2 meh
      /* 14 */ {458, 362, 258, 194},
      /* 15 */ {520, 412, 292, 220},
      /* 16 */ {586, 450, 322, 250},
      /* 17 */ {644, 504, 364, 280},
  };
  int len = input.length();
  for (int ii = 0; ii < 17; ii++) {
    qrSize = ii + 1;
    if (sizes[ii][ec_lvl] > len) {
      break;
    }
  }

  Serial.printf("len = %d, ec_lvl = %d, qrSize = %d\n", len, ec_lvl, qrSize);

  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, ec_lvl, input.c_str());

  Serial.printf("saw qr mode = %d\n", qrcode.mode);

  int xoff = tft.width() / 2 - qrcode.size;
  int yoff = tft.height() / 2 - qrcode.size;
  tft.fillScreen(APPWHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {

    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) {

      // Print each module (UTF-8 \u2588 is a solid block)
      // Serial.print(qrcode_getModule(&qrcode, x, y) ? "\u2588\u2588": "  ");
      tft.fillRect(xoff + x * 2, yoff + y * 2, 2, 2,
                   qrcode_getModule(&qrcode, x, y) ? APPBLACK : APPWHITE);
    }
  }
  delay(10000);
}

bool SendQR(String input, AsyncResponseStream *response) {
  // This code from https://github.com/arcbtc/koopa/blob/master/main.ino

  // auto detect best qr code size
  int qrSize = 10;
  int ec_lvl = 0;
  int const sizes[18][4] = {
      /* https://github.com/ricmoo/QRCode */
      /* 1 */ {17, 14, 11, 7},
      /* 2 */ {32, 26, 20, 14},
      /* 3 */ {53, 42, 32, 24},
      /* 4 */ {78, 62, 46, 34},
      /* 5 */ {106, 84, 60, 44},
      /* 6 */ {134, 106, 74, 58},
      /* 7 */ {154, 122, 86, 64},
      /* 8 */ {192, 152, 108, 84},
      /* 9 */ {230, 180, 130, 98},   // OPN:0 LND:0 good
      /* 10 */ {271, 213, 151, 119}, // BTP:0 OPN:1 good
      /* 11 */ {321, 251, 177, 137},
      /* 12 */ {367, 287, 203, 155}, // BTP:1 bad
      /* 13 */ {425, 331, 241, 177}, // BTP:2 meh
      /* 14 */ {458, 362, 258, 194},
      /* 15 */ {520, 412, 292, 220},
      /* 16 */ {586, 450, 322, 250},
      /* 17 */ {644, 504, 364, 280},
  };
  int len = input.length();
  for (int ii = 0; ii < 17; ii++) {
    qrSize = ii + 1;
    if (sizes[ii][ec_lvl] > len) {
      break;
    }
  }

  Serial.printf("len = %d, ec_lvl = %d, qrSize = %d\n", len, ec_lvl, qrSize);

  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, ec_lvl, input.c_str());

  Serial.printf("saw qr mode = %d\n", qrcode.mode);

  int xoff = tft.width() / 2 - qrcode.size;
  int yoff = tft.height() / 2 - qrcode.size;
  // tft.fillScreen(APPWHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {

    // Each horizontal module
    for (uint8_t x = 0; x < qrcode.size; x++) {

      // Print each module (UTF-8 \u2588 is a solid block)
      // Serial.print(qrcode_getModule(&qrcode, x, y) ? "\u2588\u2588": "  ");
      response->print(qrcode_getModule(&qrcode, x, y) ? "\u2588\u2588" : "  ");
      // tft.fillRect(xoff + x*2, yoff + y*2, 2, 2, qrcode_getModule(&qrcode, x,
      // y) ? APPBLACK: APPWHITE);
    }
    response->print("<br/>");
  }
}

void button_init() {
  btn1.setReleasedHandler([](Button2 &b) {
    // displayText(2,2,"btn1 click");
    sharedvars.evbuttons[0] = 1;
    sharedvars.appbuttons[0] = 1;
  });
  btn1.setLongClickHandler([](Button2 &b) {
    unsigned int time = b.wasPressedFor();
    if (time > 1000) {
      sharedvars.evbuttons[0] = 2;
      sharedvars.appbuttons[0] = 2;
    }
    // displayText(2,2,"btn1 click");
  });
  btn1.setDoubleClickHandler([](Button2 &b) {
    // displayText(2,2,"btn2 click");
    sharedvars.evbuttons[0] = 3;
    sharedvars.appbuttons[0] = 3;
  });

  btn2.setReleasedHandler([](Button2 &b) {
    // displayText(2,2,"btn2 click");
    sharedvars.evbuttons[1] = 1;
    sharedvars.appbuttons[1] = 1;
  });
  btn2.setLongClickHandler([](Button2 &b) {
    // displayText(2,2,"btn2 click");
    sharedvars.appbuttons[1] = 2;
    unsigned int time = b.wasPressedFor();
  });
  btn2.setDoubleClickHandler([](Button2 &b) {
    // displayText(2,2,"btn2 click");
    sharedvars.appbuttons[1] = 3;
    sharedvars.evbuttons[1] = 3;
  });

  // create task for changing display brightness
  xTaskCreatePinnedToCore(handlebuttonevents,     /* Task function. */
                          "Button Event Handler", /* name of task. */
                          1000,                   /* Stack size of task */
                          &sharedvars,            /* parameter of the task */
                          1,                      /* priority of the task */
                          NULL, /* Task handle to keep track of created task */
                          0);
}

void setup_display() {
  button_init();
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(APPWHITE);
  tft.setTextColor(APPBLACK);
  pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
  // digitalWrite(TFT_BL, HIGH); // Turn backlight on.
  pinMode(TFT_BL, OUTPUT);
  ledcSetup(0, 5000, 8);    // 0-15, 5000, 8
  ledcAttachPin(TFT_BL, 0); // TFT_BL, 0 - 15
  ledcWrite(0,
            sharedvars.tft_brightness); // 0-15, 0-255 (with 8 bit resolution);
                                        // 0=totally dark;255=totally shiny
  tft.setFreeFont(&FreeSans18pt7b);
  displayTextCentered(80, "Bolty");
  tft.setFreeFont(&FreeSans9pt7b);
  // tft.setFreeFont();
  // tft.setTextSize(2);
  displayTextCentered(120, "Bolt Card assistant");
  enablebrightnessctrl();
}

#endif
