#ifndef _HOME_ENERGY_MONITOR_
#define _HOME_ENERGY_MONITOR_
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
  unsigned int hchpDec 
} PayloadTX;      // create structure - a neat way of packaging data for RF comms
#endif
