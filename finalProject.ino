#include <ESP32Servo.h>
#define DEBUG_LOCK 0
#define WRITE_RFID 34
#define SERVO1 23
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 lcd(128, 64);
Servo frank;

//RFID stuff
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         27           // Configurable, see typical pin layout above
#define SS_PIN          SDA          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

void setup() {
  // put your setup code here, to run once:
  pinMode(DEBUG_LOCK, INPUT);
  pinMode(WRITE_RFID, INPUT);
  frank.attach(SERVO1);
  //set servo to 90 degrees for start
  frank.write(90);
  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  lcd.setTextColor(WHITE); 
  lcd.clearDisplay();
  lcd.setCursor(0,0);
  lcd.print("Unlocked");
  lcd.display(); 

  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
}

boolean locked = false;

void updateLock() {
  if(locked){
    frank.write(180);
    lcd.clearDisplay();
    lcd.setCursor(0,0);
    lcd.print("Locked");
    lcd.display(); 
    delay(100);//smoothness
  }else{
    frank.write(90);
    lcd.clearDisplay();
    lcd.setCursor(0,0);
    lcd.print("Unlocked");
    lcd.display();
    delay(100);//smoothness
  }
}

byte blockAddr    = 4;
byte trailerBlock = 7;
byte password[]   = {
        0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41,
    };
 

// Check for compatibility
int compatibility_check() {
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        return -1;
    }
    return 0;
}

/// In charge of handling RFID interactions
void handle_rfid() {
    byte buffer[16];
    unsigned int check = 0;
    byte size = 16;

    // If rfid card is not compatible, stop trying to interact
    if (compatibility_check() != 0) { return; }

    // Read data from RFID tag and make sure read was successful
    if ((MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size) != MFRC522::STATUS_OK) {
        return;
    }

    // Check that the password is correct
    for (int i = 0; i < size; i++) {
        check += buffer[i] ^ password[i];
    }

    // Passed password-check, unlock lock
    if (check == 0) {
        locked = false;
    }
}

// Write data to the block and check that the write succeeded
void write_rfid() {
    if ((MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, password, 16) != MFRC522::STATUS_OK) {
        return;
    }
}

/// Main loop
void loop() {
    // RFID chip in use
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) { 
        handle_rfid();
    }

    // Debugging feature to lock/unlock using button without requiring any authentification
    if (digitalRead(DEBUG_LOCK) == LOW) {
        delay(50);
        locked = !locked;
    }

    // Initialize RFID Chips that can then be used to authenticate
    if (digitalRead(WRITE_RFID) == LOW) {
        delay(50);
        write_rfid();
    }

    updateLock();
}
