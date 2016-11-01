#include <SPIFlash.h>
#include <SPI.h>
#include <Streaming.h>
#include <UTFT.h>
#include <mqtt.h>

const uint8_t PIN_FLASH_CS = 3;

#define FLASH_CS_ON digitalWrite(PIN_FLASH_CS, LOW)
#define FLASH_CS_OFF digitalWrite(PIN_FLASH_CS, HIGH)

const uint16_t DATA_SIZE = 256;

SPIFlash flash(PIN_FLASH_CS);

//flash begin pos
uint32_t pos = 0;

// Modify the line below to match your display and wiring:
// HSPI 4pin 12,13,14,15(CS)
// DC/RS 4
// RST  5
// LED(Back Light) PullUp (0 or V3.3)
const int PIN_TFT_CS = 15;
const int PIN_TFT_BL = 5;

extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

#define TFT_CS_ON digitalWrite(PIN_TFT_CS, LOW)
#define TFT_CS_OFF digitalWrite(PIN_TFT_CS, HIGH)
UTFT myGLCD(ILI9341_S5P, PIN_TFT_CS, 5, 4);

#define FHEAD 14
#define F16L  ((uint16_t)((261710 + 255)/256))*256 //must 256 align: 261888
#define F24L  F16L + 498528 + 14 //760430

union {
  uint8_t buff[9600];//9300 for 24,9600 for 16
  uint8_t fbuff24[3120][3];//for 24
  uint8_t fbuff16[4800][2];//for 16
} ubuff;//Font Data buffer

uint8_t buff[15360];//Display RAM Buffer: 15360 for 16;11520 for 24 px

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  Serial.println(F("Last Err reson"));
  Serial.println(ESP.getResetReason());

  Serial.println(F("Begin Flash Font Dump"));

  pinMode(PIN_FLASH_CS, OUTPUT);
  FLASH_CS_ON;
  flash.begin();
  Serial.println(F("Flash inited!"));
  FLASH_CS_OFF;

  //init TFT
  pinMode(PIN_TFT_CS, OUTPUT);
  pinMode(PIN_TFT_BL, OUTPUT);
  myGLCD.InitLCD(PORTRAIT);//PORTRAIT LANDSCAPE
  myGLCD.setFont(BigFont);
  unsigned long start = millis();
  myGLCD.clrScr();
  Serial.print(F("Clean screen times:\t"));
  Serial.println((millis() - start));
  myGLCD.setColor(VGA_WHITE);
  myGLCD.setBackColor(0, 0, 0xFF);
  myGLCD.print(F("Begin Dump Font"), 0, 32);
  myGLCD.setBackColor(0, 0, 0);
  Serial.println("END Init");
  delay(5000);
}


void dumpFont16() {
  uint16_t x = 0, y = 0, c = 0;
  uint8_t buff[256];
  uint16_t bcolor = VGA_BLACK, fcolor[3] = {VGA_RED, VGA_GREEN, VGA_BLUE};
  pos = 14 ;
  while (pos < 261710) {
    //    Serial.println(pos);
    sbi(myGLCD.P_CS, myGLCD.B_CS);
    delay(5);
    FLASH_CS_ON;
    delay(5);
    if (!flash.readByteArray(pos, buff, 256)) {
      Serial.println(F("Read Flash Error"));
    }
    //    for(uint8_t m=0;m<32;m++){
    //      Serial.print(buff[m],HEX);
    //    }
    //    Serial.print("    |");
    //    Serial.println(c);
    pos += 256;
    delay(40);
    FLASH_CS_OFF;
    //    cbi(myGLCD.P_CS, myGLCD.B_CS);
    //    myGLCD.setXY((c%15)*16,(c/15)*16,15,15);
    for (uint16_t m = 0; m < 8; m++) {
      for (uint16_t i = 0; i < 16; i++) {
        for (uint16_t j = 0; j < 16; j++) {
          if (((0x80 >> j % 8) & buff[m * 32 + i * 2 + (j >> 3)]) != 0) {
            myGLCD.drawPixel((c % 15) * 16 + j, (c / 15) * 16 + i);
          }
        }
      }
      //draw font pix
      c++;
      if (c == 300) {
        myGLCD.setColor(VGA_RED);
        myGLCD.setBackColor(VGA_TRANSPARENT);
        myGLCD.print("Print:", CENTER, 250);
        myGLCD.setFont(SevenSegNumFont);
        myGLCD.printNumI(pos, CENTER, 269);
        myGLCD.setColor(VGA_WHITE);
        myGLCD.setBackColor(0, 0, 0);
        myGLCD.setFont(BigFont);
        c = 0;
        delay(5000);
        myGLCD.clrScr();
      }
    }
  }
}


