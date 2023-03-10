/**************************************************************************************************
** Meine sprechende Funkuhr/NTP-Server/WWW-Server mit Bewegungsmelder und Umweltsensor.          **
** 2020-09-05 - 2023-03-07                                                                       **
** by Andreas Schumm (gh0stless)                                                                 **
**************************************************************************************************/
#include <Wire.h>                   //für i²C Bus
#include <SPI.h>                    //für Ethernet Shield
#include <Ethernet.h>               //    "
#include <EthernetUdp.h>            //    "
#include <DS1307RTC.h>              //für RTC Uhrenchipmodul
#include <Adafruit_RGBLCDShield.h>  //für RGB Display Shield
#include <TimeLib.h>                //für die "interne" Uhr nicht neuste Version benutzen!
#include <Timezone.h>               //https://github.com/JChristensen/Timezone
#include <DCF77.h>                  //für Funkzeitempfang
#include <SpeechSynthesis.h>        //Sprachsythese Shield
#include <bsec.h>                   //für Bosch BME680 Sensor

/**************************************************************************************************
** Bosch Sensor                                                                                 **
***************************************************************************************************/

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);

// Create an object of the class Bsec
Bsec iaqSensor;
String output;
static char     buf[16];                        // sprintf text buffer

struct Bosch{
  float iaq = 0;
  float rawTemperature = 0;
  float pressure = 0;
  float rawHumidity = 0;
  float gasResistance = 0;
  float stabStatus = 0;
  float runInStatus = 0;
  float temperature = 0;
  float humidity = 0;
  float staticIaq = 0;
  float co2Equivalent = 0;
  float breathVocEquivalent = 0;
  float compGasValue = 0;
  float gasPercentage = 0;
  uint8_t iaqAccuracy = 0;
  uint8_t staticIaqAccuracy = 0;
  uint8_t co2Accuracy = 0;
  uint8_t breathVocAccuracy = 0;
  uint8_t compGasAccuracy = 0;
  uint8_t gasPercentageAcccuracy = 0;
} myBosch;

#define TemperaturSensorAbweichung -0.8

/**************************************************************************************************
** Zeit                                                                                          **
***************************************************************************************************/
// Mitteleuropäische Zeit
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120};    // Daylight time = UTC + 2 hours
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, 60};      // Standard time = UTC + 1 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        // pointer to the time change rule, use to get TZ abbrev

//DCF77 stuff
#define DCF_PIN 3          // Anschlusspin des DCF 77 Gerätes
#define DCF_INTERRUPT 1    // Interruptnummer, die zu diesem Pin gehört
DCF77 DCF = DCF77(DCF_PIN,DCF_INTERRUPT);
#define DCF_ON_OFF_PIN 8

uint32_t timestamp = 0;
uint32_t tempval = 0;
uint32_t lastDCF = 0;
tmElements_t tm;
uint64_t DCFCounter = 0;

/**************************************************************************************************
** Display                                                                                       **
***************************************************************************************************/
//Farben für Hintergrundbeleuchtung
#define WHITE 0x0 
#define TEAL 0x1
#define VIOLET 0x2
#define BLUE 0x3
#define YELLOW 0x4
#define GREEN 0x5
#define RED 0x6
#define BL_OFF 0x7

//RGB-LCD Shield
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

uint8_t farbe = WHITE;
time_t prevDisplay = 0; // when the digital clock was displayed
bool DisplayModus = true;      // display modus
uint8_t memSecond =0;

/**************************************************************************************************
** Netzwerk                                                                                          **
***************************************************************************************************/
// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0 };  
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

//Time Server Port
#define NTP_PORT 123

static const int NTP_PACKET_SIZE = 48;
// buffers for receiving and sending data
byte packetBuffer[NTP_PACKET_SIZE]; 

// An Ethernet UDP instance 
EthernetUDP Udp;

//WWW Server
EthernetServer server(80); // (port 80 is default for HTTP)

//WWW Client
const byte serverIP[] = {82,165,32,210};
const String getErsterTeil =  "GET /arduino.php?";
const String getLetzterTeil = " HTTP/1.0";
const String adrHost = "Host: 369arduino.uwe-bednara.com"; 
const String pAuthServer = "kenj95jag1jak56";
int zaehler = 0;
bool AnfrageVerzoegerung = false;
long AnfrageVerzoegerungZeit = 0;
EthernetClient client;
bool onlyOneRequest = false;


/**************************************************************************************************
** PIR Sensor (Bewegungsmelder)                                                                  **
***************************************************************************************************/
#define PIR_SENSOR_PIN  5
boolean active = false;
int movement = 0;

/**************************************************************************************************
** Sprachsynthese                                                                                **
***************************************************************************************************/
byte ssr[500];
bool waiting_synthesis_complete = true;
bool waiting_play_complete = true;
char my_temp_str[3];
bool boottime = true;


/**************************************************************************************************
** Prototypen & includes                                                                                  **
***************************************************************************************************/
#include "HTML_ANFRAGE.h"
#include "display.h"
#include "netzwerk.h"


