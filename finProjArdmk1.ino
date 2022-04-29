//RFID stuff
#include <SPI.h>
#include <MFRC522.h>

//keypad stuff
#include <Keypad.h>
const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 3; //three columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte pin_rows[ROW_NUM] = {8, 7, 6,4}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {3, 2, SDA}; //connect to the column pinouts of the keypad


Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );;


//rfid stuff again
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

//servo stuff
#include <Servo.h>
Servo servo;


//const int togglePin = 2; 

boolean locked = false;
String input;
String password = "stupidESP";
boolean writeCard = false;
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
  keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
  Serial.println("Done initializing... ready to scan");

}

void updateLock() {
    if (locked) {
        servo.write(180);
        //delay(100);
    } else {
        servo.write(90);
        //delay(100);
    }
}


//RFID globals
byte sector         = 1;
byte blockAddr      = 4;

byte trailerBlock   = 7;
MFRC522::StatusCode status;
byte buffer[18];
byte size = sizeof(buffer);

// Password used to unlock the box
byte boxKey[]   = {
        0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41,
        0x41, 0x41, 0x41, 0x41,
    };
 


void readRFID(){
  //auth using A
  // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
  // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }
    //locked = !locked;
    auth();
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    Serial.println("Read done");
}

void programCard(){
  //auth using A
  // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }

   //write data to block
   Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
   Serial.println(F(" ..."));
   dump_byte_array(boxKey, 16); Serial.println();
   status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, boxKey, 16);
   if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
   }
   Serial.println();
   mfrc522.PICC_HaltA();
   mfrc522.PCD_StopCrypto1();
   Serial.println("Write done");

  
}

void auth(){
  int count = 0;
    for (int i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == boxKey[i])
            count++;
    }
    if(count == 16){
      Serial.println("Auth correct");
      locked = !locked;
    }else{
      Serial.println("Auth incorrect");
    }
  
}

void loop() {
  // put your main code here, to run repeatedly:

  //manual override with serial typing
  while(Serial.available()){
        input = Serial.readStringUntil('\n');
        Serial.print("You typed: " );
        Serial.println(input);
        if(input == password){
          Serial.println("Auth correct. Lock toggled");
          locked = !locked;
        }else if(input == "write"){
          Serial.println("THE NEXT CARD YOU PRESENT WILL BE WRITTEN TO");
          writeCard = true;
        }
    }
    char key = keypad.getKey();

    if (key){
      Serial.println(key);
    }

  // RFID chip in use
   if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        if(writeCard){
          writeCard = false;
          programCard();
        }else{
        readRFID();
        }
        delay(800);
        //Delay to prevant too fast rescan
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
