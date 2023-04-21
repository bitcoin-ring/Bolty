/**************************************************************************/
/*!
    @file     bolty.ino
    @author   Thilo
    https://github.com/bitcoin-ring/Bolty
    This sketch will start a wifi in AP or STA mode (WIFIMODE_*). If you want to
   use STA mode you have to enter the WIFI credentials of your existing network.
    A webserver allows to import keydata  in form of the raw json of the lnbits
   "Card key credentials" link. The keydata is saved on the eepom and should
   therefore be availabe after reboot/powerdown. The saved keydata can be used
   to provision/burn or wipe/delete ntag424 tags i.E. BoltCard, BoltRing.
    Button1:
    - śhort press: Load next boltconfiguration/keyset
    - long press: toggle wifi on/off
    Button2:
    - śhort press: Toggle between burn and wipe
    - long press: sleepmode

*/
/**************************************************************************/
#include "bolt.h"
#include "gui.h"
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_wifi.h>
#define DYNAMIC_JSON_DOCUMENT_SIZE 1024
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "ESPAsyncWebServer.h"
//#include <WiFiClient.h>
#include "FS.h"
#include "SPIFFS.h"
#include <WiFiAP.h>

#define WIFIMODE_AP 0
#define WIFIMODE_STA 1

#define CONFIGVERSION 1

#define WIFI_AP_PASSWORD_LENGTH 8

// remove next line for random password generation
#define WIFI_AP_PASSWORD_STATIC "wango123"

#define HTTP_CREDENTIAL_SIZE 16

//iuf you change this do not exceed HTTP_CREDENTIAL_SIZE-1 chars!
const char* http_default_username = "bolty";
const char* http_default_password = "bolty";


#define MAX_BOLT_DEVICES 5
char charpool[] = {
    "qweertzunopaasdfghjkyxcvbnm-?234567890QWEERTYUOPAASDFGHJKLZXCVBNM"};

// Set these to your desired credentials for WiFi AP-Mode.

char *ap_ssid = "Bolty";
#ifdef WIFI_AP_PASSWORD_STATIC
char ap_password[] = WIFI_AP_PASSWORD_STATIC;
#else
char ap_password[WIFI_AP_PASSWORD_LENGTH];
#endif

AsyncWebServer server(80);
const char *PARAM_CONFIG = "config";

// NFC Pins
#define PN532_SCK (17)
#define PN532_MOSI (13)
#define PN532_SS (15)
#define PN532_MISO (12)
#define PN532_RSTPD_N (2)
// For RSTPD_N to work i had to desolder a 10k resistor between RSTPD_N and VCC

#define APPS (3)
#define APP_KEYSETUP (0)
#define APP_BOLTBURN (1)
#define APP_BOLTWIPE (2)

#define APP_STATUS_START (0)
#define APP_STATUS_LOOP (1)
#define APP_STATUS_END (2)

