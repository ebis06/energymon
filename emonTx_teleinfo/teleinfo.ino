#ifndef REGEXP
#define REGEXP
#include <Regexp.h>
#endif

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
  
int findInMatch(MatchState* ms, char* label, char* matchPat, char* value)
{
char regex [24] = "";
char checksum[8] = "";
char temp[8] = "";
char result;
unsigned int sum = 0;		// Somme des codes ASCII du message + un espace
unsigned char i = 0;
  
  /* Build a string with the label and the pattern */   
  strcpy(regex, label);		
  strcat(regex, matchPat);  //  strcat(regex,  ".([0-9]+)(.)(.)");
  
  /* Match the label + pattern */
  result = ms->Match (regex);
  
  if ( result == REGEXP_MATCHED)
  {
#if DEBUG  
    Serial.println(regex);
#endif
    ms->GetCapture(value, 0);      /* Get the value associated with the label */
    ms->GetCapture(checksum, 2);   /* Get the checksum */
#if DEBUG  
    Serial.print(label);Serial.print(":"); Serial.println(value);
    Serial.print("CKSUM:"); Serial.println(checksum);
#endif

    /* Compute the checksum */
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
#if (READ_TELEINFO)
      Serial.print("Found correct "); Serial.print(label);Serial.print(" with value ");Serial.println(value);
#endif
      return 1;
    }  
    else {
#if (READ_TELEINFO)
      Serial.print("Found incorrect checksum for "); Serial.print(label);Serial.print(" with value ");Serial.print(value);Serial.print(" and checksum ");Serial.println(sum);
#endif
      return 0;
    }
  }
  else {
      Serial.print("No match for "); Serial.println(label);
      return 0;
  }
}
  

