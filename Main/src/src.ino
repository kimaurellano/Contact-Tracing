#include <AltSoftSerial.h>

// AltSoftSerial always uses these pins:
//
// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10
// Arduino Mega      46        48       44, 45

#include <TM1637Display.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <TM1637.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#define CLK  15
#define DIO  14
int ir = 24;
int buzzer = 25;
int k;
int no = 8;
int yes = 7;
int led = 13;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
TM1637Display display = TM1637Display(CLK, DIO);

AltSoftSerial altSerial;

boolean isCodeScanned = false;
boolean isBodyScanning = false;
String unoData;

void setup() {
  // For software communication
  Serial.begin(9600);
  // For arduino uno communication
  altSerial.begin(38400);
  
  // put your setup code here, to run once:
  pinMode(yes, INPUT);
  pinMode(no, INPUT);
  pinMode(ir, INPUT);
  display.setBrightness(7);
  mlx.begin();
  display.clear();
  delay(1000);

  // Signal the software of a successful connection
  delay(2500);
  Serial.print("start");
}

char data[256];
int idx = 0;
boolean startReading = false;
void loop() {
  if (altSerial.available()) {
    char c = altSerial.read();
    if(c == 'Q'){
      startReading = true;
      data[idx++] = c;
    }

    if(startReading){
      data[idx++] = c;
      if(c == '\n'){
        data[idx] = '\0';
        Serial.println(data);
        idx = 0;
        memset(data, 0, sizeof data);
        startReading = false;
      }
    }
  }
  
  if(isCodeScanned && !isBodyScanning) {
    Serial.println("[1]");
    // Send to software to parse
    // QR:FirstName,LastName,Purok,ContactNumber,Address,EContact,EName
    altSerial.print("QR:");
    altSerial.println(unoData);

    // BUG: not proceeding
    // Wait for software response
    while (true){
      String appResponse = Serial.readString();
      if(appResponse == "Not Registered"){
        // Go back to scan wait
        isCodeScanned = false;
        return;
      } else if(appResponse == "Registered") {
        // Proceed to body scan
        break;
      }
    }

    // Allow back to start but then immediately proceed to body scan
    isBodyScanning = true;
    isCodeScanned = false;
    return;

  } else if(isBodyScanning && !isCodeScanned) {
    Serial.println("[2]");
    // naka-scan
    if (digitalRead(ir) == LOW) {
      double c = mlx.readObjectTempC()*100;
      digitalWrite(led, HIGH);
      delay(1000);
      // Check for this tag in Serial
      Serial.println();
      Serial.print("TEMP:");
      Serial.print(c);
      Serial.println();
      display.showNumberDecEx(c, false, 0b01000000, 4, 0);
      delay(100);
  
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
    if (digitalRead(ir) == HIGH) {
      //cough();
      //delay(500);
      display.clear();
      lcd.clear();
      digitalWrite(led, LOW);
    }
    
    // Allow back to start
    isCodeScanned = false;
    isBodyScanning = false;
  }
}

void cough() {
  while (digitalRead(yes) == HIGH && digitalRead(no) == HIGH) {
    displayPrompt("DO YOU HAVE COUGH?", "(UBO)", 1, 0, 8, 1);
  }

  display.clear();
  // Tag for cough
  Serial.print("COUGH:");
  // Low means yes
  if (digitalRead(yes) == LOW) {
    displayResponse("COUGH","YES");
    cold();
  }

  if (digitalRead(no) == LOW) {
    displayResponse("COUGH","NO");
    cold();
  }
}

void cold() {
  while (digitalRead(yes) == HIGH && digitalRead(no) == HIGH) {
    displayPrompt("DO YOU HAVE COLD?", "(SIPON)", 2, 0, 6, 1);
  }

  display.clear();
  if (digitalRead(yes) == LOW) {
    displayResponse("COLD","YES");
  }

  if (digitalRead(no) == LOW) {
    displayResponse("COLD","NO");
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
  Serial.println(type + ":" + response);
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
