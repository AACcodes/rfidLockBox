//RFID stuff
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

//servo stuff
#include <Servo.h>
Servo servo;


//const int togglePin = 2; 

boolean locked = false;
char input;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Initializing system");

  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }
 
  // initialize the pushbutton pin as an input:
  //pinMode(togglePin, INPUT);

  //connect servo
  servo.attach(5);
  servo.write(90);

  Serial.println("Done initializing... ready to scan");

}

void updateLock() {
    if (locked) {
        servo.write(180);
        delay(100);
    } else {
        servo.write(90);
        delay(100);
    }
}


//RFID globals
byte sector         = 1;
byte blockAddr      = 4;
byte dataBlock[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
byte trailerBlock   = 7;
MFRC522::StatusCode status;
byte buffer[18];
byte size = sizeof(buffer);



void readRFID(){
  //auth using A
  // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
  // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    locked = !locked;
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    Serial.println("Read done");
}

void loop() {
  // put your main code here, to run repeatedly:

  //manual override with serial typing
  while(Serial.available()){
        input = Serial.read();
        Serial.print("You typed: " );
        Serial.println(input);
        locked = !locked;
    }

  // RFID chip in use
   if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        readRFID();
        delay(600);
   }
  /* Button code
  int buttonState = digitalRead(togglePin);
  if (buttonState == HIGH) {
    // turn LED on:
    Serial.println("Toggle");
    locked = !locked;
    delay(100);
  } 
  */
  // Added the delay so that we can see the output of button
  updateLock();
  
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
