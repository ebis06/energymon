/*
 EmonTx Teleinfo

 An example sketch for the emontx module for 
 EDF's teleinformation monitoring

 Libraries in the standard arduino libraries folder:
	- JeeLib		https://github.com/jcw/jeelib
	- EmonLib		https://github.com/openenergymonitor/EmonLib.git
        - Software Serial
 Other libraries:
        - Regex                 https://github.com/nickgammon/Regexp
*/
#ifndef REGEXP
#define REGEXP
#include <Regexp.h>
#endif
#include <SoftwareSerial.h>

#define DEBUG (0)
#define TEST_SERIAL (0)
#define READ_TELEINFO (1)

typedef struct { 
  unsigned int hchc;  
  unsigned int hchp;
  unsigned int iinst1;
  unsigned int iinst2;
  unsigned int iinst3;
  unsigned int imax1;
  unsigned int imax2;
  unsigned int imax3;
  unsigned int pmax;
  unsigned int papp; 
  unsigned int battery;
  int ptecHC;
  int power1; 
  unsigned int hchcDec;  
  unsigned int hchpDec;

} PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;                                                       

//#define FILTERSETTLETIME 5000              //  Time (ms) to allow the filters to settle before sending data

#define FILTERSETTLETIME 500             //  Time (ms) to allow the filters to settle before sending data

const int CT1 = 1;
const int CT2 = 0; // Set to 0 to disable CT channel 2
const int CT3 = 0; // Set to 0 to disable CT channel 3


#define FRAME_SIZE (256)

#define freq RF12_433MHZ                   // Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
const int nodeID = 10;                     // emonTx RFM12B node ID
const int networkGroup = 210;              // emonTx RFM12B wireless network group - needs to be same as emonBase and emonGLCD

const int UNO = 1;                         // Set to 0 if your not using the UNO bootloader (i.e using Duemilanove) - All Atmega's shipped from OpenEnergyMonitor come with Arduino Uno bootloader
#include <avr/wdt.h>                                                     

#include <JeeLib.h>                        // Download JeeLib: http://github.com/jcw/jeelib
ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Attached JeeLib sleep function to Atmega328 watchdog - enables MCU to be put into sleep mode inbetween readings to reduce power consumption 

#include "EmonLib.h"
EnergyMonitor ct1, ct2, ct3;                         // Create  instances for each CT channel

const int LEDpin = 9;                      // On-board emonTx LED 

boolean settled = false;
SoftwareSerial mySerialOne(6, 5);
SoftwareSerial mySerialTwo(8, 9); // Dummy connection to be able to disable mySerialOne
char receivedChar ='\0';

#if READ_TELEINFO
char frame[FRAME_SIZE];
#else
/* Example of data transmitted:*/
char frame[FRAME_SIZE] ="ADCO 041130079749 D \
OPTARIF HC.. < \
ISOUSC 20 8 \
HCHC 008784338 / \
HCHP 014186978 ? \
PTEC HP..   \  
IINST1 001 I \
IINST2 003 L \
IINST3 004 N \
IMAX1 021 3 \
IMAX2 031 5 \
IMAX3 030 5 \
PMAX 12780 8  \
PAPP 01880 2 \
HHPHC A , \
MOTDETAT 000000 B \
PPOT 00 #";
#endif

int i = 0;

extern int freeRam (void);
extern int findInMatch(MatchState* ms, char* label, char * matchPat, char* value);
extern void send_rf_data(void);
extern void emontx_sleep(int seconds);


void setup ()
{
  Serial.begin (9600);
  Serial.println ("Reset");

  mySerialOne.begin(1200);  
  mySerialTwo.begin(1200);
  Serial.print ("Free Mem: ");Serial.print (freeRam());Serial.println (" B");
    
  rf12_initialize(nodeID, freq, networkGroup);    // initialize RFM12B
  rf12_sleep(RF12_SLEEP);

  pinMode(LEDpin, OUTPUT);                        // Setup indicator LED
  digitalWrite(LEDpin, HIGH);
  
  if (UNO) wdt_enable(WDTO_8S);                   // Enable anti crash (restart) watchdog if UNO bootloader is selected. Watchdog does not work with duemilanove bootloader                                                     
  // Restarts emonTx if sketch hangs for more than 8s

  /* Init the TxPayload Structure */
  emontx.hchc = 0;
  emontx.hchp = 0;

  emontx.hchcDec = 0;
  emontx.hchpDec = 0;

  emontx.imax1 = emontx.imax2= emontx.imax3 = 0;
  emontx.iinst1 = emontx.iinst2= emontx.iinst3 = 0;
  emontx.papp = emontx.pmax = 0;

  if (CT1) ct1.currentTX(1, 111.1); // Setup emonTX CT channel (ADC input, calibration)
  if (CT2) ct2.currentTX(2, 111.1); // Calibration factor = CT ratio / burden resistance
  if (CT3) ct3.currentTX(3, 111.1); // Calibration factor = (100A / 0.05A) / 33 Ohms


}  // end of setup  

