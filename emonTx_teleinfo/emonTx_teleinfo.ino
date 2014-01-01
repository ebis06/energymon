/*
 EmonTx Teleinfo

 An example sketch for the emontx module for 
 EDF's teleinformation monitoring

 Libraries in the standard arduino libraries folder:
	- JeeLib		https://github.com/jcw/jeelib
	- EmonLib		https://github.com/openenergymonitor/EmonLib.git
        - Software Serial
 Other libraries:
        - Regex 
*/
#include <Regexp.h>
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
} PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;                                                       

//#define FILTERSETTLETIME 5000              //  Time (ms) to allow the filters to settle before sending data

#define FILTERSETTLETIME 500             //  Time (ms) to allow the filters to settle before sending data

#define FRAME_SIZE (256)

#define freq RF12_433MHZ                   // Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
const int nodeID = 10;                     // emonTx RFM12B node ID
const int networkGroup = 210;              // emonTx RFM12B wireless network group - needs to be same as emonBase and emonGLCD

const int UNO = 1;                         // Set to 0 if your not using the UNO bootloader (i.e using Duemilanove) - All Atmega's shipped from OpenEnergyMonitor come with Arduino Uno bootloader
#include <avr/wdt.h>                                                     

#include <JeeLib.h>                        // Download JeeLib: http://github.com/jcw/jeelib
ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Attached JeeLib sleep function to Atmega328 watchdog - enables MCU to be put into sleep mode inbetween readings to reduce power consumption 

#include "EmonLib.h"
EnergyMonitor ct1;                         // Create  instances for each CT channel

const int LEDpin = 9;                      // On-board emonTx LED 

boolean settled = false;
SoftwareSerial mySerialOne(5, 6);
SoftwareSerial mySerialTwo(8, 9); // Dummy connection to be able to disable mySerialOne
char receivedChar ='\0';

#if READ_TELEINFO
char frame[FRAME_SIZE];
//char frame[FRAME_SIZE] ="HCHC 033784338 /\0";
//char frame[FRAME_SIZE] ="PAPP 01820 ,\0";
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

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void send_rf_data()
{
  rf12_sleep(RF12_WAKEUP);
  // if ready to send + exit loop if it gets stuck as it seems too
  int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}
  rf12_sendStart(0, &emontx, sizeof emontx);
  // set the sync mode to 2 if the fuses are still the Arduino default
  // mode 3 (full powerdown) can only be used with 258 CK startup fuses
  
  //rf12_sendWait(2);
  rf12_sendWait(1);
  
  rf12_sleep(RF12_SLEEP);
}

void emontx_sleep(int seconds) {
  // if (emontx.battery > 3300) { 
    for (int i=0; i<seconds; i++) { 
      delay(1000); 
      if (UNO) wdt_reset();
    } 
  // } else Sleepy::loseSomeTime(seconds*1000);
}

