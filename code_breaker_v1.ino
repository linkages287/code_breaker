//|---|----------------|----------------|                                                                            
//|sec|key A           |key B           |                                                                            
//|---|----------------|----------------|                                                                            
//|000|  a0a1a2a3a4a5  |  b4c132439eef  |                                                                            
//|001|  6b68f62aeb9f  |  5d23e629742e  |                                                                            
//|002|  c90f6538f055  |  dfe8fb849e4e  | READ KEY          
//|003|  8008eda04e73  |  0642710cfaa1  |          
//|004|  53001b2026ea  |  1c1c455d81f2  |          
//|---|----------------|----------------|   



#include <SPI.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
#include <string.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define KEYN 5
#define KEYSIZE 6
#define BUFFSIZE 18
#define TOTALBLOCKS_MINI 20
#define BLOK_SIZE 16

#define PIN_1 3
#define PIN_2 4
#define PIN_3 5
#define MAX_CHARGE 500
#define MIN_CHARGE 0



Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key keyR; // key for reading
MFRC522::MIFARE_Key keyW; // key for writing
MFRC522::StatusCode status; // status code

int stateButton1 = 0; // button state 1 
int stateButton2 = 0; // button state 2
int stateButton3 = 0; // button state 3

const byte keysA[KEYN][KEYSIZE] PROGMEM= {
                  {0xa0 , 0xa1 , 0xa2 , 0xa3 , 0xa4 , 0xa5},
                  {0x6b , 0x68 , 0xf6 , 0x2a , 0xeb , 0x9f},
                  {0xc9 , 0x0f , 0x65 , 0x38 , 0xf0 , 0x55},
                  {0x80 , 0x08 , 0xed , 0xa0 , 0x4e , 0x73},
                  {0x53,  0x00 , 0x1b , 0x20 , 0x26 , 0xea}
                  };
const byte keysB[KEYN][KEYSIZE] PROGMEM= {
                  {0xb4 , 0xc1 , 0x32 , 0x43 , 0x9e , 0xef},
                  {0x5d , 0x23 , 0xe6 , 0x29 , 0x74 , 0x2e},
                  {0xdf , 0xe8 , 0xfb , 0x84 , 0x9e , 0x4e}, //df e8 fb 84 9e 4e
                  {0x06 , 0x42 , 0x71 , 0x0c , 0xfa , 0xa1},
                  {0x1c , 0x1c , 0x45 , 0x5d , 0x81 , 0xf2}
                  };

void setup() {
              // DEFINE digital inputs for button reading
              pinMode(PIN_1, INPUT);
              pinMode(PIN_2, INPUT);
              pinMode(PIN_3, INPUT);


              Serial.begin(9600);    // Initialize serial communications with the PC
              SPI.begin();            // Init SPI bus
              mfrc522.PCD_Init();     // Init MFRC522 card
              
              if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c))   {  
                                                                  Serial.println(F("error display"));
                                                               }
              delay(1000); // Pause for 1 seconds
            
              display.clearDisplay(); // 
              display.display();
            
              
              display_str(F("SYSTEM \nIS READY"), 1 , 10 , 2); // it contains display.display f

            
} // end setup




void(* resetFunc) (void) = 0;//declare reset function at address 0

