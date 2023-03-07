/*
 * DCFSignalQuality
 * Ralf Bohnen, 2013
 * This example code is in the public domain.
*/
#define DCF77PIN 2
void setup() {
  Serial.begin(9600);
  pinMode(DCF77PIN, INPUT);
  Serial.println("Beginne DCF77 Messung, bitte warten :");                                                       
  //Ein schlechter Empfang bedeutet nicht das wir kein Empfang haben, nur das es länger dauern würde
  //das Signal auszuwerten.
  Serial.println("KEIN SIGNAL <- |                                          <- MISERABEL <- |  SCHLECHT <- |          GUT          | -> SCHLECHT  | -> MISERABEL ->");
}
 
void loop() {
  //Führe eine Messung über 10 Impulse aus, ein Impuls dauert genau eine Sekunde
  int q = DCF77signalQuality(10);
  //Wenn kein Wechsel zwischen HIGH und LOW am Anschluss erkannt wurde
  //bedeutet das in 99,99% aller Fälle das der DCF Empfänger nicht arbeitet
  //denn bei extrem schlechten Empfang hat man Wechsel, nur kann man sie nicht auswerten.
  if (!q) {Serial.print("# (Schaltung pruefen!)");}
  for (int i = 0; i < q; i++) {
    Serial.print(">");
  }
  Serial.println("");
 
}
 
int DCF77signalQuality(int pulses) {
  int prevSensorValue=0;
  unsigned long loopTime = 10000; //Impuls Länge genau eine Sekunde
  //Da wir ja mitten in einem Impuls einsteigen könnten, verwerfen wir den ersten.
  int rounds = -1; 
  unsigned long gagingStart = 0;
  unsigned long waitingPeriod = 0;
  int overallChange = 0;
  int change = 0;
 
  while (true) {
    //Unsere Schleife soll das Eingangssignal (LOW oder HIGH) 10 mal pro
    //Sekunde messen um das sicherzustellen, messen wir dessen Ausführungszeit.
    gagingStart = micros();
    int sensorValue = digitalRead(DCF77PIN);
    //Wenn von LOW nach HIGH gewechselt wird beginnt ein neuer Impuls
    if (sensorValue==1 && prevSensorValue==0) { 
      rounds++;
      if (rounds > 0 && rounds < pulses + 1) {overallChange+= change;}
      if (rounds == pulses) { return overallChange /pulses;}
      change = 0; 
    }
    prevSensorValue = sensorValue;
    change++;
 
    //Ein Wechsel zwichen LOW und HIGH müsste genau alle 100 Durchläufe stattfinden
    //wird er größer haben wir kein Empfang
    //300 habe ich als guten Wert ermittelt, ein höherer Wert würde die Aussage festigen
    //erhöht dann aber die Zeit.
    if (change > 300) {return 0;}
    //Berechnen und anpassen der Ausführungszeit
    waitingPeriod = loopTime - (micros() - gagingStart);
    delayMicroseconds(waitingPeriod);
  }
}