int findInMatch(MatchState* ms, char* label, char* value)
{
char regex [24] = "";
char checksum[8] = "";
char temp[8] = "";
unsigned int sum = 0;		// Somme des codes ASCII du message + un espace
unsigned char i = 0;
  strcpy(regex, label);		

  strcat(regex,  ".([0-9]+)(.)(.)");
#if DEBUG  
  Serial.println(regex);
#endif
  ms->Match (regex);
  ms->GetCapture(value, 0);
  ms->GetCapture(checksum, 2);
#if DEBUG  
  Serial.print(label);Serial.print(":"); Serial.println(value);
  Serial.print("CKSUM:"); Serial.println(checksum);
#endif
  sum = 0x40;

#if DEBUG
  Serial.print("lENGTH of labal = ");Serial.println(strlen(label), DEC);
  Serial.print("lENGTH of value = ");Serial.println(strlen(value), DEC);
#endif
  
  for (i=0; i < strlen(label); i++) {
#if DEBUG    
    Serial.print("First sum = ");Serial.print(sum, HEX);Serial.print(" Label[i] = ");Serial.print(label[i], HEX);
#endif
    sum = sum + label[i];
#if DEBUG    
    Serial.print(" Updated sum = ");Serial.println(sum, HEX);
#endif

  }
#if DEBUG  
  Serial.print("Value length = ");Serial.println(strlen(value));
#endif
  for (i=0; i < strlen(value); i++) {
#if DEBUG    
    Serial.print("First sum = "); Serial.print(sum, HEX); Serial.print(" value[i] = "); Serial.print(value[i], HEX);
#endif
    sum = sum + value[i];
#if DEBUG    
    Serial.print(" Updated sum = ");Serial.println(sum, HEX);
#endif
  }
    sum = ((sum + 0X20) & 0x3F) + 0x20 ;
#if DEBUG
  Serial.print("Computed checksum: ");Serial.println(sum, HEX);
  Serial.print("Provided checksum: ");Serial.println(checksum);
#endif
  if(sum == checksum[0]) {
    Serial.print("Found correct "); Serial.print(label);Serial.print(" with value ");Serial.println(value);
    return 1;
  }  
  else {
    Serial.print("Found incorrect checksum for "); Serial.print(label);Serial.print(" with value ");Serial.print(value);Serial.print(" and checksum ");Serial.println(sum);
    return 0;
  }
}
  

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
  emontx.imax1 = emontx.imax2= emontx.imax3 = 0;
  emontx.iinst1 = emontx.iinst2= emontx.iinst3 = 0;
  emontx.papp = emontx.pmax = 0;

}  // end of setup  

void loop () {
char value [32] = ""; 


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
  
#if READ_TELEINFO
  /* Read a teleinfo frame */
  mySerialOne.listen();

  while(receivedChar != 0x02) // Wait for the beginning of the frame
  {
    if (mySerialOne.available())
      receivedChar = mySerialOne.read() & 0x7F;
//       Serial.write(receivedChar);
  }
  i = 0;
  while(receivedChar != 0x03) // 
  {
    if (mySerialOne.available()) {
      receivedChar = mySerialOne.read() & 0x7F;
//      Serial.write(receivedChar);
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
  
  /* string we are searching in */
  ms.Target (frame);
  // original buffer
//  Serial.println (frame);
    
  if (findInMatch(&ms, "HCHC",  value))  emontx.hchc =   (unsigned int)(atol(value)/1000);      
  if (findInMatch(&ms, "HCHP",  value))  emontx.hchp =   (unsigned int)(atol(value)/1000); 
  if (findInMatch(&ms, "IINST1",value))  emontx.iinst1 = atol(value);      
  if (findInMatch(&ms, "IINST2",value))  emontx.iinst2 = atol(value);      
  if (findInMatch(&ms, "IINST3",value))  emontx.iinst3 = atol(value);      
  if (findInMatch(&ms, "IMAX1", value))  emontx.imax1 =  atol(value);      
  if (findInMatch(&ms, "IMAX2", value))  emontx.imax2 =  atol(value);      
  if (findInMatch(&ms, "IMAX3", value))  emontx.imax3 =  atol(value);      
  if (findInMatch(&ms, "PMAX",  value))  emontx.pmax =   atol(value);      
  if (findInMatch(&ms, "PAPP",  value))  emontx.papp =   atol(value);      
  
  // because millis() returns to zero after 50 days ! 
//  Serial.println(millis());
  if (!settled && millis() > FILTERSETTLETIME) settled = true;

  if (settled)                                                            // send data only after filters have settled
  { 
#if 1 //DEBUG
    Serial.println("Sending data");
#endif
    send_rf_data();                                                       // *SEND RF DATA* - see emontx_lib
#if 1 //DEBUG
    Serial.println("Data sent");
#endif
    digitalWrite(LEDpin, HIGH); delay(2); digitalWrite(LEDpin, LOW);      // flash LED
    emontx_sleep(5);                                                      // sleep or delay in seconds - see emontx_lib
  }
}
#endif
