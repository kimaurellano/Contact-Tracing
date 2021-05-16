#include <AltSoftSerial.h>

// AltSoftSerial always uses these pins:
//
// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10
// Arduino Mega      46        48       44, 45

#include <LiquidCrystal.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <TM1637Display.h>
#include <TM1637.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
const byte CLK = 15;   // define CLK pin (any digital pin)
const byte DIO = 14;   // define DIO pin (any digital pin)
int ir = 24;
int buzzer = 25;
int k;
int no = 8;
int yes = 7;
int led = 13;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
TM1637Display display(CLK, DIO);

AltSoftSerial altSerial;

boolean isCodeScanned = false;
boolean isBodyScanning = false;
String unoData;

void setup() {
  //****************************************Serials
  // For software communication
  Serial.begin(115200);
  // For arduino uno communication
  altSerial.begin(38400);

  //****************************************Pin setup
  pinMode(yes, INPUT);
  pinMode(no, INPUT);
  pinMode(ir, INPUT);

  //****************************************7-Segment
  display.setBrightness(7);    // set the brightness to 100 %

  //****************************************MLX
  mlx.begin();

  //****************************************
  delay(1000);
  display.clear();
}

char data[256];
int idx = 0;
boolean startRead = false;
void loop() {
  if (altSerial.available()) {
    // .read() required
    char c = altSerial.read();

    if ((byte)c == 13 || (byte)c == 10) {
      Serial.println("reading...");
      data[idx] = '\0';
      startRead = true;
      return;
    }

    data[idx++] = c;
  }

  if (startRead) {
    // Wait for software response
    Serial.println("Waiting for response...");
    while (true) {
      String appResponse = altSerial.readString();
      Serial.println(appResponse);
      if (appResponse == "Not Registered") {
        // Go back to scan wait
        isCodeScanned = false;
        return;
      } else if (appResponse == "Registered") {
        // Proceed to body scan
        break;
      }
    }

    while (true) {
      // naka-scan
      if (digitalRead(ir) == LOW) {
        double c = mlx.readObjectTempC() * 100;
        digitalWrite(led, HIGH);
        delay(1000);
        // Check for this tag in Serial
        Serial.println();
        Serial.print("TEMP:");
        Serial.print(c);
        Serial.println();
        display.showNumberDecEx(c, false, 0b01000000, 4, 0);
        altSerial.print("TEMP:");
        altSerial.print(c);
        delay(3000);

        // c:temperature
        if (c < 3800) {
          digitalWrite(led, LOW);
          cough();
        } else if (c >= 3730) {
          Serial.begin(9600);
          Serial.print("HIGH TEMPERATURE, SCAN AGAIN");
          digitalWrite(led, LOW);
          lcd.begin(20, 4);
          lcd.setCursor(2, 1);
          lcd.print("HIGH TEMPERATURE");
          lcd.setCursor(5, 2);
          lcd.print("SCAN AGAIN");
          //delay(1000);
          tone(buzzer, 1000);
          delay(1000);
          noTone(buzzer);
          display.clear();
          delay(1000);
          //sev();
        }
      }
    }

    startRead = !startRead;
  }
}

void cough() {
  while (digitalRead(yes) == HIGH && digitalRead(no) == HIGH) {
    displayPrompt("DO YOU HAVE COUGH?", "(UBO)", 1, 0, 8, 1);
  }

  display.clear();
  if (digitalRead(yes) == LOW) {
    displayResponse("COUGH", "YES");
    cold();
  }

  if (digitalRead(no) == LOW) {
    displayResponse("COUGH", "NO");
    cold();
  }
}

void cold() {
  while (digitalRead(yes) == HIGH && digitalRead(no) == HIGH) {
    displayPrompt("DO YOU HAVE COLD?", "(SIPON)", 2, 0, 6, 1);
  }

  display.clear();
  if (digitalRead(yes) == LOW) {
    displayResponse("COLD", "YES");
  }

  if (digitalRead(no) == LOW) {
    displayResponse("COLD", "NO");
    sev();
  }
}

void sev() {
  if (digitalRead(ir) == HIGH) {
    delay(1000);
    display.clear();

  }
  if (digitalRead(ir) == LOW) {
    delay(1000);
    display.clear();

  }
}

void displayResponse(String type, String response) {
  sev();
  delay(1000);
  lcd.clear();
  display.clear();
  // digitalWrite(10,HIGH);
  lcd.begin(20, 4);
  lcd.setCursor(9, 1);
  lcd.print(response);
  
  // Tag
  String inType = type + ":";
  altSerial.print(inType);
  altSerial.println(response);

  Serial.print(inType);
  Serial.println(response);
  
  delay(1000);
  sev();
}

void displayPrompt(String english, String tagalog, int row1x, int row1y, int row2x, int row2y) {
  sev();
  //delay(10000);
  //digitalWrite(13,HIGH);
  lcd.clear();
  lcd.begin(20, 4);
  // Print a message to the LCD.
  lcd.setCursor(row1x, row1y);
  lcd.print(english);
  lcd.setCursor(row2x, row2y);
  lcd.print(tagalog);
  delay(1000);
  sev();
}
