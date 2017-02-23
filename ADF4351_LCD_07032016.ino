//ADF4351 
//original by By Alain Fort F1CJN feb 2,2016 edited and tuned by Giorgio IZ2XBZ version 2.1





//  ******************************************** HARDWARE IMPORTANT********************************************************
//  With an Arduino UN0 : uses a resistive divider to reduce the voltage, MOSI (pin 11) to
//  ADF DATA, SCK (pin13) to ADF CLK, Select (PIN 3) to ADF LE
//  Resistive divider 560 Ohm with 1000 Ohm to ground on Arduino pins 11, 13 et 3 to adapt from 5V
//  to 3.3V the digital signals DATA, CLK and LE send by the Arduino.
//  Arduino pin 2 (for lock detection) directly connected to ADF4351 card MUXOUT.
//  The ADF card is 5V powered by the ARDUINO (PINs +5V and GND are closed to the Arduino LED).



#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <SPI.h>

#define ADF4351_LE 3

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


//#include <Encoder.h>

//Encoder myEnc(2, A5);
long oldPosition  = -999;


byte poscursor = 0;
byte line = 0;
byte memory,RWtemp; // 

uint32_t registers[6] =  {0x4580A8, 0x80080C9, 0x4E42, 0x4B3, 0xBC803C, 0x580005} ; // 437 MHz avec ref à 25 MHz
int address,modif=0,WEE=0;
int lcd_key = 0;
int adc_key_in  = 0;
int timer = 0,timer2=0; 
unsigned int i = 0;


double RFout, REFin, INT, PFDRFout, OutputChannelSpacing, FRACF;
double RFoutMin = 35, RFoutMax = 4400, REFinMax = 250, PDFMax = 32;
unsigned int long RFint,RFintold,INTA,RFcalc,PDRFout, MOD, FRAC;
byte OutputDivider;byte lock=2;
unsigned int long reg0, reg1;

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int read_LCD_buttons()
{
  adc_key_in = analogRead(0);     
  if (adc_key_in < 790)lcd.blink();
  
  if (adc_key_in < 80)return btnRIGHT;  
  if (adc_key_in < 195)return btnUP;
  if (adc_key_in < 380)return btnDOWN;
  if (adc_key_in < 555)return btnLEFT;
  if (adc_key_in < 790)return btnSELECT; 
  
  //if (adc_key_in < 50)return btnRIGHT; 
  //if (adc_key_in < 250)return btnUP;
  //if (adc_key_in < 450)return btnDOWN;
  //if (adc_key_in < 650)return btnLEFT;
  //if (adc_key_in < 850)return btnSELECT; // for  ROBOT LCD SHIELD  1.1

  return btnNONE;  
}





void printAll ()
{
  //RFout=1001.10 // test
  lcd.setCursor(0, 0);
  lcd.print("RF = ");
  if (RFint < 100000) lcd.print(" ");
  if (RFint < 10000)  lcd.print(" ");
  lcd.print(RFint/100);lcd.print(".");
  RFcalc=RFint-((RFint/100)*100);
  if (RFcalc<10)lcd.print("0");
  lcd.print(RFcalc);
  lcd.print(" MHz");
  lcd.setCursor(0,1);
  if (WEE==0) {lcd.print("REE=");}
  else {lcd.print("WEE=");}
  if (memory<10)lcd.print(" ");
  lcd.print(memory,DEC);
  if  ((digitalRead(A4)==1))lcd.print(" LOCKED ");
  else lcd.print(" NOLOCK ");
  lcd.print(PFDRFout,DEC);
  lcd.setCursor(poscursor,line);
}

void WriteRegister32(const uint32_t value)  
{
  digitalWrite(ADF4351_LE, LOW);
  for (int i = 3; i >= 0; i--)         
  SPI.transfer((value >> 8 * i) & 0xFF); 
  digitalWrite(ADF4351_LE, HIGH);
  digitalWrite(ADF4351_LE, LOW);
}

void SetADF4351() 
{ for (int i = 5; i >= 0; i--)  
    WriteRegister32(registers[i]);
}






void EEPROMWritelong(int address, long value)
      {
      //scomposition long (32bits) in 4 bytes
      //3 = MSB -> 4 = lsb
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Ecrit 4 bytes dans la memory EEPROM
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
      }





long EEPROMReadlong(long address)
      {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
      }


//************************************ Setup ****************************************




