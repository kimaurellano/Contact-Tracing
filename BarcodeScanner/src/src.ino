#include <AltSoftSerial.h>

// AltSoftSerial always uses these pins:
//
// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10
// Arduino Mega      46        48       44, 45

#include <usbhid.h>
#include <hiduniversal.h>
#include <Usb.h>
#include <usbhub.h>
#include <hidboot.h>

AltSoftSerial altSerial;

USB Usb;
USBHub Hub(&Usb);
HIDUniversal Hid(&Usb);

class KbdRptParser : public KeyboardReportParser {
    void PrintKey(uint8_t mod, uint8_t key);
  protected:
    virtual void OnKeyDown  (uint8_t mod, uint8_t key);
    virtual void OnKeyPressed(uint8_t key);
};

KbdRptParser Prs;
boolean isCodeScanned = false;
boolean isBodyScanning = false;
boolean startRead = false;
char data[256];

// avoid missing first character
int idx = 1;

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key) {
  uint8_t c = OemToAscii(mod, key);

  if (c)
    OnKeyPressed(c);
}

void KbdRptParser::OnKeyPressed(uint8_t key) {
  if (key == 13) {
    data[idx] = '\0';
    idx = 0;
    isCodeScanned = true;
    Serial.println("Scanned");
  }
  data[idx++] = (char)key;
}

void setup() {
  // Need to be initialize first before altSoftSerial
  Usb.Init();

  Serial.begin(9600);
  altSerial.begin(38400);

  Hid.SetReportParser(0, (HIDReportParser*)&Prs);
}

void loop() {
  Usb.Task();

  if (isCodeScanned) {
    Serial.print("QR:");
    Serial.println(data);
    data[0] = '\0';
    idx = 0;

    altSerial.println("proceed");

    // Do not proceed until software response
    while (!Serial.available());
    String response = Serial.readString();
    Serial.println(response);

    // Send response to mega
    altSerial.print(response);

    isCodeScanned = !isCodeScanned;
  }

  // all data from mega will go directly to software
  if (altSerial.available() && !isCodeScanned) {
    Serial.print("string attempt:");
    Serial.println(altSerial.readString());
  }
}