BoltDevice bolt(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

struct sSettings {
  char essid[33];
  char password[65];
  uint8_t wifimode;
  bool wifi_enabled;
  char http_username[HTTP_CREDENTIAL_SIZE];
  char http_password[HTTP_CREDENTIAL_SIZE];
};
sSettings mSettings;

uint8_t app_active;
int8_t app_next;
uint8_t app_status;
String SIpAddress = "Waiting for ip..";
IPAddress myIP;

uint8_t active_bolt_config;
sBoltConfig mBoltConfig;

bool signal_update_screen = false;

typedef void (*tAppHandler)();
typedef void (*tEvtHandler)(uint8_t btn, uint8_t evt);

struct sAppHandler {
  String app_title;
  String app_desc;
  tAppHandler app_start; // function pointer start lifecycle
  tAppHandler app_end;   // function pointer end lifecycle
  tAppHandler app_loop;  // function pointer mainloop
  uint16_t app_fgcolor;
  uint16_t app_bgcolor;
};

sAppHandler mAppHandler[APPS];

void dumpconfig() {
  Serial.println(mBoltConfig.wallet_name);
  Serial.println(mBoltConfig.wallet_host);
  Serial.println(mBoltConfig.wallet_url);
  Serial.println(mBoltConfig.uid);
  Serial.println(mBoltConfig.card_name);
  Serial.println(mBoltConfig.url);
  Serial.println(mBoltConfig.k0);
  Serial.println(mBoltConfig.k1);
  Serial.println(mBoltConfig.k2);
  Serial.println(mBoltConfig.k3);
  Serial.println(mBoltConfig.k4);
}

void dumpsettings() {
  Serial.println(mSettings.wifimode);
  Serial.println(mSettings.essid);
  Serial.println(mSettings.password);
  Serial.println(mSettings.wifi_enabled);
  Serial.println(mSettings.http_username);
  Serial.println(mSettings.http_password);
}

void saveSettings() {
  char path[20];
  sprintf(path, "/settings.dat");
  fs::File myFile = SPIFFS.open(path, FILE_WRITE);
  myFile.write((byte *)&mSettings, sizeof(sSettings));
  myFile.close();
}

void loadSettings() {
  char path[20];
  sprintf(path, "/settings.dat");
  Serial.print(path);
  if (SPIFFS.exists(path) == 1) {
    Serial.println(" found");
    fs::File myFile = SPIFFS.open(path, FILE_READ);
    myFile.read((byte *)&mSettings, sizeof(sSettings));
    myFile.close();
  } else {
    Serial.println(" not found");
    mSettings.essid[0] = 0;
    mSettings.password[0] = 0;
    mSettings.wifimode = WIFIMODE_AP;
    mSettings.wifi_enabled = 1;
    memcpy(mSettings.http_username, http_default_username, HTTP_CREDENTIAL_SIZE);
    memcpy(mSettings.http_password, http_default_password, HTTP_CREDENTIAL_SIZE);
  }
  dumpsettings();
}

void saveBoltConfig(uint8_t slot) {
  char path[20];
  sprintf(path, "/config%02x.dat", slot);
  fs::File myFile = SPIFFS.open(path, FILE_WRITE);
  myFile.write((byte *)&mBoltConfig, sizeof(sBoltConfig));
  myFile.close();
}

/*
uint8_t boltconfig_key[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; uint8_t newlen =
bolt.nfc->ntag424_addpadding(strlen(mBoltConfig.url), 16,  (uint8_t *)
&mBoltConfig.url); Serial.println(bolt.nfc->ntag424_encrypt(boltconfig_key,
newlen, (uint8_t *) &mBoltConfig.url, testurl_enc));
Serial.println(bolt.nfc->ntag424_decrypt(boltconfig_key, newlen, testurl_enc,
testurl)); bolt.nfc->PrintHexChar(testurl, 56);
*/

void loadBoltConfig(uint8_t slot) {
  char path[20];
  sprintf(path, "/config%02x.dat", slot);
  Serial.print(path);
  Serial.println(sizeof(((sBoltConfig *)0)->url));
  memset(&mBoltConfig, 0, sizeof(sBoltConfig));
  uint8_t testurl_enc[sizeof(((sBoltConfig *)0)->url)];
  uint8_t testurl[sizeof(((sBoltConfig *)0)->url)];
  if (SPIFFS.exists(path) == 1) {
    Serial.println(" found");
    fs::File myFile = SPIFFS.open(path, FILE_READ);
    myFile.read((byte *)&mBoltConfig, sizeof(sBoltConfig));
    myFile.close();
  } else {
    Serial.println(" not found");
    strcpy(mBoltConfig.card_name, "*new*");
    mBoltConfig.url[0] = 0;
    mBoltConfig.uid[0] = 0;
    mBoltConfig.k0[0] = 0;
    mBoltConfig.k1[0] = 0;
    mBoltConfig.k2[0] = 0;
    mBoltConfig.k3[0] = 0;
    mBoltConfig.k4[0] = 0;
    mBoltConfig.wallet_name[0] = 0;
    mBoltConfig.wallet_url[0] = 0;
    mBoltConfig.wallet_host[0] = 0;
  }
  dumpconfig();
}

String exportBoltConfig() {
  String path = "/backup.dat";
  SPIFFS.remove(path);
  fs::File myFile = SPIFFS.open(path, FILE_APPEND);
  for (uint8_t i = 0; i < MAX_BOLT_DEVICES; i++) {
    loadBoltConfig(i);
    myFile.write((byte *)&mBoltConfig, sizeof(sBoltConfig));
  }
  myFile.close();
  loadBoltConfig(active_bolt_config);
  return path;
}

void importBoltConfig() {
  String path = "/backup.dat";
  fs::File myFile = SPIFFS.open(path, FILE_READ);
  for (uint8_t i = 0; i < MAX_BOLT_DEVICES; i++) {
    myFile.seek(i * sizeof(sBoltConfig));
    myFile.readBytes((char *)&mBoltConfig, sizeof(sBoltConfig));
    // loadBoltConfig(i);
    saveBoltConfig(i);
    dumpconfig();
  }
  myFile.close();
  loadBoltConfig(active_bolt_config);
}

// Keysetup
void app_keysetup_start() { Serial.println("app_KEYSETUP_start"); }

long lasttime = 0;
String default_app_message = "* Buttons are locked! *";
String app_message = default_app_message;
void app_keysetup_loop() {
  if ((millis() - lasttime) > 200) {
    lasttime = millis();
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(APPRED);
    bool success = bolt.scanUID();
    app_message = bolt.getScannedUid();
    if (app_message == "") {
      app_message = default_app_message;
    }
    tft.fillRect(0, -3 + (3 * 23), tft.width(), 21, APPWHITE);
    displayTextCentered(-3 + (4 * 21), app_message);
  }
  if (!mSettings.wifi_enabled) {
    app_next = APP_BOLTBURN;
  }
  delay(50);
}

String shortenkeys(const String &var){
  return var.substring(0,3) + "*************";
}

String processor_default(const String &var){
  if (var == "cnn")
    return String(active_bolt_config + 1).c_str();
  if (var == "cn")
    return mBoltConfig.card_name;
  if (var == "url")
    return mBoltConfig.url;
  if (var == "ks0")
    return shortenkeys(mBoltConfig.k0);
  if (var == "ks1")
    return shortenkeys(mBoltConfig.k1);
  if (var == "ks2")
    return shortenkeys(mBoltConfig.k2);
  if (var == "ks3")
    return shortenkeys(mBoltConfig.k3);
  if (var == "ks4")
    return shortenkeys(mBoltConfig.k4);
  if (var == "k0")
    return mBoltConfig.k0;
  if (var == "k1")
    return mBoltConfig.k1;
  if (var == "k2")
    return mBoltConfig.k2;
  if (var == "k3")
    return mBoltConfig.k3;
  if (var == "k4")
    return mBoltConfig.k4;
  return String();
}

String web_keysetup_processor(const String &var) {
  // Serial.println("web_keysetup_loop");
  if (var == "wallet_name")
    return mBoltConfig.wallet_name;
  if (var == "wallet_host")
    return mBoltConfig.wallet_host;
  if (var == "wallet_url")
    return mBoltConfig.wallet_url;
  if (var == "wallet_link")
    return mBoltConfig.wallet_url;
  if (var == "uid")
    return mBoltConfig.uid;
  return processor_default(var);
}

void app_keysetup_end() { Serial.println("app_KEYSETUP_end"); }

// Ringsetup
void APP_BOLTBURN_start() { Serial.println("APP_BOLTBURN_start"); }

uint32_t previousMillis;
uint32_t Interval = 100;

void APP_BOLTBURN_loop() {
  // Serial.println("APP_BOLTBURN_loop");
  // set the keys
  if (millis() - previousMillis < Interval) {
    return;
  }
  previousMillis = millis();
  Interval = 100;
  bolt.setDefautKeysCur();
  bolt.setNewKey(mBoltConfig.k0, 0);
  bolt.setNewKey(mBoltConfig.k1, 1);
  bolt.setNewKey(mBoltConfig.k2, 2);
  bolt.setNewKey(mBoltConfig.k3, 3);
  bolt.setNewKey(mBoltConfig.k4, 4);
  String lnurl = String(mBoltConfig.url);
  uint8_t burn_result = bolt.burn(lnurl);
  if (burn_result != JOBSTATUS_WAITING) {
    previousMillis = millis();
    Interval = 3000;
    dumpconfig();
  }
}


String web_burn_processor(const String &var) {
  Serial.println("web_ringsetup_loop");
  if (var == "job")
    return "Burn";
  return processor_default(var);
}
void APP_BOLTBURN_end() { Serial.println("APP_BOLTBURN_end"); }
// Ringsetup
void APP_BOLTWIPE_start() { Serial.println("APP_BOLTWIPE_start"); }

void APP_BOLTWIPE_loop() {
  // Serial.println("APP_BOLTWIPE_loop");
  if (millis() - previousMillis < Interval) {
    return;
  }
  previousMillis = millis();
  Interval = 100;
  bolt.setDefautKeysNew();
  bolt.setCurKey(mBoltConfig.k0, 0);
  bolt.setCurKey(mBoltConfig.k1, 1);
  bolt.setCurKey(mBoltConfig.k2, 2);
  bolt.setCurKey(mBoltConfig.k3, 3);
  bolt.setCurKey(mBoltConfig.k4, 4);
  uint8_t wipe_result = bolt.wipe();
  if (wipe_result != JOBSTATUS_WAITING) {
    previousMillis = millis();
    Interval = 3000;
    dumpconfig();
  }
}

String web_ringwipe_processor(const String &var) {
  Serial.println("web_ringwipe_loop");
  if (var == "job")
    return "Wipe";
  return processor_default(var);
}
void APP_BOLTWIPE_end() { Serial.println("APP_BOLTWIPE_end"); }

void wifi_stop() {
  server.end();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_stop();
  Serial.println("WIFI: Disconnected");
  delay(100);
  WiFi.mode(WIFI_OFF);
  delay(100);
  if (mSettings.wifi_enabled) {
    mSettings.wifi_enabled = false;
    saveSettings();
  }
}

void wifi_start() {
  // if we have credentials try connecting. WIFIMODE_STA will fallback to
  // mSettings.wifimode == WIFIMODE_AP
  if ((mSettings.essid != "") && (mSettings.password != "")) {
    mSettings.wifimode = WIFIMODE_STA;
  }
  if (mSettings.wifimode == WIFIMODE_AP) {
#ifndef WIFI_AP_PASSWORD_STATIC
    randomchar(ap_password, sizeof(ap_password));
#endif
    WiFi.softAP(ap_ssid, ap_password);
    myIP = WiFi.softAPIP();
  }

  uint8_t connect_count = 0;
  if (mSettings.wifimode == WIFIMODE_STA) {
    WiFi.begin(mSettings.essid, mSettings.password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      connect_count++;
      Serial.println("Connecting to WiFi..");
      // fall back to AP mode if we cannot connect for 10 seconds
      if (connect_count > 10) {
        Serial.println("Cannot connect to Network. Fallback to AP-Mode.");
        wifi_stop();
        delay(50);
        mSettings.wifimode = WIFIMODE_AP;
#ifndef WIFI_AP_PASSWORD_STATIC
        randomchar(ap_password, sizeof(ap_password));
#endif
        WiFi.softAP(ap_ssid, ap_password);
        myIP = WiFi.softAPIP();
        break;
      }
    }
    // still STA mode? then we should be connected by now!
    if (mSettings.wifimode == WIFIMODE_STA) {
      myIP = WiFi.localIP();
    }
  }
  if (!mSettings.wifi_enabled) {
    mSettings.wifi_enabled = true;
    saveSettings();
    //set wifimode  after savesettings. we dont want to persist it if we fallback to apmode.
    if ((mSettings.wifimode == WIFIMODE_STA) && (connect_count <= 10)) {
      mSettings.wifimode = WIFIMODE_AP;
    }
  }
  server.begin();
}

void wifi_toogle() {
  if (mSettings.wifi_enabled) {
    wifi_stop();
  } else {
    wifi_start();
  }
}

void nfc_start() {
  Serial.println("switching nfc on");
  digitalWrite(PN532_RSTPD_N, HIGH);
}

void nfc_stop() {
  Serial.println("switching nfc off");
  digitalWrite(PN532_RSTPD_N, LOW);
}


String getIpAddress() {
  if (mSettings.wifimode == WIFIMODE_AP)
    myIP = WiFi.softAPIP();
  if (mSettings.wifimode == WIFIMODE_STA)
    myIP = WiFi.localIP();
  return myIP.toString();
}

uint8_t lineh = 21;
void update_screen() {
  tft.fillScreen(fromrgb(0xed, 0xef, 0xf2));
  int8_t ofs = -3;
  tft.fillRect(0, 0, tft.width(), 23, mAppHandler[app_active].app_bgcolor);
  draw_battery(true);
  draw_wifi(mSettings.wifi_enabled);

  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(APPWHITE);
  displayTextCentered(ofs + (1 * lineh), mAppHandler[app_active].app_title);
  tft.setTextColor(mAppHandler[app_active].app_fgcolor);
  displayTextCentered(ofs + (2 * lineh), String(active_bolt_config + 1) + ". " +
                                             mBoltConfig.card_name);
  displayTextCentered(ofs + (3 * lineh), mAppHandler[app_active].app_desc);
  if (mSettings.wifi_enabled) {
    // displayTextLeft(ofs + (4 * lineh), "WiFi");
    if (mSettings.wifimode == WIFIMODE_AP) {
      displayTextLeft(ofs + (5 * lineh),
                      "WiFi " + String(ap_ssid) + ":" + String(ap_password));
    }
    SIpAddress = getIpAddress();
    displayTextLeft(ofs + (6 * lineh), SIpAddress);
  }
  signal_update_screen = false;
}

void handle_events() {
  // Button 0 short clicky = next keyset
  if ((sharedvars.appbuttons[0] == 1) && (app_active != APP_KEYSETUP)) {
    // we dont want to interrupt anything done with keyysetup
    active_bolt_config += 1;
    if (active_bolt_config >= MAX_BOLT_DEVICES) {
      active_bolt_config = 0;
    }
    loadBoltConfig(active_bolt_config);
    signal_update_screen = true;
  }
  // Button 1 short clicky = next app
  if ((sharedvars.appbuttons[1] == 1) && (app_active != APP_KEYSETUP)) {
    // we dont want to interrupt anything done with keyysetup
    app_next = app_active + 1;
    if (app_next > APPS - 1) {
      app_next = 1;
    }
  }
  // Button 0 long clicky =
  if (sharedvars.appbuttons[0] == 2) {
    wifi_toogle();
    signal_update_screen = true;
  }
  // Button 1 long clicky =
  if (sharedvars.appbuttons[1] == 2) {
    nfc_stop();
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0); // 1 = High, 0 = Low
    esp_deep_sleep_start();
  }
  // Button 0 double clicky =
  if (sharedvars.appbuttons[0] == 3) {
    Serial.println("double click btn0");
    String wurl =
        String(mBoltConfig.wallet_host) + "?" + String(mBoltConfig.wallet_url);
    Serial.println(wurl);
    // displayQR(wurl);
  }
  // Button 1 duoble clicky =
  if (sharedvars.appbuttons[1] == 3) {
    Serial.println("double click btn1");
  }
  // reset button events
  sharedvars.appbuttons[0] = 0;
  sharedvars.appbuttons[1] = 0;
}

