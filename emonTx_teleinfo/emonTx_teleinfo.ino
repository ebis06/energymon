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

typedef struct { int power1, power2, power3, battery; } PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;                                                       

const int LEDpin = 9;                                                   // On-board emonTx LED 

boolean settled = false;
SoftwareSerial cptSerial(5, 6);

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

  unsigned long count;

  // what we are searching (the target)
  //char buf [100] = "The quick brown fox jumps over the lazy wolf";
  char buf [100] = "ADCO 524 /";
  // match state object
  MatchState ms (buf);

  // original buffer
  Serial.println (buf);

  // search for three letters followed by a space (two captures)
//  count = ms.GlobalMatch ("(%a+)( )", match_callback);

  count = ms.GlobalMatch ("(%a+)%s+(%d+)", match_callback);

  // show results
  Serial.print ("Found ");
  Serial.print (count);            // 8 in this case
  Serial.println (" matches.");

}  // end of setup  

void loop () {

  /* Monitor the baterry of emontx */
  emontx.battery = ct1.readVcc();                                      //read emonTx battey (supply voltage)
  Serial.print(" "); Serial.print(emontx.battery);
  Serial.println(); delay(100);

  /* Read teleinfo */
  if (cptSerial.available())
    Serial.write(cptSerial.read() & 0x7F);

  // because millis() returns to zero after 50 days ! 
  if (!settled && millis() > FILTERSETTLETIME) settled = true;

  if (settled)                                                            // send data only after filters have settled
  { 
    send_rf_data();                                                       // *SEND RF DATA* - see emontx_lib
    digitalWrite(LEDpin, HIGH); delay(2); digitalWrite(LEDpin, LOW);      // flash LED
    emontx_sleep(5);                                                      // sleep or delay in seconds - see emontx_lib
  }
}