//-----------------------------------------------------------------------------
// main LOOP
void loop() {

  byte buffer1[BUFFSIZE] = {0}; // w/r buffer line 0a0
  byte buffer2[BUFFSIZE] = {0}; // w/r buffer line 090
  //byte buffer3[BUFFSIZE] = {0}; // w/r buffer line 080
  bool change_line = 0;
  int to_be_charged = 0; // money to  be charged
  int total_amount = 0;

    
  display.clearDisplay();
 
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( !mfrc522.PICC_IsNewCardPresent())
                                        {
                                           
                                            digitalWrite(13, HIGH); // SET LED TO HIGH
                                            return;
                                            
                                        }


   if ( ! mfrc522.PICC_ReadCardSerial())  
                                        {
                                           
                                            digitalWrite(13, HIGH); // SET LED TO HIGH
                                            return;
                                            
                                        }

    
    
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

    display_str(array_to_str((mfrc522.uid.uidByte), mfrc522.uid.size), 0  , 0 , 1);

    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
      

   // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K)
                                              {
                                                
                                              display.clearDisplay();
                                              display_str(F("KEY NOT \nCOMPATIBLE"),0,30,2);
                                              return;
                                              
                                              }

     copy_key (&keyR.keyByte[0] , keysA , 2); // MOV key for reading from stored keys . number 2
     copy_key (&keyW.keyByte[0] , keysB , 2); // MOV key for reading from stored keys . number 2 
    

     try_key(&keyR, 0x0a, buffer1);                    // CHECK FOR MONEY STORED LINE

     delay(100);
     
     if (buffer1[0] == 0x55) {
                            
                                  try_key(&keyR,0x09, buffer2);
                                  change_line = 0; // flag the line to be changed
                                  total_amount = read_money(buffer2); // read and display current money
                                 
                                  
                             }

                             else 
                             
                                   {
                                      try_key(&keyR,0x08, buffer2);
                                      change_line = 1; // flag the line to be changed
                                      total_amount = read_money(buffer2); // read and display current money
                                     
                                   }
     

    to_be_charged = (int)read_button_page_2(); // read the money to be charged and write it on the key

    total_amount = total_amount + to_be_charged; // sum the old amount to the amount to be charged
                                                  // we stored the line to be written we need to prepare the line and 
                                                  // store the new line
                                                  // dont forget to chage the soting code on the old buffer and advance the counter
     
  
     
      if (write_to_charge(buffer1, buffer2, total_amount)){
        
                                            display.clearDisplay(); // Clear display      
                                            display_str(F("SUCCESS"),0,30,3);
        
        }

      // before exit
      mfrc522.PICC_HaltA();       // Halt PICC
      mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
      delay(1500);
      resetFunc();

}


//----------------------------------------------------------------------------------
// defined functions 
//----------------------------------------------------------------------------------

String array_to_str(byte * arr , byte s_arr)
          {

            String my_str= "";
            
            
            for (byte i = 0; i < s_arr; i++) 
                  {


                      my_str += String(*arr < 0x10 ? " 0" : " ");
                      my_str += String(*arr, HEX);
                      arr++;
                    
                  }
                  
                my_str.toUpperCase();              
                
                return my_str;
          }

 
int read_button_page_2 (){ // exit from this cycle when enter is pressed 

        int to_be_charged = 0;
        bool push_1 = false;
        bool push_2 = false;
        bool push_3 = false;
        bool is_changed = false;
    
       
        // press enter to start reading
        while (true)
            {
            stateButton1 = digitalRead(PIN_1);

            if ((stateButton1 == 0) && (push_1 == false)) 
            
                         {
                             push_1 = true;
                         }


            if ((stateButton1 == 1) && push_1 == true) 
                          { 
              
                            push_1 = false;
                            break;
                          }
             
          
          }
        
          
          display.clearDisplay(); 
          display_str(F("HOW MUCH\nTO CHARGE"), 0,0,2 );
          

          // add or subtact money until the next transaction
            while(true) // exec until the button is pressed
                {
                  stateButton1 = digitalRead(PIN_1);
                  stateButton2 = digitalRead(PIN_2);
                  stateButton3 = digitalRead(PIN_3);
    
                        if (stateButton2 == 0 && push_2 == false) 
                                           {
                                             push_2 = true;
                                           }
          
                                                                        
                        if (stateButton3 == 0 && push_3 == false)
                                          {
                                             push_3 = true;
                                          }
          
          
                        if (stateButton2 == 1 && push_2 == true)
                                           {
                                            if (to_be_charged< MAX_CHARGE){
                                                                           to_be_charged +=10; 
                                                                           push_2 = false;
                                                                           is_changed=true;
                                                                           
                                                                          }
                                          }
          
                       if (stateButton3 == 1 && push_3 == true)
                                          {
                        
                                          if (to_be_charged> MIN_CHARGE)
                                                                          {
                                                                            to_be_charged -=10; 
                                                                            push_3 = false;
                                                                            is_changed=true;
                                                                            
                                                                          }
                                          }
    

                              if (is_changed){
                                           display.clearDisplay(); // Clear display      
                                           display_str(String(to_be_charged), 2, 0 , 3);
                                           //display_str(F("Cent(s)"),2,40,2);
                                           is_changed = false;
                              }
                                                                                                                
                                            // exit stata from cycle
                                            if ((stateButton1 == 0) && (push_1 == false)) 
                                
                                             {
                                                 push_1 = true;
                                             }
                    
                    
                                            if ((stateButton1 == 1) && push_1 == true) 
                                              { 
                                  
                                                push_1 = false;
                                                return to_be_charged;
                                              }

              }
  }





