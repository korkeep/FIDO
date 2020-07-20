
#include <SArduino.h>
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11);
unsigned char buf[256];
BYTE output[1024];
size_t len;
int fingerID;
uint8_t tempCounter;
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

/////
int Button[4] = {21, 20, 19, 18}; // Button Pin 정의 (Interrupt 2, 3, 4, 5)
/////

short getShort(byte bArray[], short bOff) {
  byte a = bArray[(short)bOff];
  byte b = bArray[(short)(bOff + 1)];

  return (short)(a + b * 256);
}

short setShort(byte bArray[], short bOff, short sValue) {
  bArray[(short)bOff] = (byte)(sValue & 0x00ff);
  bArray[(short)(bOff + 1)] = (byte)((sValue & 0xff00) >> 8);
  return (short)(bOff + 2);
}

// buf에 저장된 데이터를 시리얼 모니터로 출력하는 함수
void dump( byte* buf, int len ) {
  int i;
  for ( i = 0; i < len; i++ ) {
    Serial.print( (char)buf[ i ] );
  }
  Serial.println();
}

/////
// Interrupt Control 함수
void Interrupt_Control_2() {
  Serial.println("@1");
  Serial2.write("@1");
}
void Interrupt_Control_3() {
  Serial.println("@2");
  Serial2.write("@2");
}
void Interrupt_Control_4() {
  Serial.println("@3");
  Serial2.write("@3");
}
void Interrupt_Control_5() {
  Serial.println("@4");
  Serial2.write("@4");
}
/////
void setup() {
  Serial2.begin(9600, SERIAL_8E2);
  // 시리얼 모니터 사용
  Serial.begin(9600, SERIAL_8E2);
  // SE 초기화
  if ( !Init_SE() ) {
    Serial.println("SE Connection Failure");
  }
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  finger.getTemplateCount();
  tempCounter = finger.templateCount;

  /////
  // Button PIN을 입력으로 설정
  for (int i = 0; i < 4; i++)
    pinMode(Button[i], INPUT_PULLUP);
  // Interrupt 설정
  attachInterrupt(2, Interrupt_Control_2, RISING);
  attachInterrupt(3, Interrupt_Control_3, RISING);
  attachInterrupt(4, Interrupt_Control_4, RISING);
  attachInterrupt(5, Interrupt_Control_5, RISING);
  /////
}
void loop() {
  int i;
  int tmp_id;
  int cmd;
  // put your main code here, to run repeatedly:
  int testbio = 0;
  int len_data;
  int output_len;
  if (Serial2.available()) {
    len = Serial2.readBytes(buf, 254);
    for (int i = 0; i < len; i++) {
      Serial.print(buf[i], HEX); Serial.print(" ");
    }
    Serial.println();

    if ((buf[0] == 0x04 || buf[0] == 0x03 || buf[0] == 0x02 || buf[0] == 0x01) && buf[1] == 0x34) { // Register CMD
      cmd = buf[0];
      Serial.println("ipjang");
      Serial.println(cmd);
      switch (cmd) {
        case 0x1:
          if (FIDO_UAF(buf, len, output, &output_len)) {
            Serial.println("Deleting All FingerPrint");
            finger.emptyDatabase();
            for (i = 0; i < output_len; i++) {
              Serial2.write(output[i]);
            }
          }
          break;
        case 0x2: // Register
          tmp_id = getFingerprintID();
          if (tmp_id != -1) {
            Serial.println(tmp_id);
            Serial2.write("Error: FingerPrint Exists");
            break;
          }
          Serial.println("Getting");
          fingerID = getNotStoredID();
          Serial.println("NOW");
          if (fingerID == -1) {
            Serial2.write("Error: Not enough storage");
            break;
          }
          Serial.print("Finger Not Stored: "); Serial.println(fingerID);
          if (getFingerprintEnroll(fingerID) != FINGERPRINT_OK) {
            Serial2.write("Error: FingerPrint Not Match");
            break;
          }
          Serial.println(len);
          for (i = 0; i < len; i++) {
            Serial.print(" "); Serial.print(buf[i], HEX);
          }
          Serial.println();
          len = setShort(buf, len, 0x4E01);
          len = setShort(buf, len, 1);
          buf[len] = fingerID;
          len++;
          len_data = getShort(buf, 2);
          len_data += 5;
          setShort(buf, 2, len_data);
          Serial.println(len);
          for (i = 0; i < len; i++) {
            Serial.print(" "); Serial.print(buf[i], HEX);
          }
          Serial.println();
          if (FIDO_UAF(buf, len, output, &output_len)) {
            for (i = 0; i < output_len; i++) {
              Serial2.write(output[i]);
            }
          } else {
            Serial2.write("Error: No Such User");
          }
          break;
        case 0x3:
          Serial.println("Authenticate");
          fingerID = getFingerprintID();
          if (fingerID == -1) {
            Serial2.write("Error: No Such Finger");
            break;
          }
          Serial.println(len);
          for (i = 0; i < len; i++) {
            Serial.print(" "); Serial.print(buf[i], HEX);
          }
          Serial.println();
          len = setShort(buf, len, 0x4E01);
          len = setShort(buf, len, 1);
          buf[len] = fingerID;
          len++;
          len_data = getShort(buf, 2);
          len_data += 5;
          setShort(buf, 2, len_data);
          Serial.println(len);
          for (i = 0; i < len; i++) {
            Serial.print(" "); Serial.print(buf[i], HEX);
          }
          Serial.println();
          if (FIDO_UAF(buf, len, output, &output_len)) {
            for (i = 0; i < output_len; i++) {
              Serial2.write(output[i]);
            }
          } else {
            Serial2.write("Error: No Such User");
          }
          break;
        case 0x4:
          Serial.println(len);
          for (i = 0; i < len; i++) {
            Serial.print(" "); Serial.print(buf[i], HEX);
          }
          Serial.println();
          if (FIDO_UAF(buf, len, output, &output_len)) {
            for (i = 0; i < output_len; i++) {
              Serial2.write(output[i]);
            }
            deleteFingerprint(fingerID);

          } else {
            Serial2.write("Error: No Such User");
          }
          break;
      }
      if ( !Init_SE() ) {
        Serial.println("SE Connection Failure");
      }

    }
  }

  if (Serial.available()) {
    Serial.readBytes(buf, 254);
    Serial2.write((char *)buf);
  }

}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}
int getNotStoredID() {
  int i;
  for (i = 1; i < 15; i++) {
    if ( finger.loadModel(i) != FINGERPRINT_OK ) break;
  }
  if (i == 15) {
    return -1;
  }
  Serial.println(i);
  return i;
}

int getFingerprintID() {
  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return -1;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return -1;
  } else {
    Serial.println("Unknown error");
    return -1;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  delay(500);
  // Serial.print("Found ID #"); Serial.print(finger.fingerID);
  //Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

uint8_t getFingerprintEnroll(uint8_t id2) {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id2);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id2);
  p = -1;
  delay(500);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        delay(500);
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      delay(500);
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  delay(500);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id2);
  p = finger.storeModel(id2);
  if (p == FINGERPRINT_OK) {
    delay(500);
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
}