void setup() {
 

  
  
  
  lcd.begin(16, 2);
  lcd.display();
  analogWrite(10,255);

  Serial.begin (9600); 
  Serial.print("   GENERATOR  ");
  lcd.print("   GENERATOR   ");
  lcd.setCursor(0, 1);
  lcd.print("ADF4351  34-4000");
  poscursor = 7; line = 0; 
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("   By  IZ2XBZ    ");
   delay(1000);

  pinMode(2, INPUT); 
  pinMode(ADF4351_LE, OUTPUT);          // Setup pins
  digitalWrite(ADF4351_LE, HIGH);
  SPI.begin();                          // Init SPI bus
  SPI.setDataMode(SPI_MODE0);           // CPHA = 0  Clock positive
  SPI.setBitOrder(MSBFIRST);          


  if (EEPROM.read(100)==55){PFDRFout=EEPROM.read(20*4);} 
  else {PFDRFout=10;}

  if (EEPROM.read(101)==55){RFint=EEPROMReadlong(memory*4);}
  else {RFint=7000;}

  RFintold=1234;
  RFout = RFint/100 ; 
  OutputChannelSpacing = 0.01; 

   PFDRFout=10;


  WEE=0;  address=0;
  lcd.blink();
  printAll(); delay(500);


} // Fin setup