// save the stored key in the KEY structure
void copy_key( byte *key, const byte keyS[KEYN][KEYSIZE] , int keyN)
  {
  
        for (byte i=0; i<KEYSIZE; i++) {
                                          *key = keyS[keyN][i]; // key store
                                          key++; // advance pointer                                 
                                       }
  }




void display_str (String mystr, int posx, int posy, int sizeofchar)
{
  
    display.setTextSize(sizeofchar); // Draw X-scale text
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(posx, posy);
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.println(mystr);
    display.display();      // Show initial text
    delay(200);
  
  }


// READ BLOCK WILL BE SAVED INSIDE GLOBAL VARIABLE: buffer1
bool try_key(MFRC522::MIFARE_Key *key , byte block , byte *buffer_r)
{
 
      
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, key, &(mfrc522.uid));
    
                    if (status != MFRC522::STATUS_OK) // ERROR IN BLOCK READING WITH SUGGESTED KEY
                            {
                                   
                                   return false;
                            }
          delay(100);

          byte byteCount = 18; // CHECK SIZE OF READING BUFFER
          
          status = mfrc522.MIFARE_Read(block, buffer_r, &byteCount); // WRITE DATA INTO BUFFER
    
                    if (status != MFRC522::STATUS_OK) 
                            {
                                    
                                    return false;
                            }
   
            return true;
         
}
                   

//----------------------------------------------------------------------
// read the current amount of money
int read_money ( byte buff[18])
  {
    // sector 2 is the one that we will deal with 
    // KEY_A to access this sector will be used to read all data needed
    
    int amount = 0; // vsariable usedd to store data
    int upperbyte = 0;
    int lowerbyte = 0;
    upperbyte = (int)buff[2];
    lowerbyte = (int)buff[1];
    
    amount = (upperbyte << 8) + lowerbyte;

    display_str(F("MONEY: "), 0, 40 , 2);
    display_str(String((float)amount/100),70 , 40 , 2);
    
    return amount;

  }


  // write block function
bool write_to_charge(byte buffer_1[18], byte buffer_2[18], int amount) 
     {    

         buffer_2[1] = (byte)amount; // lower byte amount
         buffer_2[2] = (byte)(amount>>8); // upper byte amount
         buffer_2[3] = buffer_2[1] ^ buffer_2[2]; // xor check and store
         
        byte block;
    

    // change line 0a0
    if (buffer_1[0] == 0xAA)  {
                                 buffer_1[0] = 0x55;
                                 block = 0x09;
                              }
                          
                          else 
                               {               
                                      buffer_1[0] = 0xAA; // xor the old contenct FF
                                      block = 0x08;
                               }
       buffer_1[1]++; // increment the transaction counter
       buffer_2[15] = buffer_1[1]; // set the trnasaction on the new content
      
      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 010, &keyW, &(mfrc522.uid));
    
                  if (status != MFRC522::STATUS_OK) 
                                {
                                  Serial.println (F("error auth K-b"));
                                  return false;
                                }
                  
                  status = mfrc522.MIFARE_Write(0x0a, buffer_1, 16); // save the block counter
    
                  if (status != MFRC522::STATUS_OK) 
                        {
                          Serial.println("buffer not saved 0a");
                          return false;
                        }

                status = mfrc522.MIFARE_Write(block, buffer_2, 16); // save the bloch money
    
                  if (status != MFRC522::STATUS_OK) 
                        {
                          Serial.println("buffer not saved 080 090");
                          return false;
                        }

                  

        return true;
  
}
