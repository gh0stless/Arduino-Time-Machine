/**************************************************************************************************
** Display time and sensor data on the LCD                                                       **
***************************************************************************************************/

void printLCDDigits(int digits);

void LCDClockDisplay(){
    // digital clock display of the time on LCD
    lcd.setCursor(0, 0);
    printLCDDigits(hour());
    lcd.print(":");
    printLCDDigits(minute());
    lcd.print(":");
    printLCDDigits(second());
    lcd.setCursor(0, 1);
    printLCDDigits(day());
    lcd.print(".");
    printLCDDigits(month());
    lcd.print(".");
    
    if (DisplayModus){
      switch (weekday()){
        case 2:
          lcd.print("MON");
          break;
        case 3:
          lcd.print("TUE");
          break;
        case 4:
          lcd.print("WED");
          break;
        case 5:
          lcd.print("THU");
          break;
        case 6:
          lcd.print("FRI");
          break;
        case 7:
          lcd.print("SAT");
          break;
        case 1:
          lcd.print("SUN");
          break;
      }
    }
    
    // Sensordaten anzeigen
    if (DisplayModus){
        output = " " + String(myBosch.temperature) + "\337 ";  
        lcd.setCursor(9, 0);
        lcd.print(output);
        output = String(myBosch.humidity) + "\45";
        lcd.setCursor(10, 1);
        lcd.print(output);
    }
    else{
        output = " " + String(myBosch.iaq) + " ";
        lcd.setCursor(9, 0);
        lcd.print(output);
        output = String((myBosch.pressure),0);
        lcd.setCursor(10, 1);
        lcd.print(output);
    }
    //DCF Symbol, wenn letzter DCF Empfang nicht älter als 24h: 
    if (DCFCounter && ((myTZ.toUTC(now())-lastDCF) <= 86400)){ 
        lcd.setCursor(8,0);  
        lcd.print("\134");
    }
}

// utility function for digital clock display: prints leading 0
void printLCDDigits(int digits){
    if(digits < 10)
      lcd.print('0');
    lcd.print(digits);
}

/**************************************************************************************************
** Speak function                                                                                **
***************************************************************************************************/
void sayTime(){
    if(waiting_synthesis_complete && waiting_play_complete){
      int p_m = 0;
      int m_hour = hour();
      if (m_hour >12){
        m_hour = m_hour - 12;
        p_m = 1;
      }
      if (m_hour == 0) {
        m_hour = 12;
      }
      itoa (m_hour,my_temp_str,10);
      SpeechSynthesis.English(ssr,4,strcpy(buf,"9")); //volume in grade 5
      SpeechSynthesis.English(ssr,2,strcpy(buf,"2")); //speed
      SpeechSynthesis.English(ssr,6,strcpy(buf,my_temp_str));
      SpeechSynthesis.English(ssr,6,strcpy(buf," o' clock and "));
      itoa (minute(),my_temp_str,10);
      SpeechSynthesis.English(ssr,6,strcpy(buf,my_temp_str));
      SpeechSynthesis.English(ssr,6,strcpy(buf,"  minutes"));
      if (p_m) {
        SpeechSynthesis.English(ssr,6,strcpy(buf,"  P M"));
      }  
      else {
        SpeechSynthesis.English(ssr,6,strcpy(buf,"  A M")); 
      }
      SpeechSynthesis.Espeaking(0,19,4,ssr);
      //Executive commands above, "0" is synthesis command; "19" select speaker; "4" speech function
      waiting_synthesis_complete = false;
      waiting_play_complete = false;  
    }
}