//*************************************Loop***********************************
void loop()
{

  
  RFout=RFint;
  RFout=RFout/100;
  if ((RFint != RFintold)|| (modif==1)) {
    //Serial.print(RFout,DEC);Serial.print("\r\n");
    if (RFout >= 2200) {
      OutputDivider = 1;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout < 2200) {
      OutputDivider = 2;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout < 1100) {
      OutputDivider = 4;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout < 550)  {
      OutputDivider = 8;
      bitWrite (registers[4], 22, 0);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout < 275)  {
      OutputDivider = 16;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 0);
    }
    if (RFout < 137.5) {
      OutputDivider = 32;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 0);
      bitWrite (registers[4], 20, 1);
    }
    if (RFout < 68.75) {
      OutputDivider = 64;
      bitWrite (registers[4], 22, 1);
      bitWrite (registers[4], 21, 1);
      bitWrite (registers[4], 20, 0);
    }

    INTA = (RFout * OutputDivider) / PFDRFout;
    MOD = (PFDRFout / OutputChannelSpacing);
    FRACF = (((RFout * OutputDivider) / PFDRFout) - INTA) * MOD;
    FRAC = round(FRACF); // On arrondit le résultat

    registers[0] = 0;
    registers[0] = INTA << 15; // OK
    FRAC = FRAC << 3;
    registers[0] = registers[0] + FRAC;

    registers[1] = 0;
    registers[1] = MOD << 3;
    registers[1] = registers[1] + 1 ; // ajout de l'adresse "001"
    bitSet (registers[1], 27); // Prescaler sur 8/9

    bitSet (registers[2], 28); // Digital lock == "110" sur b28 b27 b26
    bitSet (registers[2], 27); // digital lock 
    bitClear (registers[2], 26); // digital lock
   
    SetADF4351();  // Programme tous les registres de l'ADF4351
    RFintold=RFint;modif=0;
    printAll();  // Affichage LCD
  }

  lcd_key = read_LCD_buttons();  

  switch (lcd_key)              
  {
    case btnRIGHT: //Droit
      poscursor++; // cursor to the right
      Serial.print(poscursor,DEC);  
      Serial.print("-");  
      Serial.println(line,DEC); 
      if (line == 0) {
        if (poscursor == 9 ) {
          poscursor = 10;
          line = 0; } //si curseur sur le .
        if (poscursor == 12 ) {
          poscursor = 0; line = 1; }; 
      }
     if (line == 1) {
        if (poscursor == 1 ) {poscursor = 5; line = 1; } 
     //  if (poscursor == 6 ) {poscursor = 15; line = 1; } 
              if (poscursor == 6 ) {poscursor = 5; line = 1; }

        if (poscursor==16) {poscursor=5; line=0;};     
      }  
      //Serial.print (" RIGHT Button\r\n");
      lcd.setCursor(poscursor, line);
      break;
      
    case btnLEFT:
      poscursor--; 
   Serial.print(poscursor,DEC);  
      Serial.print("-");  
      Serial.println(line,DEC); 
      if (line == 0) {
        if (poscursor == 4) {poscursor = 15; line = 1;  };
        if (poscursor == 9) {   poscursor = 8; line=0;}
      }
       if(line==1){
          if (poscursor==255) {poscursor=11; line=0;};
    //      if (poscursor==4) {poscursor=0; line=1;};
                    if (poscursor==4) {poscursor=11; line=0;};


          if (poscursor==14) {poscursor=5; line=1;};


      }
     // Serial.print(poscursor,DEC);  
      lcd.setCursor(poscursor, line);
      break;
      
    case btnUP: //Haut
      if (line == 0)
      { // RFoutfrequency
        //Serial.print(oldRFint,DEC);
        if (poscursor == 5) RFint = RFint + 100000 ;
        if (poscursor == 6) RFint = RFint + 10000 ;
        if (poscursor == 7) RFint = RFint + 1000 ;
        if (poscursor == 8) RFint = RFint + 100 ;
        if (poscursor == 10) RFint = RFint + 10 ;
        if (poscursor == 11) RFint = RFint + 1 ;
        if (RFint > 440000)RFint = RFintold;
        //Serial.print(RFint,DEC);
        //Serial.print("  \r\n");
      }
      if (line == 1)
      { 
        if (poscursor == 5){ memory++; 
        if (memory==20)memory=0;
        if (WEE==0){RFint=EEPROMReadlong(memory*4); 
           if (RFint>440000) RFint=440000; 
           } 
        }  
        if (poscursor==15){ 
          poscursor = 5;
    //    if( PFDRFout==10){PFDRFout=25;}
    //    else if ( PFDRFout==25){PFDRFout=10;}
    //    else PFDRFout=25;// au cas ou PFDRF different de 10 et 25
   //     modif=1; 
   }
                    
      if( (poscursor==0) && (WEE==1))WEE=0;
      else if ((poscursor==0) && (WEE==0))WEE=1;                  
      }
        printAll();
      break; // fin bouton up

    case btnDOWN: //bas
      if (line == 0) {
        if (poscursor == 5) RFint = RFint - 100000 ;
        if (poscursor == 6) RFint = RFint - 10000 ;
        if (poscursor == 7) RFint = RFint - 1000 ;
        if (poscursor == 8) RFint = RFint - 100 ;
        if (poscursor == 10) RFint = RFint - 10 ;
        if (poscursor == 11) RFint = RFint - 1 ;
        if (RFint < 3450) RFint = RFintold;
        if (RFint > 440000)  RFint = RFintold;
        break;
      }

     if (line == 1)
      { 
        if (poscursor == 5){memory--; 
        if (memory==255)memory=19;
        if (WEE==0){RFint=EEPROMReadlong(memory*4); 
           if (RFint>440000) RFint=440000;
          // Serial.print(RFint,DEC);  
           } 
        } // fin poscursor =5 

       if (poscursor==15){ 
          poscursor = 5;
 //      if( PFDRFout==10){PFDRFout=25;} //reglage FREF
 //      else if ( PFDRFout==25){PFDRFout=10;}
 //      else PFDRFout=25;// au cas ou PFDRF different de 10 et 25
//       modif=1;
       }
                   
       if( (poscursor==0) && (WEE==1))WEE=0;
       else if ((poscursor==0)&&(WEE==0))WEE=1;                          
      
       printAll();
      // Serial.print (" DOWN Button  \r\n");
      break; // fin bouton bas
      }

    case btnSELECT:
      do {
        adc_key_in = analogRead(0);    
        delay(1); timer2++;        
        if (timer2 > 600) {
         if (WEE==1 || poscursor==15){ 
         if (line==1 && poscursor==15){ EEPROMWritelong(20*4,PFDRFout);EEPROM.write(100,55);} 
         else if (WEE==1) {EEPROMWritelong(memory*4,RFint);EEPROM.write(101,55);}
          lcd.setCursor(0,1); lcd.print("  Storing....  ");}
          lcd.setCursor(poscursor,line);
          delay(500);timer2=0;
          printAll();
        }; // mes

        } 
      while (adc_key_in < 900);
      break; 
     case btnNONE: {
        break;
      };
      break;
  }// Fin LCD keys



  do { adc_key_in = analogRead(0); delay(1);} while (adc_key_in < 900);
   delay (10);timer++; // inc timer
   //Serial.print(timer,DEC);
  if (timer>1000){lcd.noBlink();timer=0;} 


  

}  


