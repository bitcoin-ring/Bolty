#ifndef BOLT_H
#define BOLT_H


#include "gui.h"
#include <Adafruit_PN532_NTAG424.h>
#include <SPI.h>
#include <Wire.h>

#define JOBSTATUS_IDLE 0
#define JOBSTATUS_WAITING 1
#define JOBSTATUS_PROVISIONING 2
#define JOBSTATUS_WIPING 3
#define JOBSTATUS_DONE 4
#define JOBSTATUS_ERROR 5

String boltstatustext[6] = {
    "idle",          "waiting for nfc-tag..",  "provisioning data..",
    "wiping data..", "done - remove the card", "error",
};

struct sBoltConfig {
  char card_name[50];
  char wallet_host[50];
  char wallet_name[256];
  char wallet_url[256];
  char url[256];
  char uid[17];
  char k0[33];
  char k1[33];
  char k2[33];
  char k3[33];
  char k4[33];
};

String convertIntToHex(uint8_t *input, uint8_t len) {
  String ret = "";
  for (uint8_t i = 0; i < len; i++) {
    char hexChar[2];
    sprintf(hexChar, "%02X", input[i]);
    ret += hexChar;
  }
  return ret;
}

uint8_t convertCharToHex(char ch) {
  uint8_t returnType;
  switch (toupper(ch)) {
  case '0':
    returnType = 0;
    break;
  case '1':
    returnType = 1;
    break;
  case '2':
    returnType = 2;
    break;
  case '3':
    returnType = 3;
    break;
  case '4':
    returnType = 4;
    break;
  case '5':
    returnType = 5;
    break;
  case '6':
    returnType = 6;
    break;
  case '7':
    returnType = 7;
    break;
  case '8':
    returnType = 8;
    break;
  case '9':
    returnType = 9;
    break;
  case 'A':
    returnType = 10;
    break;
  case 'B':
    returnType = 11;
    break;
  case 'C':
    returnType = 12;
    break;
  case 'D':
    returnType = 13;
    break;
  case 'E':
    returnType = 14;
    break;
  case 'F':
    returnType = 15;
    break;
  default:
    returnType = 0;
    break;
  }
  return returnType;
}

class BoltDevice {

public: // Access specifier
  Adafruit_PN532 *nfc = NULL;
  uint8_t key_cur[5][16] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  uint8_t key_new[5][16] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  uint8_t job_status; // 0=idle; 1=wait for tag; 2=busy provisioning;3=busy wipe
  uint8_t job_perc;
  uint8_t job_ok;
  String last_scanned_uid;

  BoltDevice(uint8_t SCK, uint8_t MISO, uint8_t MOSI,
             uint8_t SS) { // Constructor
    nfc = new Adafruit_PN532(SCK, MISO, MOSI, SS);
  }

  void setDefautKeys(uint8_t keys[5][16]) {
    for (int i = 0; i < 5; i++) {
      memset((void *)(keys[i]), 0, 16);
    }
  }

  void setDefautKeysNew() { setDefautKeys(key_new); }
  void setDefautKeysCur() { setDefautKeys(key_cur); }

  void setKey(uint8_t keys[16], String key) {
    for (int i = 0; i < key.length(); i += 2) {
      uint8_t ki = (i / 2);
      uint8_t upper = (convertCharToHex(key[i]) << 4);
      uint8_t lower = (convertCharToHex(key[i + 1]));
      keys[ki] = (upper | lower);
    }
    // Serial.print("Key Set: ");
    // Serial.println(key);
  }

  void setNewKey(String key, uint8_t keyno) { setKey(key_new[keyno], key); }

  void setCurKey(String key, uint8_t keyno) { setKey(key_cur[keyno], key); }

  bool begin() {

    nfc->begin();

    uint32_t versiondata = nfc->getFirmwareVersion();
    if (!versiondata) {
      Serial.print("Didn't find PN53x board");
      return false;
        ; // halt
    }
    // Got ok data, print it out!
    Serial.print("Found chip PN53x");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    // configure board to read RFID tags
    nfc->SAMConfig();

    Serial.println("NFC Ready...");
    return true;
  }

  String get_job_status() { return boltstatustext[job_status]; }

  uint8_t get_job_perc() { return job_perc; }

  uint8_t get_job_status_id() { return job_status; }

  void set_job_status_id(uint8_t new_status) {
    job_status = new_status;
    uint16_t statcolor = APPBLACK;
    if (job_status == JOBSTATUS_ERROR) {
      statcolor = APPRED;

    } else if (job_status == JOBSTATUS_WIPING) {
      statcolor = APPORANGE;
    } else if (job_status == JOBSTATUS_PROVISIONING) {
      statcolor = APPORANGE;
    } else if (job_status == JOBSTATUS_DONE) {
      statcolor = APPGREEN;
    }
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(APPWHITE);
    tft.fillRect(0, -3 + (3 * 23), tft.width(), 21, statcolor);
    displayTextCentered(-3 + (4 * 21), get_job_status());
    tft.setTextColor(APPBLACK);
  }

