//RFID stuff
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Keypad.h>

#define CONST1 0x12345600
#define CONST2 0x42434400
#define NUMROWS 2

<<<<<<< HEAD
// RFID globals
=======
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
>>>>>>> 12303b6ec43ace6e261a39017e6a51a321b4145d
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
byte blockAddr      = 4;
byte trailerBlock   = 7;
byte buffer[18];
byte size = sizeof(buffer);

// Keypad Globals
const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 3; //three columns
byte pin_rows[ROW_NUM] = {8, 7, 6,4}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {3, 2, SDA}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );;

char keys[ROW_NUM][COLUMN_NUM] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
};

// Other globals
Servo servo;
boolean locked = false;
boolean writeCard = false;
String input;
String password = "stupidESP";

// Password used to unlock the box
char hashed_key[]   = {
        0x4b, 0xa6, 0x52, 0xd5, 0x91, 0x49, 0x32, 0x95,
        0x52, 0xbf, 0x41, 0xc6, 0x93, 0x83, 0xfa, 0x0d,
    };

// Original key, needed to write rfid tags
char orig_key[]   = {
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
    };

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing system");

<<<<<<< HEAD
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    // Prepare Key A using 0xFFFFFFFFFFFF which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
 
    // Connect the servo-motor
    servo.attach(5);
    servo.write(90);
=======
  //connect servo
  servo.attach(5);
  servo.write(90);
  keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );
  Serial.println("Done initializing... ready to scan");
>>>>>>> 12303b6ec43ace6e261a39017e6a51a321b4145d

    // Initialize keypad
    keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

    Serial.println("Done initializing... ready to scan");
}

void updateLock() {
    if (locked) {
        servo.write(180);
    } else {
        servo.write(90);
    }
}

/// Compute a quick non-cryptographically secure hash for the passwords. Since no knowledge about
/// the correct password is exposed to the attacker, hash collisions are no concern.
void hash_password() {
    unsigned long hash;
    char tmp;
    
    for (int i = 0; i < NUMROWS; i++) {
        hash = 0;
        for (int j = 0; j < 8; j++) {
            hash += (buffer[(i * 8) + j] << (8 * j));
        }
        hash ^= hash << 13;
        hash ^= hash >> 17;
        hash ^= hash << 43;
        for (int j = 0; j < 8; j++) {
            buffer[(i * 8) + j] = ((hash >> (j * 8)) & 0xff);
        }
    }
}
 