void dumpFont16Quick() {
  uint16_t x = 0, y = 0;
  pos = 14 ;
  unsigned long start = millis();
  while (pos < 261710) {
    //    Serial.println(pos);
    sbi(myGLCD.P_CS, myGLCD.B_CS);
    delay(5);
    FLASH_CS_ON;
    delay(5);
    if (!flash.readByteArray(pos, ubuff.buff, 9600)) {
      Serial.println(F("Read Flash Error"));
    }
    //    for(uint8_t m=0;m<32;m++){
    //      Serial.print(buff[m],HEX);
    //    }
    //    Serial.print("    |");
    //    Serial.println(c);
    pos += 9600;
    delay(40);
    FLASH_CS_OFF;
    cbi(myGLCD.P_CS, myGLCD.B_CS);
    //    myGLCD.setXY((c%15)*16,(c/15)*16,15,15);
    for (uint16_t m = 0; m < 300; m++) {
      uint16_t mChars = m * 16;
      //      uint16_t linePointX = (m % 10) * 24;
      uint16_t linePointY = (m / 30) * 32;
      //      uint16_t ml = 0;
      uint16_t b1 = (m % 15) * 32;//(m % 15) * 32;
      if((m/15)%2 == 1) b1 += 7680;
      if(m % 30 == 0){
        myGLCD.setXY(0, linePointY, 239, linePointY + 31);
        bzero(buff, 15360);
      }
      for (uint8_t i = 0; i < 16; i++) {//16 or 24
        uint16_t b2 = b1 + i * 480;
        uint16_t m1 = mChars + i;
        for (uint8_t j = 0; j < 16; j++) {
          // 16 * 16
          // if (((0x80 >> j % 8) & ubuff.buff[m * 32 + i * 2 + (j >> 3)]) != 0) {
          //   myGLCD.drawPixel((c % 15) * 16 + j, (c / 15) * 16 + i);
          // 24 * 24
          // if ((i1 & ubuff.fbuff[m * 24 + j][i2]) != 0) {
          //   myGLCD.drawPixel((c % 10) * 24 + j, (c / 10) * 24 + i);
          uint16_t b3 = b2 + j * 2;
          if (((0x80 >> j % 8) & ubuff.fbuff16[m1][j>>3]) != 0) {
            buff[b3] = 0xFF;
            buff[b3 + 1] = 0xFF;
          }
        }
      }
      //draw font pix
      if (m % 30 == 29) {
        sbi ( myGLCD.P_RS, myGLCD.B_RS );
        SPI.writeBytes(buff, 15360);
      }
      //c++;
      // Serial.print(F("2222\t"));
      // Serial.println(c);
      if (m == 299) { //299 for 16, 129 for 24
        Serial.println(F("3333"));
        myGLCD.setColor(VGA_RED);
        myGLCD.setBackColor(VGA_BLUE);
        myGLCD.print("Print:", CENTER, 230);
        myGLCD.printNumI((millis() - start), CENTER, 250);
        start = millis();
        myGLCD.setFont(SevenSegNumFont);
        myGLCD.printNumI(pos, CENTER, 269);
        myGLCD.setColor(VGA_WHITE);
        myGLCD.setBackColor(0, 0, 0);
        myGLCD.setFont(BigFont);
        //c = 0;
        delay(500);
        myGLCD.clrScr();
        delay(5);
        Serial.print(F("Heap Size:"));
        Serial.print(ESP.getFreeHeap());
        Serial.print(F("\tNext Pos:"));
        Serial.println(pos);
        Serial.println(F("4444"));
      }
      delay(5);
    }
  }
}


