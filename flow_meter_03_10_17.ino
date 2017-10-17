#include <LiquidCrystal.h>
#include "SIM900.h"          //Include library for SIM900 GSM modem
#include "inetGSM.h"        //Include library for GPRS
#include "sms.h"            //include library for SMS
#include "call.h"
#include <SoftwareSerial.h>
#include <DS1307.h>
#include<Wire.h>

double litres;        //Serial 0 value in website
String Litre_Str;          //Litre string
unsigned long litres_long=0;
String lit_total;           //Sum value String
//double Sum;                  //Sum of litres
unsigned long Sum = 0;
char Litre_chr[40];
unsigned long units;

int flag=1;              //One time execution flag for manipulation  
int condition=0;           
String Date;
String T_ime;
String DAT_M;
String MONTH;
String YEAR;
String Correct_DATE;
//char Number[]="+918682846199";
unsigned long PreviousTime=0;
unsigned long CurrentTime=0;
unsigned long  sensorValue = 0;        // value read from the pot


DS1307 rtc(SDA, SCL);
InetGSM inet;              //Instatnce of GPRS
SMSGSM sms;                //Instance of SMS
CallGSM notify;

boolean started = false; 

int devID = 4;
int resetPin = 12;

LiquidCrystal lcd(28, 32, 42, 44, 46, 48);


void setup() 
{
  digitalWrite(resetPin,HIGH);
  delay(20000);
  Serial.begin(9600);
  Serial.println("Setup_begin");
  pinMode(22,OUTPUT);   //VSS
  digitalWrite(22,LOW);
  pinMode(24,OUTPUT);   //Vcc
  digitalWrite(24,HIGH);
  pinMode(26,OUTPUT);   //Vee
  digitalWrite(26,LOW);
  pinMode(30,OUTPUT);   //R/W
  digitalWrite(30,LOW); 
  pinMode(34,OUTPUT);   //D0
  digitalWrite(34,LOW); 
  pinMode(36,OUTPUT);   //D1
  digitalWrite(36,LOW); 
  pinMode(38,OUTPUT);   //D2
  digitalWrite(38,LOW);
  pinMode(40,OUTPUT);   //D3
  digitalWrite(40,LOW);
  pinMode(50,OUTPUT);   //LED+
  digitalWrite(50,HIGH);
  pinMode(52,OUTPUT);   //LED-
  digitalWrite(52,LOW);
  
  pinMode(resetPin,OUTPUT);
  digitalWrite(resetPin,HIGH);
  
  lcd.begin(16, 2);
  
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("SALIEABS");
  lcd.setCursor(0, 1); 
  lcd.print("  SMART  METER  ");
  delay(5000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting GSM");
  startmodem();
  lcd.setCursor(0, 1); 
  if(started==true)
  {
    lcd.print("GSM Success...");
  }
  else
  {
    lcd.print("GSM Fail...");
    digitalWrite(resetPin,LOW);
  }
  delay(5000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting RTC...");
  rtc.begin();
  rtc.halt(false);
  delay(5000);
  
  Serial.println("Setup_End");
  //delay(10000);
}

void loop() 
{
  flowCal();
  RTC();
  print2Serial();
  showLCD(litres_long,units);
  CurrentTime=millis();
  if(PreviousTime == 0 || CurrentTime-PreviousTime>=43200000)             //3600000 for 1 hour //43200000 for half day  //86400000 for one day 
  {
    startmodem();
    send2Falcon();
    PreviousTime=millis();
  }
}

void startmodem(void)
{
  if (gsm.begin(2400))                      //Begin GSM modem at 2400 bits per second
  {
    Serial.println("READY");                //If modem responds, print ready and update 'started' variable.
    started = true;
  }
  else
  {
    Serial.println("IDLE");                //If modem does not respond, print "IDLE"
  }
}



void flowCal()
{
  sensorValue = analogRead(A0);
  if (sensorValue <=750)
  {
    flag++;
    if(flag==1)
    {
      litres_long=litres_long+1;
      Litre_Str=String(litres_long);
      units=(litres_long/1000);
    }

  }
  else
  {
    flag=0;
    Litre_Str=String(litres_long);
  }
}

void print2Serial()
{
  Serial.println(Correct_DATE);
  Serial.print('\t');
  Serial.print(T_ime);
  Serial.print('\t');
  //Serial.print(FlowFrequency);
  Serial.print('\t');
  Serial.print(litres_long);
  Serial.print('\t');
  //Serial.println(lit_total);
}

void showLCD(long thisSample,unsigned long cumulative)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  //lcd.print("SMART METER");
  lcd.print(thisSample);
  lcd.setCursor(14, 0);
  lcd.print("lt");
  lcd.setCursor(0, 1);
  lcd.print("UNITS:");
  lcd.print(cumulative);
}

void RTC()
{ 
  Date =rtc.getDateStr();
  DAT_M= Date.substring(0, 2);
  MONTH= Date.substring(3, 5);
  YEAR = Date.substring(6, 10);
  Correct_DATE=DAT_M +"/"+MONTH +"/"+YEAR;

  //Serial.println(Correct_DATE);
  T_ime =rtc.getTimeStr();
  //Serial.println(T_ime);
}

void send2Falcon()
{ 
  int numdata = 0;
  char msg[10];

  if (started)
  {
    //GPRS attach, put in order APN, username and password.
    //If no needed auth let them blank.
    if (inet.attachGPRS("airtelgprs.com", "", ""))
      Serial.println("ATT");
    else
      Serial.println("ERR");
    delay(1000);

    //Read IP address.
    gsm.SimpleWriteln("AT+CIFSR");
    delay(5000);
    //Read until serial buffer is empty.
    gsm.WhileSimpleRead();

    //Prepare the REST API string by adding PPS in analog0 value and valve status in digital0 value
    String jsn = "/json/AddTrackDetails.aspx?deviceid=";
    jsn = jsn + String(devID);
    jsn = jsn + "&datetime=";
    jsn = jsn + Correct_DATE;
    jsn = jsn + "%20";
    jsn = jsn + T_ime;
    jsn = jsn + "&analog0=";
    jsn = jsn + Litre_Str;
    jsn = jsn + "&analog1=";
    jsn = jsn + String(units);
    jsn = jsn  +"&analog2=0&analog3=0&analog4=0&analog5=0&digital0=0";
    jsn = jsn + "&digital1=0&digital2=0&digital3=0&digital4=0&digital5=0";
    jsn = jsn +"&serial0=0";
    // jsn = jsn + Litre_Str;
    jsn = jsn +"&serial1=0";
    jsn = jsn +"&serial2=0&serial3=0&latitude=+11.6550853&longitude=+78.1531832&keypad=0&customstring=SCMC";

    numdata = inet.httpGET("track.salieabs.com", 80, &jsn[0], msg, 10);

    //Print the results.
    Serial.print("JSON: ");
    Serial.println(jsn);
    Serial.println("size:");
    Serial.println(numdata);
    Serial.println("Data:");
    //Serial.println(msg);
  }
}