/**************************************************************************************************
** setup function                                                                                **
***************************************************************************************************/
void setup() {
  Serial.begin(9600);
  Ethernet.init(10);  // Most Arduino shields

  Wire.begin();
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  checkIaqSensorStatus();
  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };
  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();
  
  pinMode(DCF_ON_OFF_PIN, OUTPUT);    
  digitalWrite(DCF_ON_OFF_PIN, LOW);  //(DCF77-Modul ON)
  pinMode(PIR_SENSOR_PIN, INPUT);

  //start the Ethernet and UDP:
  Ethernet.begin(mac, ip, myDns);
  delay(1000);
  server.begin();
  Udp.begin(NTP_PORT);
  delay(2000);
  lcd.begin(16, 2); // set up the LCD's number of columns and rows: 
  for ( farbe; farbe<7; farbe++) {lcd.setBacklight(farbe); delay(250);} //Lightshow
  farbe = WHITE;
  lcd.setBacklight(BL_OFF);
  
  DCF.Start();
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) {
    tm.Hour = 1;
    tm.Minute = 0;
    tm.Second = 0;
    tm.Day = 1;
    tm.Month = 1;
    tm.Year = CalendarYrToTm(2000);
    RTC.write(tm);
  }
}

/**************************************************************************************************
** loop function                                                                                 **
***************************************************************************************************/
void loop() {
    SpeechSynthesis.buf_init(ssr);//initialize the buff
    
    time_t DCFtime = DCF.getTime(); // Check if new DCF77 time is available
    if (DCFtime!=0)
    {
      setTime(DCFtime); //Setze Systemzeit
      RTC.set(DCFtime); //Setze RTC-Zeit
      lastDCF = myTZ.toUTC(now());
      DCFCounter++;
      if (DCFCounter == 1){
        saySync();
       }
    }
    
    timestamp = 2208988800UL + myTZ.toUTC(now()); //Sekunden seit 1.1.1900, mit Hilfe der timezone Bibliothek
    if (boottime){
      sayTime();
      boottime = false;
    }
    processNTP(); //bediene Zeitserver
    processWWW(); //bediene WWW Server

    //Wenn Taste sprich Zeit
    uint8_t buttons = lcd.readButtons();
    if (buttons) {
      if (buttons & BUTTON_SELECT) {
        sayTime();
      }
    }
    
    //Bewgungsmelder
    movement = digitalRead(PIR_SENSOR_PIN);
      // eine neue Bewegung wurde erkannt
    if(movement == HIGH && active == false){
      active = true;
      lcd.setBacklight(farbe);
    }
      // keine Bewegung nachdem eine Bewegung erkannt wurde
    if(movement == LOW && active == true){
      active = false;
      lcd.setBacklight(BL_OFF);
    }
    
    // Sensordaten lesen
    if (iaqSensor.run()) { // If new data is available
      myBosch.pressure = (iaqSensor.pressure/pow((1-0.0065*118.0/282.65),(0.03416/0.0065))); //mit 9,5° Durschnitt in Coswig(in Kelvin), 118m Höhe
      myBosch.iaq = iaqSensor.iaq;
      myBosch.temperature = iaqSensor.temperature + TemperaturSensorAbweichung;
      myBosch.humidity = iaqSensor.humidity;
      myBosch.co2Equivalent = iaqSensor.co2Equivalent;
    }
    else {
      checkIaqSensorStatus();
    }

    // Farbe nach Luftgüte setzen  
    int myIAQ = (int)myBosch.iaq;
    int oldFarbe = farbe;
    if ( myIAQ >=   0 && myIAQ <=  50 ) {farbe = GREEN;}
    if ( myIAQ >=  51 && myIAQ <= 100 ) {farbe = BLUE;}
    if ( myIAQ >= 101 && myIAQ <= 150 ) {farbe = YELLOW;}
    if ( myIAQ >= 151 && myIAQ <= 200 ) {farbe = VIOLET;}
    if ( myIAQ >= 201 && myIAQ <= 250 ) {farbe = RED;}
    if ( myIAQ >= 251 && myIAQ <= 350 ) {farbe = RED;}
    if ( myIAQ >= 351 )                 {farbe = RED;}
    
    if (farbe != oldFarbe) lcd.setBacklight(farbe);
  
     
    //Warten auf Ende der Sprachsynthese
    if (!waiting_synthesis_complete){
      if (Serial.read() == 0x41){
        waiting_synthesis_complete = true;
      } 
    }
    else{
      if (!waiting_play_complete){
        if (Serial.read() == 0x4F){
          waiting_play_complete = true;
        }
      }
    }
    //setze Sperre zurück
    if(((minute()%10)==4)  && (second()==0)){
      onlyOneRequest = false;
    }

    //Übermittle alle 10 Minuten Daten an Server
    if (((minute()%10)==3)  && (second()==0) && (!AnfrageVerzoegerung) && (!onlyOneRequest)){
          onlyOneRequest = true;
          makeHTMLrequest();
    }
    if (AnfrageVerzoegerung){
      if ( millis() > AnfrageVerzoegerungZeit + 1000){
        while(client.available()){
          daten_eingang();
        }
        zaehler = 0;
        AnfrageVerzoegerung = false;
      }
    }

    //Display
    //wechsle Modus alle 10 Sekunden
    if ((second()%10) == 0 && (second() != memSecond)) {
      memSecond = second();
      DisplayModus = !DisplayModus;
    }
    
    if (timeStatus() != timeNotSet) {
      if (now() != prevDisplay) { //update the display only if time has changed
        prevDisplay = now();
        LCDClockDisplay();
        
        if (minute()==0 && second()==0){ //Sprachansage zur vollen Stunde
          sayTime();
        }
      }
    }
    
} // loop

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      //Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      //Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      //Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      //Serial.println(output);
    }
  }
}

void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}
  
