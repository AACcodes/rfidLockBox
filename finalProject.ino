#include <ESP32Servo.h>
#define BUTTON 0
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

MFRC522::MIFARE_Key key;

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON, INPUT);
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

  // Prepare the key (used both as key A and as key B
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }
}


boolean locked = false;

void updateLock(){
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

byte blockAddr      = 4;
byte dataBlock[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
byte trailerBlock   = 7;
 

void writeRFID(){
   mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  
}

bool authRFID(){
  mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
}
void loop() {
  // put your main code here, to run repeatedly:

//Button locking feature
  
  if(digitalRead(BUTTON) == LOW){
    delay(50);//debounce
    if(locked){
      locked = false;
    }else{
      locked = true;
    }
  }


 // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        //Serial.println(F("This sample only works with MIFARE Classic cards."));
        return;
    }


  updateLock();
  
}