  long lastscan = 0;

  bool scanUID() {
    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
    uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                       // card type)
    if ((millis() - lastscan) > 2000) {
      last_scanned_uid = "";
    };
    // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
    // the UID, and uidLength will indicate the size of the UUID (normally 7)
    success =
        nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
    if (success) {
      nfc->PrintHex(uid, uidLength);
      if (((uidLength == 7) || (uidLength == 4)) &&
          (nfc->ntag424_isNTAG424())) {
        lastscan = millis();
        last_scanned_uid = convertIntToHex(uid, uidLength);
        return true;
      }
    }
    return false;
  }

  String getScannedUid() { return last_scanned_uid; }

  uint8_t burn(String lnurl) {
    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
    uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                       // card type)
    job_status = JOBSTATUS_WAITING;
    set_job_status_id(JOBSTATUS_WAITING);
    // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
    // the UID, and uidLength will indicate the size of the UUID (normally 7)
    success =
        nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
    if (success) {
      set_job_status_id(JOBSTATUS_PROVISIONING);
      // Display some basic information about the card
      Serial.println("Found an ISO14443A tag");
      Serial.print("  UID Length: ");
      Serial.print(uidLength, DEC);
      Serial.println(" bytes");
      Serial.print("  UID Value: ");
      nfc->PrintHex(uid, uidLength);
      Serial.println("");

      //&& (nfc->ntag424_isNTAG424())
      if (((uidLength == 7) || (uidLength == 4)) &&
          (nfc->ntag424_isNTAG424())) {
        uint8_t filename[7] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};
        nfc->ntag424_ISOSelectFileByDFN(filename);
        int fileid = 0xe103;
        nfc->ntag424_ISOSelectFileById(fileid);
        fileid = 0xe104;
        nfc->ntag424_ISOSelectFileById(fileid);
        uint8_t uriIdentifier = 0;
        uint8_t piccDataOffset = lnurl.length() + 10;
        uint8_t sdmMacOffset = lnurl.length() + 45;
        lnurl += "?p=00000000000000000000000000000000&c=0000000000000000";
        int len = lnurl.length();
		if (len > 0xff){
			set_job_status_id(JOBSTATUS_ERROR);
			Serial.println("lnurl cannot be longer than 256");
			return job_status;
		}
        uint8_t ndefheader[7] = {
            0x0,     /* Tag Field (0x03 = NDEF Message) */
            (uint8_t) (len + 5), /* Payload Length (not including 0xFE trailer) */
            0xD1, /* NDEF Record Header (TNF=0x1:Well known record + SR + ME +
                     MB) */
            0x01, /* Type Length for the record type indicator */
            (uint8_t) (len + 1), /* Payload len */
            0x55,         /* Record Type Indicator (0x55 or 'U' = URI Record) */
            uriIdentifier /* URI Prefix (ex. 0x01 = "http://www.") */
        };
        uint8_t *filedata = (uint8_t *)malloc(len + sizeof(ndefheader));
        memcpy(filedata, ndefheader, sizeof(ndefheader));
        memcpy(filedata + sizeof(ndefheader), lnurl.c_str(), lnurl.length());
        nfc->ntag424_ISOUpdateBinary(filedata, len + sizeof(ndefheader));
        free(filedata);
        uint8_t keyno = 0;
        uint8_t authenticated =
            nfc->ntag424_Authenticate(key_cur[keyno], keyno, 0x71);

        if (authenticated == 1) {
          Serial.println("Authentication successful.");
          Serial.println("Enable Mirroring and SDM.");
          // int piccDataOffset = 81;
          // int sdmMacOffset = 116;
          uint8_t fileSettings[] = {0x40,
                                    0x00,
                                    0xE0,
                                    0xC1,
                                    0xFF,
                                    0x12,
                                    (uint8_t) (piccDataOffset & 0xff),
                                    (uint8_t) ((piccDataOffset >> 8) & 0xff),
                                    (uint8_t) ((piccDataOffset >> 16) & 0xff),
                                    (uint8_t) (sdmMacOffset & 0xff),
                                    (uint8_t) ((sdmMacOffset >> 8) & 0xff),
                                    (uint8_t) ((sdmMacOffset >> 16) & 0xff),
                                    (uint8_t) (sdmMacOffset & 0xff),
                                    (uint8_t) ((sdmMacOffset >> 8) & 0xff),
                                    (uint8_t) ((sdmMacOffset >> 16) & 0xff)
                                    };
          nfc->ntag424_ChangeFileSettings((uint8_t)2, fileSettings,
                                          (uint8_t)sizeof(fileSettings),
                                          (uint8_t)NTAG424_COMM_MODE_FULL);
          for (int i = 0; i < 5; i++) {
            success &=
                nfc->ntag424_ChangeKey(key_cur[4 - i], key_new[4 - i], 4 - i);
            if (!success) {
              Serial.print("ChangeKey error! Key: ");
              Serial.println(i);
              set_job_status_id(JOBSTATUS_ERROR);
            }
          }
        } else {
          Serial.println("Authentication 1 failed.");
          set_job_status_id(JOBSTATUS_ERROR);
          // return false;
        }
        authenticated = 0;
        authenticated = nfc->ntag424_Authenticate(key_new[4], 4, 0x71);
        // Display the current page number
        Serial.print("Response ");
        // Display the results, depending on 'success'
        if (authenticated == 1) {
          Serial.println("Authentication 2 Success.");
          set_job_status_id(JOBSTATUS_DONE);
        } else {
          Serial.println("Authentication 2 failed.");
          set_job_status_id(JOBSTATUS_ERROR);
        }
      } else {
        Serial.println("This doesn't seem to be an NTAG424 tag. (UUID length "
                       "!= 7 bytes and UUID length != 4)!");
      }
    }
    return job_status;
  }

  uint8_t wipe() {
    uint8_t success;
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
    uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                       // card type)

    // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
    // the UID, and uidLength will indicate the size of the UUID (normally 7)

    set_job_status_id(JOBSTATUS_WAITING);
    // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
    // the UID, and uidLength will indicate the size of the UUID (normally 7)
    success =
        nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
    if (success) {
      set_job_status_id(JOBSTATUS_WIPING);
      // Display some basic information about the card
      Serial.println("Found an ISO14443A tag");
      Serial.print("  UID Length: ");
      Serial.print(uidLength, DEC);
      Serial.println(" bytes");
      Serial.print("  UID Value: ");
      nfc->PrintHex(uid, uidLength);
      Serial.println("");

      //&& (nfc->ntag424_isNTAG424())
      if (((uidLength == 7) || (uidLength == 4)) &&
          (nfc->ntag424_isNTAG424())) {
        uint8_t filename[7] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};
        nfc->ntag424_ISOSelectFileByDFN(filename);
        int fileid = 0xe103;
        nfc->ntag424_ISOSelectFileById(fileid);
        fileid = 0xe104;
        nfc->ntag424_ISOSelectFileById(fileid);
        // Figure out how long the string is
        uint8_t keyno = 0;
        uint8_t authenticated =
            nfc->ntag424_Authenticate(key_cur[keyno], keyno, 0x71);

        // Display the current page number
        Serial.print("Response ");
        // Display the results, depending on 'success'
        if (authenticated == 1) {
          Serial.println("Authentication successful.");
          Serial.println("Disable Mirroring and SDM.");

          uint8_t fileSettings[] = {0x00, 0xE0, 0xEE};

          nfc->ntag424_ChangeFileSettings((uint8_t)2, fileSettings,
                                          (uint8_t)sizeof(fileSettings),
                                          (uint8_t)NTAG424_COMM_MODE_FULL);

          for (int i = 0; i < 5; i++) {
            success &=
                nfc->ntag424_ChangeKey(key_cur[4 - i], key_new[4 - i], 4 - i);
            if (!success) {
              Serial.print("ChangeKey error! Key: ");
              Serial.println(i);
              job_status = JOBSTATUS_ERROR;
            }
          }

          uint8_t filename[7] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};
          nfc->ntag424_ISOSelectFileByDFN(filename);
          int fileid = 0xe103;
          nfc->ntag424_ISOSelectFileById(fileid);
          fileid = 0xe104;
          nfc->ntag424_ISOSelectFileById(fileid);

          if (nfc->ntag424_FormatNDEF()) {
            job_perc = 100;
          }
        } else {
          Serial.println("Authentication failed.");
          success = false;
          set_job_status_id(JOBSTATUS_ERROR);
        }
        // try authenticating with the new key
        authenticated = 0;
        authenticated = nfc->ntag424_Authenticate(key_new[4], 4, 0x71);
        if (authenticated == 1) {
          Serial.println("Authentication 2 Success.");
          set_job_status_id(JOBSTATUS_DONE);
        } else {
          Serial.println("Authentication 2 failed.");
          set_job_status_id(JOBSTATUS_ERROR);
        }
      } else {
        Serial.println("This doesn't seem to be an NTAG424 tag. (UUID length "
                       "!= 7 bytes and UUID length != 4)!");
      }
    }
    return job_status;
  }
};
#endif
