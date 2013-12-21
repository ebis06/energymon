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

/* Example of data transmitted:
ADCO 524563565245 /
OPTARIF HC.. <
ISOUSC 20 8
HCHC 001065963 _
HCHP 001521211 '
PTEC HC.. S
IINST 001 I
IMAX 008 2 
PMAX 06030 3
PAPP 01250 +
HHPHC E 0
MOTDETAT 000000 B
PPOT 00 #
ADCO 524563565245 /
OPTARIF HC.. <
ISOUSC 20 8
*/

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


#define FILTERSETTLETIME 5000                                           //  Time (ms) to allow the filters to settle before sending data

#define freq RF12_433MHZ                                                // Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
const int nodeID = 10;                                                  // emonTx RFM12B node ID
const int networkGroup = 210;                                           // emonTx RFM12B wireless network group - needs to be same as emonBase and emonGLCD

const int UNO = 1;                                                      // Set to 0 if your not using the UNO bootloader (i.e using Duemilanove) - All Atmega's shipped from OpenEnergyMonitor come with Arduino Uno bootloader
#include <avr/wdt.h>                                                     

#include <JeeLib.h>                                                     // Download JeeLib: http://github.com/jcw/jeelib
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                              // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption 

#include "EmonLib.h"
EnergyMonitor ct1,ct2,ct3;                                              // Create  instances for each CT channel

const int LEDpin = 9;                                                   // On-board emonTx LED 

boolean settled = false;
SoftwareSerial cptSerial(5, 6);

char receivedChar ='\0';
char frame[512] ="ADCO 524563565245 / \
  OPTARIF HC.. < \
  ISOUSC 20 8 \
  HCHC 001065963 _\
  HCHP 001521211 '\
  PTEC HC.. S\
  IINST1 001 I\
  IINST2 001 I\
  IINST3 001 I\
  IMAX1 008 2 \
  IMAX2 008 2 \
  IMAX3 008 2 \
  PMAX 06030 3\
  PAPP 01250 +\
  HHPHC E 0\
  MOTDETAT 000000 B\
  PPOT 00 #\
  ADCO 524563565245 /\
  OPTARIF HC.. <\
  ISOUSC 20 8";
int i = 0;

void send_rf_data()
{
  rf12_sleep(RF12_WAKEUP);
  // if ready to send + exit loop if it gets stuck as it seems too
  int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}
  rf12_sendStart(0, &emontx, sizeof emontx);
  // set the sync mode to 2 if the fuses are still the Arduino default
  // mode 3 (full powerdown) can only be used with 258 CK startup fuses
  rf12_sendWait(2);
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

/*called for each match */
void match_callback  (const char * match,          // matching string (not null-terminated)
                      const unsigned int length,   // length of matching string
                      const MatchState & ms)      // MatchState in use (to get captures)
{
#if 1
  char cap [10];   // must be large enough to hold captures
  
  Serial.print ("Matched: ");
  Serial.write ((byte *) match, length);
  Serial.println ();
  
  for (byte i = 0; i < ms.level; i++)
    {
    Serial.print ("Capture "); 
    Serial.print (i, DEC);
    Serial.print (" = ");
    ms.GetCapture (cap, i);
    Serial.println (cap); 
    Serial.println (atol(cap), DEC);
    }  // end of for each capture
#endif
}  // end of match_callback 

void setup ()
{
  Serial.begin (9600);
  cptSerial.begin(1200);

  Serial.println ();

  rf12_initialize(nodeID, freq, networkGroup);    // initialize RFM12B
  rf12_sleep(RF12_SLEEP);

  pinMode(LEDpin, OUTPUT);                        // Setup indicator LED
  digitalWrite(LEDpin, HIGH);
  
  if (UNO) wdt_enable(WDTO_8S);                   // Enable anti crash (restart) watchdog if UNO bootloader is selected. Watchdog does not work with duemilanove bootloader                                                             // Restarts emonTx if sketch hangs for more than 8s

}  // end of setup  

void loop () {
unsigned long count;
char value [10]; 

  /* Monitor the baterry of emontx */
  emontx.battery = ct1.readVcc();                                      //read emonTx battey (supply voltage)
  Serial.print(" "); Serial.print(emontx.battery);
  Serial.println(); delay(100);

#if 0
  /* Read a teleinfo frame */
  while(receivedChar != 0x02) // Wait for the beginning of the frame
  {
    if (cptSerial.available())
   //   Serial.write(cptSerial.read() & 0x7F);
      receivedChar = cptSerial.read() & 0x7F;   
  }
  i = 0;
  while(receivedChar != 0x03) // 
  {
    if (cptSerial.available()) {
   //   Serial.write(cptSerial.read() & 0x7F);
      receivedChar = cptSerial.read() & 0x7F;
      frame[i++] = receivedChar;
    }  
  }
  frame[i++]='\0';
#endif
  
  /* Match state object */
  MatchState ms;
  
  /* string we are searching in */
  ms.Target (frame);

  /* Init the TxPayload Structure */
  emontx.hchc = 0;
  emontx.hchp = 0;
  emontx.imax1 = emontx.imax2= emontx.imax3 = 0;
  emontx.iinst1 = emontx.iinst2= emontx.iinst3 = 0;
  emontx.papp = emontx.papp = 0;
  
  ms.Match ("HCHC%s+(%d+)");
  ms.GetCapture(value, 0);
  emontx.hchc = (unsigned int)(atol(value)/1000);      
  
  ms.Match ("HCHP%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.hchp = (unsigned int)(atol(value)/1000);      
  
  ms.Match ("IINST1%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.iinst1 = atoi(value);      
    
  ms.Match ("IINST2%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.iinst2 = atoi(value);      
    
  ms.Match ("IINST3%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.iinst3 = atoi(value);      

  ms.Match ("IMAX1%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.imax1 = atoi(value);      
    
  ms.Match ("IMAX2%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.imax2 = atoi(value);      
    
  ms.Match ("IMAX3%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.imax3 = atoi(value);      

  ms.Match ("PMAX%s+(%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.pmax = atoi(value);      
  
  ms.Match ("PAPP (%d+)", 0);
  ms.GetCapture(value, 0);
  emontx.papp = atol(value);      
  
  // show results
  Serial.print ("Found ");
  //Serial.print (count);            // 8 in this case
  //Serial.println (" matches.");

  // original buffer
  //Serial.println (frame);

  //count = ms.GlobalMatch ("(%a+)%s+(%d+)", match_callback);

  // because millis() returns to zero after 50 days ! 
  if (!settled && millis() > FILTERSETTLETIME) settled = true;

  if (settled)                                                            // send data only after filters have settled
  { 
    send_rf_data();                                                       // *SEND RF DATA* - see emontx_lib
    digitalWrite(LEDpin, HIGH); delay(2); digitalWrite(LEDpin, LOW);      // flash LED
    emontx_sleep(5);                                                      // sleep or delay in seconds - see emontx_lib
  }
}

