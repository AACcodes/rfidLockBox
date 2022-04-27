#include <ESP32Servo.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>

#define DEBUG_LOCK 0
#define WRITE_RFID 34
#define SERVO1 23
#define RST_PIN 27
#define SS_PIN SDA

MFRC522 mfrc522(SS_PIN, RST_PIN);
Adafruit_SSD1306 lcd(128, 64);
Servo servo;
boolean locked = false;

void setup() {
    // Initialize RFID interface
    Serial.begin(9600); // Initialize serial communications with the PC
    while (!Serial);    // Do nothing if no serial port is opened
    SPI.begin();
    mfrc522.PCD_Init();

    // Setup buttons
    pinMode(DEBUG_LOCK, INPUT);
    pinMode(WRITE_RFID, INPUT);

    // Initialize Servo to 90 degrees
    servo.attach(SERVO1);
    servo.write(90);

    Serial.println("Initializing Devive");
}


// Update the lock based on the `locked` variable
void updateLock() {
    if (locked) {
        servo.write(180);
        lcd.clearDisplay();
        lcd.setCursor(0,0);
        lcd.print("Locked");
        lcd.display(); 
        delay(100);
    } else {
        servo.write(90);
        lcd.clearDisplay();
        lcd.setCursor(0,0);
        lcd.print("Unlocked");
        lcd.display();
        delay(100);
    }
}

// Region in which the password is stored on the rfid chip
byte blockAddr = 4;

// Password used to unlock the box
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

    Serial.println("HANDLE_RFID hit");

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