void dumpFont24() {
  uint16_t x = 0, y = 0;//, c = 0;
  //uint16_t bcolor = VGA_BLACK, fcolor[3] = {VGA_RED, VGA_GREEN, VGA_BLUE};
  pos = 14 + F16L;
  Serial.println(pos);
  unsigned long start = millis();
  while (pos < F24L) {
    //    Serial.println(pos);
    Serial.println(F("11111"));
    sbi(myGLCD.P_CS, myGLCD.B_CS);
    delay(5);
    FLASH_CS_ON;
    delay(5);
    Serial.println(F("Begint read flash"));
    start = millis();
    if (!flash.readByteArray(pos, ubuff.buff, 9360)) {//256 for 16;216 for 24 px
      Serial.println(F("Read Flash Error"));
    }
    Serial.println(millis() - start);
    Serial.println(F("End read flash"));
    //    for(uint8_t m=0;m<32;m++){
    //      Serial.print(buff[m],HEX);
    //    }
    //    Serial.print("    |");
    //    Serial.println(c);
    pos += 9360;//256 for 16;216 for 24 px
    delay(40);
    FLASH_CS_OFF;
    cbi(myGLCD.P_CS, myGLCD.B_CS);
    //    myGLCD.setXY((c%15)*16,(c/15)*16,15,15);
    //    uint16_t cidx = 0;
    for (uint8_t m = 0; m < 130; m++) {// 8 for 16; 3 for 24
      // for (uint8_t n = 0; n < 10; n++) {
      //   uint8_t z = m*n;
//      if (m % 10 == 0) {
//        bzero(buff, 11520);
//      }
      uint16_t mChars = m * 24;
      //      uint16_t linePointX = (m % 10) * 24;
      uint16_t linePointY = (m / 10) * 24;
      //      uint16_t ml = 0;
      uint16_t b1 = (m % 10) * 48;
      if (b1 == 0)
      {
        //        cidx = 0;
        bzero(buff, 11520);
        myGLCD.setXY(0, linePointY, 239, linePointY + 23);
      }
      for (uint8_t i = 0; i < 24; i++) {//16 or 24
        uint8_t i1 = 0x80 >> i % 8;
        uint8_t i2 = i / 8;
        //        ml = linePointY + i;
        uint16_t b2 = b1 + i * 480;
        for (uint8_t j = 0; j < 24; j++) {
          // 16 * 16
          // if (((0x80 >> j % 8) & ubuff.buff[m * 32 + i * 2 + (j >> 3)]) != 0) {
          //   myGLCD.drawPixel((c % 15) * 16 + j, (c / 15) * 16 + i);
          // 24 * 24
          // if ((i1 & ubuff.fbuff[m * 24 + j][i2]) != 0) {
          //   myGLCD.drawPixel((c % 10) * 24 + j, (c / 10) * 24 + i);
          uint16_t b3 = b2 + j * 2;
          if ((i1 & ubuff.fbuff24[mChars + j][i2]) != 0) {
            // myGLCD.drawPixel(linePointX + j, ml);//10 char per line
            //y*30+(m%10)*3+y/8+y%8
            buff[b3] = 0xFF;
            buff[b3 + 1] = 0xFF;
          }
        }
      }
      //draw font pix
      if (m % 10 == 9) {
        sbi ( myGLCD.P_RS, myGLCD.B_RS );
        SPI.writeBytes(buff, 11520);
      }
      //c++;
      // Serial.print(F("2222\t"));
      // Serial.println(c);
      if (m == 129) { //300 for 16, 129 for 24
        Serial.println(F("3333"));
        myGLCD.setColor(VGA_RED);
        myGLCD.setBackColor(VGA_BLUE);
        myGLCD.print("Print:", CENTER, 230);
        myGLCD.printNumI((millis() - start), CENTER, 250);
        start = millis();
        myGLCD.setFont(SevenSegNumFont);
        myGLCD.printNumI(pos, CENTER, 269);
        myGLCD.setColor(VGA_WHITE);
        myGLCD.setBackColor(0, 0, 0);
        myGLCD.setFont(BigFont);
        //c = 0;
        delay(500);
        myGLCD.clrScr();
        delay(5);
        Serial.print(F("Heap Size:"));
        Serial.print(ESP.getFreeHeap());
        Serial.print(F("\tNext Pos:"));
        Serial.println(pos);
        Serial.println(F("4444"));
      }
      delay(5);
    }
    //    sbi(myGLCD.P_CS, myGLCD.B_CS);
  }
  // }
}



void loop() {
  // put your main code here, to run repeatedly:
  myGLCD.clrScr();
  sbi(myGLCD.P_CS, myGLCD.B_CS);
  delay(5);
  dumpFont16Quick();
  delay(3000);
  unsigned long start = millis();
  myGLCD.clrScr();
  Serial.print(F("Clean screen times:\t"));
  Serial.println((millis() - start));
  // sbi(myGLCD.P_CS, myGLCD.B_CS);
  delay(5);
  Serial.print(F("Heap Size:"));
  Serial.println(ESP.getFreeHeap());
  dumpFont24();
  delay(3000);
}