/// This is basically a way-complicated memcmp to greatly mitigate a class of hardware based attacks
/// called sidechannels. 2 popular approaches attacks are power analysis and fault injections. 
///
/// PA attacks are performed by capturing power traces of the program during execution using an
/// oscilloscope. These are then analyzed (either manually or through statistical/ml-based analysis)
/// to learn about properties of the underlying code. This information can then be used to determine
/// cryptographical keys or in the case of a memcmp, the password that is being compared against.
/// This attack is extremely applicable in practice with very low-cost equipment, and is thus worth
/// mitigating against.
///
/// Fault injections rely on glitching the device during execution to make the processor skip
/// instructions. There are multiple ways to achieving this depending on the resources available to 
/// the attacker. The simplest method would be to simply cut power to the device at a critical point
/// during execution and then re-enabling it nano-seconds later. This can be used to trivially
/// bypass a simple 'if (key == 0xdeadbeef)' type check. More complex/expensive methods can include
/// electromagnetic pulses, lighting gates up with a sufficient photon intensity, or even lasers.
///
/// The below implementation uses several generally suggsted mitigations against these attacks as
/// described below.
///
/// 1. The entire operation is performed on set-size hashes of the password. This makes it harder
///    to learn anything of relevance about the actual password using power analysis.
/// 2. Additionally the function is extended with non-trivial values for important state. This is
///    done by preventing diff or tmpdiff to ever be all 1's or all 0's since these can often be
///    easily forced using fault injections. The lowest byte is set to 0 and used to accumulate the
///    difference into, which will result in a return of (CONST1 | CONST2). If some sidechannel
///    attack manages to flip a bit in any of the tmpdiff bits, this will be detected.
/// 3. While this code is not entirely branchless, the branch-operations are entirely duplicated
///    using decoy-inputs, which means that a power-trace should have almost the exact same result.
///    There exists a tiny difference between branch taken or branch not taken, but this is 
///    considerably harder to detect & interpret than the larger change of not duplicating code.
/// 4. On every iteration of the loop there is a 50% chance that we will execute a decoy operation
///    instead of the actual operation. This adds an extra hurdle for the attacker of having to
///    identify if their powertrace is currently in a decoy branch or in the operational.
/// 5. The operations are all done in a time-constant manner (apart from the intentional
///    randomization described above), which means that an attacker cannot easily identify the
///    password due to an early termination in the power-trace once an incorrect character is hit.
/// 6. Whenever the password is validated, we are looping through the arrays in a random order. This
///    makes it hard for an attacker to interpret the traces because they will differ greatly on
///    each run even without accounting the other mitigations.
/// 7. Finally a 'check' variable is maintained throughout the operations. If any important
///    operation is faulted during execution, the check variable has a high chance of detecting
///    this to stop any potential fault attacks.
///
/// Usually "security through obscurity" is frowned upon, however in the case of sidechannel
/// attacks, where the attacker has to often rely solely on power traces, introducing a lot of
/// additional noise and minimizing the noise generated by important operations can make it 
/// extremely hard (impossible without expensive equipment) for an attacker to make any progress on
/// a target.
int validate_pw(int n) {
    char decoy1[16];
    char decoy2[16];
    volatile int idx, decoy, tmpdiff, is_decoy;
    volatile int diff = 0;
    volatile int i = 0;
    volatile int rnd = rand() % (n - 1);
    volatile int check = 3;

    // Loop through the input in a random order
    while (i < n) {
        // Get a random index, wrap around once necessary
        idx = (i + rnd) % n;

        // Random chance to execute decoy operations instead of actual operation
        is_decoy = (rand() & 1);
        if (is_decoy) {
            check++;
            decoy   = ((CONST1 | decoy1[idx]) ^ (CONST2 | decoy2[idx]));
            tmpdiff = (CONST1 | CONST2);
        } else {
            check++;
            tmpdiff = ((CONST1 | buffer[idx]) ^ (CONST2 | hashed_key[idx]));
            decoy   = (CONST1 | CONST2);
        }

        // Accumulate the actual difference
        diff = diff | tmpdiff;

        // Increment counter if this was not a decoy operation
        i += (is_decoy ^ 1);

        check--;
    }

    // Verify that the function executed correctly
    return check == 3 ? diff : 0;
}

void authenticate() {
    // Initialize to non-zero so the authentication call can't trivially be bypassed by glitching
    // the function call instruction
    volatile int result = 7;
    volatile int result2 = 3;
    volatile int wait = 0;
    volatile int check;

    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
        trailerBlock, &key, &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }

    // Read data from the block
    Serial.print(F("Reading data from block ")); 
    Serial.print(blockAddr);
    Serial.println(F(" ..."));

    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return;
    }

    hash_password();

    // Password validation routine. 
    {
        result = validate_pw(16);
        if (result == (CONST1 | CONST2)) {
            wait = rand() % 10;
            for (int i = 0; i < wait; i++) {
                check = check + 1;
            }

            // Verify that no fault has occured thus far
            if (check == wait) {
                // Recompute the result
                result2 = validate_pw(16);

                // Double check result, this time with a different logic that is slightly different
                // from the previous one
                if ((result2 & 0xff) == 0x0) {
                    locked = !locked;
                }
            }
        }
    }

    Serial.print(F("Data in block ")); 
    Serial.print(blockAddr); 
    Serial.println(F(":"));
    dump_byte_array(buffer, 16); 
    Serial.println();
    Serial.println();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    Serial.println("Read done");
}

void programCard() {
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
        trailerBlock, &key, &(mfrc522.uid));

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
   dump_byte_array(orig_key, 16); Serial.println();
   status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, orig_key, 16);
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

void loop() {
  // Manual override to unlock without key
  while (Serial.available()) {
        input = Serial.readStringUntil('\n');
        Serial.print("You typed: ");
        Serial.println(input);
        if (input == password) {
            Serial.println("Auth correct. Lock toggled");
            locked = !locked;
        } else if (input == "write") {
            Serial.println("THE NEXT CARD YOU PRESENT WILL BE WRITTEN TO");
            writeCard = true;
        }
    }
    char key = keypad.getKey();

    if (key){
      Serial.println(key);
    }

    char key = keypad.getKey();

    if (key) {
        Serial.println(key);
    }

    // RFID chip in use
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        if (writeCard) {
            writeCard = false;
            programCard();
        } else {
            authenticate();
        }
        // Delay to prevant too fast rescan
        delay(800);
    }
    updateLock();
}

/// Helper routine to dump a byte array as hex values to Serial.
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
