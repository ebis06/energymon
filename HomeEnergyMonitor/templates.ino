#include "utility/font_helvB18.h"
#include "utility/font_helvB14.h"
#include "utility/font_helvB12.h"
#include "utility/font_clR4x6.h"
#include "utility/font_clR6x6.h"
#include "HomeEnergyMonitor.h"


void draw_raw1_page(PayloadTX *emontx)
{ 
  
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);
  
  char str[50];    			 //variable to store conversion 
  glcd.setFont(font_clR6x6);      
  strcpy(str,"HCHC:");
  glcd.drawString(0,0,str);
  itoa((int)emontx->hchc,str,10);
  strcat(str," kWh");   
  glcd.drawString(70,0,str);
  
  strcpy(str,"HCHP:");
  glcd.drawString(0,9,str);
  itoa((int)emontx->hchp,str,10);
  strcat(str," kWh");   
  glcd.drawString(70,9,str);
  
  strcpy(str,"IINST1: ");
  glcd.drawString(0,18,str);
  itoa((int)emontx->iinst1,str,10);
  strcat(str," A");   
  glcd.drawString(70,18,str);
  
  strcpy(str,"IINST2: ");
  glcd.drawString(0,27,str);
  itoa((int)emontx->iinst2,str,10);
  strcat(str," A");   
  glcd.drawString(70,27,str);
  
  strcpy(str,"IINST3: ");
  glcd.drawString(0,36,str);
  itoa((int)emontx->iinst3,str,10);
  strcat(str," A");   
  glcd.drawString(70,36,str);
          
}

void draw_raw2_page(PayloadTX *emontx)
{ 
  
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);
  
  char str[50];    			 //variable to store conversion 
  glcd.setFont(font_clR6x6);      
  strcpy(str,"IMAX1:");
  glcd.drawString(0,0,str);
  itoa((int)emontx->imax1,str,10);
  strcat(str," A");   
  glcd.drawString(70,0,str);
  
  strcpy(str,"IMAX2:");
  glcd.drawString(0,9,str);
  itoa((int)emontx->imax2,str,10);
  strcat(str," A");   
  glcd.drawString(70,9,str);
  
  strcpy(str,"IMAX3: ");
  glcd.drawString(0,18,str);
  itoa((int)emontx->imax3,str,10);
  strcat(str," A");   
  glcd.drawString(70,18,str);
  
  strcpy(str,"PAPP:");
  glcd.drawString(0,27,str);
  itoa((int)emontx->papp,str,10);
  strcat(str," VA");   
  glcd.drawString(70,27,str);
  
  strcpy(str,"PMAX: ");
  glcd.drawString(0,36,str);
  itoa((int)emontx->pmax,str,10);
  strcat(str," W");   
  glcd.drawString(70,36,str);  
}

void draw_raw3_page(PayloadTX *emontx)
{ 
  
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);
  
  char str[50];    			 //variable to store conversion 
  glcd.setFont(font_clR6x6);      
  strcpy(str,"PTEC:");
  glcd.drawString(0,0,str);
  //itoa((int)emontx->ptec,str,10);
  strcpy(str, emontx->ptec);
  glcd.drawString(70,0,str);
  
  strcpy(str,"Battery: ");
  glcd.drawString(0,9,str);
  itoa((int)emontx->battery,str,10);
  strcat(str," V");   
  glcd.drawString(70,9,str);  
}

//------------------------------------------------------------------
// Draws a page showing a single power and energy value in big font
//------------------------------------------------------------------
void draw_power_page(char* powerstr, double powerval, double powerHc, double powerHp)
{ 
  glcd.clear();
  glcd.fillRect(0,0,128,64,0);
  
  char str[50];    			 //variable to store conversion 
  glcd.setFont(font_clR6x6);      
  strcpy(str,powerstr);  
  strcat(str," NOW:"); 
  glcd.drawString(0,0,str);
  strcpy(str,"HP TODAY:"); 
  glcd.drawString(0,34,str);
  strcpy(str,"HC TODAY:"); 
  glcd.drawString(0,40,str);

  // power value
  glcd.setFont(font_helvB18);
  itoa((int)powerval,str,10);
  strcat(str,"VA");   
  glcd.drawString(3,9,str);
  
  // kwh per day value
  glcd.setFont(font_clR6x6);
  if (powerHp<10.0) dtostrf(powerHp,0,1,str); else itoa((int)powerHp,str,10);
  strcat(str,"kWh");
  glcd.drawString(87,34,str);
  if (powerHc<10.0) dtostrf(powerHc,0,1,str); else itoa((int)powerHc,str,10);
  strcat(str,"kWh");
  glcd.drawString(87,40,str);  
}

//------------------------------------------------------------------
// Draws a footer showing GLCD temperature and the time
//------------------------------------------------------------------
void draw_temperature_time_footer(double temp, double mintemp, double maxtemp, double hour, double minute)
{
  glcd.drawLine(0, 47, 128, 47, WHITE);     //middle horizontal line 

  char str[50];
  // GLCD Temperature
  glcd.setFont(font_helvB12);  
  dtostrf(temp,0,1,str); 
  strcat(str,"C");
  glcd.drawString(0,50,str);  
  
  // Minimum and maximum GLCD temperature
  glcd.setFont(font_clR4x6);             
  itoa((int)mintemp,str,10);
  strcat(str,"C");
  glcd.drawString_P(46,51,PSTR("MIN"));
  glcd.drawString(62,51,str);
               
  itoa((int)maxtemp,str,10); 
  strcat(str,"C");
  glcd.drawString_P(46,59,PSTR("MAX"));
  glcd.drawString(62,59,str);
  
  // Time
  char str2[5];
  itoa((int)hour,str,10);
  if  (minute<10) strcat(str,": 0"); else strcat(str,": ");
  itoa((int)minute,str2,10);
  strcat(str,str2); 
  glcd.setFont(font_helvB12);
  glcd.drawString(82,50,str);

}