void app_stateengine() {
  handle_events();
  if (signal_update_screen){
      update_screen();
  }

  if (app_next >= APPS)
    app_next = 0;
  // do not switch to keysetup using buttons
  if (app_next < 0)
    app_next = APPS - 1;

  if (app_active != app_next) {
    Serial.print("start: current app:");
    Serial.println(app_active);
    Serial.print("switching to app:");
    Serial.println(app_next);
    app_status = APP_STATUS_END;
  }
  if (app_status == APP_STATUS_START) {
    (*mAppHandler[app_active].app_start)();
    app_status = APP_STATUS_LOOP;
    update_screen();
  }
  if (app_status == APP_STATUS_LOOP) {
    draw_battery();
    draw_wifi(mSettings.wifi_enabled);
    (*mAppHandler[app_active].app_loop)();
  }
  if (app_status == APP_STATUS_END) {
    Serial.print("end: ending app:");
    Serial.println(app_active);
    Serial.print("end: activating app:");
    Serial.println(app_next);
    (*mAppHandler[app_active].app_end)();
    app_active = app_next;
    app_status = APP_STATUS_START;
  }
}

void checkparams(AsyncWebServerRequest *request) {
  if (request->hasParam("d")) {
    AsyncWebParameter *p = request->getParam("d");
    Serial.println("got d param");
    Serial.println(p->value().c_str());
    if (p->value() == "p") {
      active_bolt_config =
          constrain((active_bolt_config - 1), 0, MAX_BOLT_DEVICES - 1);
    }
    if (p->value() == "n") {
      active_bolt_config =
          constrain((active_bolt_config + 1), 0, MAX_BOLT_DEVICES - 1);
    }
    loadBoltConfig(active_bolt_config);
    signal_update_screen = true;
  }
  Serial.println(active_bolt_config);
}

