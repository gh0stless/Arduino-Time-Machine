struct Daten {
        String strTemp;
        String strLuftfeuchte;
        String strLuftguete;
        String strDruck;
        String strZeit;
} dat;

void daten_eingang(){
    if (client.available()) {
      char c = client.read();
      //Serial.print(c);
    }

    if (!(client.connected())) {
      //Serial.println();
      //Serial.println("disconnecting.");
      client.stop();
    }
}

void sende_request(void){
  //Serial.println("connecting...");
  if (client.connect(serverIP, 80)){
    //Serial.println("connected..");
    //Serial.println(client.remoteIP());
    client.println(getErsterTeil + "pAuthServer=" + pAuthServer + "&temperatur=" + dat.strTemp + "&Luftfeuchtigkeit=" + dat.strLuftfeuchte + "&Luftguete=" + dat.strLuftguete + "&luftdruck=" + dat.strDruck + "&zeitstempel=" + dat.strZeit + getLetzterTeil);
    client.println(adrHost);
    client.println();
    AnfrageVerzoegerung = true;
    AnfrageVerzoegerungZeit = millis();

  }
  else {
    if(zaehler<5){
      zaehler++;
      //delay(500);
      sende_request();
    }
    zaehler = 0;
    //Serial.println("connection failed");
  }
}

void makeHTMLrequest(void){
   // Sensordaten lesen
    
      dat.strTemp = String(myBosch.temperature);
      dat.strLuftfeuchte = String(myBosch.humidity);
      dat.strLuftguete = String(myBosch.iaq);
      dat.strDruck = String(myBosch.pressure);
      dat.strZeit =  String(myTZ.toUTC(now()));
      sende_request();

}