void loop () {
char value [32] = ""; 
char hourInt [8] = ""; 
char hourDec [8] = ""; 

#if TEST_SERIAL
  mySerialOne.listen();
  if (mySerialOne.available())
      Serial.write(mySerialOne.read() & 0x7F);
}
#else
  /* Monitor the baterry of emontx */
  emontx.battery = ct1.readVcc();                                      //read emonTx battey (supply voltage)
  //Serial.print(" "); Serial.print(emontx.battery);
  //Serial.println(); 
  delay(100);
  Serial.print ("Free Mem: ");Serial.print (freeRam());Serial.println (" B");

  if (CT1) {
    emontx.power1 = ct1.calcIrms(1480) * 240.0; //ct.calcIrms(number of wavelengths sample)*AC RMS voltage
    Serial.print(emontx.power1);
  }  
  if (CT2) {
   // emontx.power2 = ct2.calcIrms(1480) * 240.0;
   // Serial.print(" "); Serial.print(emontx.power2);
  }

  if (CT3) {
    //emontx.power3 = ct3.calcIrms(1480) * 240.0;
    //Serial.print(" "); Serial.print(emontx.power3);
  } 

#if READ_TELEINFO
  /* Read a teleinfo frame */
  mySerialOne.listen();

  while(receivedChar != 0x02) // Wait for the beginning of the frame
  {
    if (mySerialOne.available())
      receivedChar = mySerialOne.read() & 0x7F;
  }
  i = 0;
  while(receivedChar != 0x03) // 
  {
    if (mySerialOne.available()) {
      receivedChar = mySerialOne.read() & 0x7F;
      frame[i++] = receivedChar;
      if( i > (FRAME_SIZE-1))
      {
        receivedChar=0x03;
        i=0;
        Serial.println("early exit");
      }    
    }  
  }
  frame[i++]='\0';
  
  mySerialTwo.listen();
#endif

    /* Match state object */
  MatchState ms;
  MatchState ms2;
  
  
  /* string we are searching in */
  ms.Target (frame);
  // original buffer
//  Serial.println (frame);
    
  if (findInMatch(&ms, "HCHC",  ".([0-9]+)(.)(.)", value))  
  {
    //Serial.write(atol(value.substring(0,5)));
    ms2.Target(value);
    ms2.Match ("(%d%d%d%d%d%d)(%d%d%d)");
    ms2.GetCapture(hourInt, 0);
    ms2.GetCapture(hourDec, 1);
    
    
    
//    Serial.println("Interger part");    
//    Serial.println((hourInt));
//    Serial.println("Decimal part");    
//    Serial.println((hourDec));
    
    //Serial.println("end of sring");    
   // emontx.hchc =   (unsigned int)(atol(value) & 0xFFFF); //(atol(value)/1000);
    emontx.hchc =   (unsigned int)(atol(hourInt));    
    emontx.hchcDec = (unsigned int)(atol(hourDec));
  }
  if (findInMatch(&ms, "HCHP",  ".([0-9]+)(.)(.)", value))  {
    ms2.Target(value);
    ms2.Match ("(%d%d%d%d%d%d)(%d%d%d)");
    ms2.GetCapture(hourInt, 0);
    ms2.GetCapture(hourDec, 1);
//    emontx.hchp =   (unsigned int)(atol(value) & 0xFFFF); //(atol(value)/1000); 
    emontx.hchp =   (unsigned int)(atol(hourInt));    
    emontx.hchpDec = (unsigned int)(atol(hourDec));
  }
  if (findInMatch(&ms, "IINST1",".([0-9]+)(.)(.)", value))  emontx.iinst1 = atol(value);      
  if (findInMatch(&ms, "IINST2",".([0-9]+)(.)(.)", value))  emontx.iinst2 = atol(value);      
  if (findInMatch(&ms, "IINST3",".([0-9]+)(.)(.)", value))  emontx.iinst3 = atol(value);      
  if (findInMatch(&ms, "IMAX1", ".([0-9]+)(.)(.)", value))  emontx.imax1 =  atol(value);      
  if (findInMatch(&ms, "IMAX2", ".([0-9]+)(.)(.)", value))  emontx.imax2 =  atol(value);      
  if (findInMatch(&ms, "IMAX3", ".([0-9]+)(.)(.)", value))  emontx.imax3 =  atol(value);      
  if (findInMatch(&ms, "PMAX",  ".([0-9]+)(.)(.)", value))  emontx.pmax =   atol(value);      
  if (findInMatch(&ms, "PAPP",  ".([0-9]+)(.)(.)", value))  emontx.papp =   atol(value);      
  if (findInMatch(&ms, "PTEC",  ".(....)(.)(.)", value))  emontx.ptecHC == (value=="HC.." ? 1 : 0);      
  
  // because millis() returns to zero after 50 days ! 
//  Serial.println(millis());
  if (!settled && millis() > FILTERSETTLETIME) settled = true;

  if (settled)                                                            // send data only after filters have settled
  { 
#if DEBUG
    Serial.println("Sending data");
#endif
    send_rf_data();                                                       // *SEND RF DATA* - see emontx_lib
#if DEBUG
    Serial.println("Data sent");
#endif
    digitalWrite(LEDpin, HIGH); delay(2); digitalWrite(LEDpin, LOW);      // flash LED
    emontx_sleep(5);                                                      // sleep or delay in seconds - see emontx_lib
  }
}
#endif