void empty() {
  //
}

void randomchar(char *outbuf, uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(i);
    Serial.print(":");
    outbuf[i] = charpool[random(0, 63)];
    Serial.println(outbuf[i]);
  }
  outbuf[count] = 0;
}

// handles uploads
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index,
                  uint8_t *data, size_t len, bool final) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() +
                      " " + request->url();
  Serial.println(logmessage);

  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    // open the file on first call and store the file handle in the request
    // object
    request->_tempFile = SPIFFS.open("/backup.dat", "w");
    Serial.println(logmessage);
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) +
                 " index=" + String(index) + " len=" + String(len);
    Serial.println(logmessage);
  }

  if (final) {
    logmessage = "Upload Complete: " + String(filename) +
                 ",size: " + String(index + len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    Serial.println(logmessage);
    importBoltConfig();
    request->redirect("/");
  }
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // for Leonardo/Micro/Zero
  // setup tft display
  setup_display();
  pinMode(PN532_RSTPD_N, OUTPUT);
  nfc_start();
  bolt.begin();
  Serial.println("Setup done!");
  app_active = APP_KEYSETUP;
  app_next = APP_BOLTBURN;
  app_status = APP_STATUS_START;
  mAppHandler[APP_KEYSETUP].app_title = "Key-Setup";
  mAppHandler[APP_KEYSETUP].app_desc = "Use a webbrowser";
  mAppHandler[APP_KEYSETUP].app_start = app_keysetup_start;
  mAppHandler[APP_KEYSETUP].app_end = app_keysetup_end;
  mAppHandler[APP_KEYSETUP].app_loop = app_keysetup_loop;
  mAppHandler[APP_KEYSETUP].app_fgcolor = APPBLACK;
  mAppHandler[APP_KEYSETUP].app_bgcolor = fromrgb(0x3e, 0xaf, 0x7c);

  mAppHandler[APP_BOLTBURN].app_title = "Burn";
  mAppHandler[APP_BOLTBURN].app_desc = "Burn a Bolt Card";
  mAppHandler[APP_BOLTBURN].app_start = APP_BOLTBURN_start;
  mAppHandler[APP_BOLTBURN].app_end = APP_BOLTBURN_end;
  mAppHandler[APP_BOLTBURN].app_loop = APP_BOLTBURN_loop;
  mAppHandler[APP_BOLTBURN].app_fgcolor = APPBLACK;
  mAppHandler[APP_BOLTBURN].app_bgcolor = fromrgb(0xff, 0xad, 0x33);

  mAppHandler[APP_BOLTWIPE].app_title = "Wipe";
  mAppHandler[APP_BOLTWIPE].app_desc = "Wipe a Bolt Card";
  mAppHandler[APP_BOLTWIPE].app_start = APP_BOLTWIPE_start;
  mAppHandler[APP_BOLTWIPE].app_end = APP_BOLTWIPE_end;
  mAppHandler[APP_BOLTWIPE].app_loop = APP_BOLTWIPE_loop;
  mAppHandler[APP_BOLTWIPE].app_fgcolor = APPBLACK;
  mAppHandler[APP_BOLTWIPE].app_bgcolor = fromrgb(0xee, 0xa0, 0xa0);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // initialize the bolt configurations
  active_bolt_config = 0;
  loadBoltConfig(active_bolt_config);
  Serial.println("Configuring access point...");
  loadSettings();
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    app_next = APP_BOLTBURN;
    checkparams(request);
    request->send(SPIFFS, "/burn.html", String(), false, web_burn_processor);
  });
  // Route for root / web page
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/favicon.ico", "image/png");
  });
  // Export Config
  server.on("/export", HTTP_GET, [](AsyncWebServerRequest *request) {
    String exportfile = exportBoltConfig();
    request->send(SPIFFS, exportfile, "application/octet-stream");
  });

  // Route for keys / web page
  server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(!request->authenticate(mSettings.http_username, mSettings.http_password))
        return request->requestAuthentication();
    app_next = APP_KEYSETUP;
    checkparams(request);
    AsyncWebServerResponse *response = request->beginResponse(
        SPIFFS, "/setup.html", String(), false,
        web_keysetup_processor); // Sends File with cross-origin-header
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    // request->send(SPIFFS, "/setup.html", String(), false,
    // web_keysetup_processor);
  });
  // Route for root / web page
  server.on("/wipe", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Serial.println("request wipe");
    app_next = APP_BOLTWIPE;
    checkparams(request);
    request->send(SPIFFS, "/wipe.html", String(), false,
                  web_ringwipe_processor);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(
        SPIFFS, "/style.css", String());
    response->addHeader("last-modified", "Mon, 13 Jun 2022 11:05:21 GMT");
    response->addHeader("expires", "Sun, 13 Jun 2032 11:05:21 GMT");
    response->addHeader("cache-control", "max-age=86400");
    request->send(response);
  });
  server.on("/setup.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(!request->authenticate(mSettings.http_username, mSettings.http_password))
        return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse(
        SPIFFS, "/setup.js", String());
    response->addHeader("last-modified", "Mon, 13 Jun 2022 11:05:21 GMT");
    response->addHeader("expires", "Sun, 13 Jun 2032 11:05:21 GMT");
    response->addHeader("cache-control", "max-age=86400");
    request->send(response);
  });
  server.on("/bolty.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(
        SPIFFS, "/bolty.js", String());
    response->addHeader("last-modified", "Mon, 13 Jun 2022 11:05:21 GMT");
    response->addHeader("expires", "Sun, 13 Jun 2032 11:05:21 GMT");
    response->addHeader("cache-control", "max-age=86400");
    request->send(response);
  });
  server.on("/qr", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response =
        request->beginResponseStream("text/html; charset=utf-8");
    response->print(
        "<html><head><link rel='stylesheet' media='screen' "
        "href='https://fontlibrary.org/face/dejavu-sans-mono' "
        "type='text/css'></head><body><pre style='font-family: "
        "DejaVuSansMonoBold,monospace;font-size: 0.2em;'>"); // font-size: 1vw;
    String walurl = web_keysetup_processor("wallet_link");
    SendQR(walurl, response);
    response->print("</pre></body></html>");
    request->send(response);
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
      "/status", [](AsyncWebServerRequest *request, JsonVariant &json) {
        // Serial.println(bolt.get_job_status());
        request->send(200, "application/json",
                      "{\"status\":\"" + bolt.get_job_status() +
                          "\",\"app\":\"" + app_active + "\",\"cnn\":\"" +
                          (active_bolt_config + 1) + "\"}");
      });

  server.addHandler(handler);

  AsyncCallbackJsonWebHandler *handler_uid = new AsyncCallbackJsonWebHandler(
      "/uid", [](AsyncWebServerRequest *request, JsonVariant &json) {
        // Serial.println(bolt.get_job_status());
        request->send(200, "application/json",
                      "{\"uid\":\"" + bolt.getScannedUid() + "\"}");
      });

  server.addHandler(handler_uid);

  AsyncCallbackJsonWebHandler *handler2 = new AsyncCallbackJsonWebHandler(
      "/keys", [](AsyncWebServerRequest *request, JsonVariant &json) {
      if(!request->authenticate(mSettings.http_username, mSettings.http_password))
          return request->requestAuthentication();
        Serial.println("received keys");
        StaticJsonDocument<200> data;
        if (json.is<JsonArray>()) {
          data = json.as<JsonArray>();
        } else if (json.is<JsonObject>()) {
          data = json.as<JsonObject>();
        }
        if (data.containsKey("card_name"))
          strcpy(mBoltConfig.card_name, data["card_name"]);
        if (data.containsKey("lnurlw_base"))
          strcpy(mBoltConfig.url, data["lnurlw_base"]);
        if (data.containsKey("wallet_name"))
          strcpy(mBoltConfig.wallet_name, data["wallet_name"]);
        if (data.containsKey("wallet_url"))
          strcpy(mBoltConfig.wallet_url, data["wallet_url"]);
        if (data.containsKey("wallet_host"))
          strcpy(mBoltConfig.wallet_host, data["wallet_host"]);
        if (data.containsKey("uid"))
          strcpy(mBoltConfig.uid, data["uid"]);
        if (data.containsKey("k0"))
          strcpy(mBoltConfig.k0, data["k0"]);
        if (data.containsKey("k1"))
          strcpy(mBoltConfig.k1, data["k1"]);
        if (data.containsKey("k2"))
          strcpy(mBoltConfig.k2, data["k2"]);
        if (data.containsKey("k3"))
          strcpy(mBoltConfig.k3, data["k3"]);
        if (data.containsKey("k4"))
          strcpy(mBoltConfig.k4, data["k4"]);
        saveBoltConfig(active_bolt_config);
        Serial.println(mBoltConfig.card_name);
        request->send(200, "application/json",
                      "{\"status\":\"received_keys\"}");
      });
  server.addHandler(handler2);

  AsyncCallbackJsonWebHandler *handlerwifi = new AsyncCallbackJsonWebHandler(
      "/wifi", [](AsyncWebServerRequest *request, JsonVariant &json) {
      if(!request->authenticate(mSettings.http_username, mSettings.http_password))
        return request->requestAuthentication();
        Serial.println("received wifi-settings");
        StaticJsonDocument<200> data;
        if (json.is<JsonArray>()) {
          data = json.as<JsonArray>();
        } else if (json.is<JsonObject>()) {
          data = json.as<JsonObject>();
        }
        if (data["essid"] != "") {
          strcpy(mSettings.essid, data["essid"]);
        }
        if (data["password"] != "") {
          strcpy(mSettings.password, data["password"]);
        }
        if (data["wifimode"] == "sta") {
          mSettings.wifimode = WIFIMODE_STA;
        } else {
          mSettings.wifimode = WIFIMODE_AP;
        }
        saveSettings();
        request->send(200, "application/json",
                      "{\"status\":\"received_keys\"}");
        ESP.restart();
      });
  server.addHandler(handlerwifi);
  
  AsyncCallbackJsonWebHandler *handlerac = new AsyncCallbackJsonWebHandler(
      "/ac", [](AsyncWebServerRequest *request, JsonVariant &json) {
      if(!request->authenticate(mSettings.http_username, mSettings.http_password))
        return request->requestAuthentication();
        Serial.println("received credential-settings");
        StaticJsonDocument<200> data;
        if (json.is<JsonArray>()) {
          data = json.as<JsonArray>();
        } else if (json.is<JsonObject>()) {
          data = json.as<JsonObject>();
        }
        if ((strlen(data["http_u"]) >= HTTP_CREDENTIAL_SIZE) || (strlen(data["http_p"]) >= HTTP_CREDENTIAL_SIZE)){
          request->send(505, "application/json",
                      "{\"status\":\"credentials too long\"}");
          return;
        }
        if (data["http_u"] != "") {
          strcpy(mSettings.http_username, data["http_u"]);
        }
        if (data["http_p"] != "") {
          strcpy(mSettings.http_password, data["http_p"]);
        }
        saveSettings();
        request->send(200, "application/json",
                      "{\"status\":\"received_credentials\"}");
        ESP.restart();
      });
  server.addHandler(handlerac);

  // run handleUpload function when any file is uploaded
  server.on(
      "/upload", HTTP_POST,
      [](AsyncWebServerRequest *request) { request->send(200); }, handleUpload);

  if (mSettings.wifi_enabled) {
    wifi_start();
    IPAddress myIP;
    SIpAddress = getIpAddress();
    Serial.println(SIpAddress);
  }
  Serial.println("Server started");
}

void loop(void) {
  app_stateengine();

  delay(100);
}
