/*
 EmonTx Teleinfo

 An example sketch for the emontx module for 
 EDF's teleinformation monitoring



 Libraries in the standard arduino libraries folder:
	- JeeLib		https://github.com/jcw/jeelib
	- EmonLib		https://github.com/openenergymonitor/EmonLib.git

*/
#include <SoftwareSerial.h>
#include <avr/wdt.h>                                                     
#include <JeeLib.h> 
ISR(WDT_vect) { Sleepy::watchdogEvent(); 

#include "EmonLib.h"

const int nodeID = 10;

#define freq RF12_433MHZ // Frequency of RF12B module
const int nodeID = 10;   // emonTx RFM12B node ID
const int networkGroup = 210;  // emonTx RFM12B wireless network group -


typedef struct { int power1, power2, power3, battery; } PayloadTX;      // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;                                                       

const int LEDpin = 9;                                                   // On-board emonTx LED 

boolean settled = false;

SoftwareSerial cptSerial(5, 6);

void setup() {
  
        Serial.begin(9600);     // opens serial port, sets data rate to 1200 bps
        cptSerial.begin(1200);
        Serial.println("Hello World");
}

void loop() {

  if (cptSerial.available())
    Serial.write(cptSerial.read() & 0x7F);
}











