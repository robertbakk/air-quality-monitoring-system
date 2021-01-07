#include <SoftwareSerial.h>
#include <TimerOne.h>
#include <MQUnifiedsensor.h>
#include "MQ135.h"
#include "DHT.h"

#define DHTPIN A3 //pinul la care este conectat senzorul DHT11
#define DHTTYPE DHT11 //tipul senzorului DHT
#define MQ135PIN A0 //pinul la care este conectat senzorul MQ135
#define MQ9PIN A1 //pinul la care este conectat senzorul MQ9

#define Board "Arduino UNO"
#define MQ9Type "MQ-9"
#define Voltage_Resolution 5
#define ADC_Bit_Resolution 10
#define RatioMQ9CleanAir 9.6 //ratia senzorului MQ in aer

#define NO_EMERGENCY -1 //nicio urgenta
#define EMERGENCY_CO2 0 //nivel de urgenta pentru CO2
#define EMERGENCY_CH4 1 //nivel de urgenta pentru CH4
#define EMERGENCY_CO 2 //nivel de urgenta pentru CO
#define NUMAR_TELEFON "+40746842759" //numarul de telefon pe care se primesc SMS-urile

SoftwareSerial mySerial(3, 2); //RX, TX
DHT dht(DHTPIN, DHTTYPE); // declarare senzor DHT11
MQ135 mq135 = MQ135(MQ135PIN); //initializare senzor MQ135
MQUnifiedsensor MQ9(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ9PIN, MQ9Type); //initializare senzor MQ9
float h, t; //umiditate si temperatura
float MQ9_LPG, MQ9_CH4, MQ9_CO, MQ135_CO2; //valorile in ppm de LPG, CH4, CO si CO2
volatile int nrSecunde[3] = {300, 300, 300}; //nr de secunde care au trecut de la ultimele SMS-uri cu un anumit nivel de urgenta
                                            //nrSecunde[EMERGENCY_CO2], nrSecunde[EMERGENCY_CH4], nrSecunde[EMERGENCY_CO]
String message; //mesajul care va fi trimis prin SMS

void setup() {
  Timer1.initialize(1000000); //se initializeaza un timer care are intrerupere la fiecare secunda
  Timer1.attachInterrupt(function); //se ataseaza functia function timer-ului
  pinMode(MQ9PIN, INPUT); //seteaza modul pinului MQ9PIN ca INPUT
  pinMode(MQ135PIN, INPUT); //seteaza modul pinului MQ135PIN ca INPUT
  dht.begin(); //initializare senzor DHT11
  Serial.begin(9600); //initializare serial monitor
  mySerial.begin(9600); //initializare software serial

  MQ9.setRegressionMethod(1); //functie exponentiala

  float calcR0MQ9 = 0;
  float calcR0MQ135 = 0;

  //se face calibrarea pentru senzorii MQ9 si MQ135
  for (int i = 0; i < 10; i++)
  {
    MQ9.update();
    h = dht.readHumidity();
    t = dht.readTemperature();
    calcR0MQ9 += MQ9.calibrate(RatioMQ9CleanAir);
    calcR0MQ135 += mq135.getCorrectedRZero(t, h);
  }

  MQ9.setR0(calcR0MQ9 / 10); //se seteaza R0 pentru MQ9
  mq135.setR0(calcR0MQ135 / 10); //se seteaza R0 pentru MQ135

  //configurare software serial
  mySerial.println("AT");
  delay(200);
  mySerial.println("AT+CMGF=1"); //configurare mod SMS TEXT
  delay(200);
  mySerial.println("AT+CNMI=1,2,0,0,0");
}
void loop() {
  checkForSMS();
  h = dht.readHumidity();
  t = dht.readTemperature();

  MQ9.update(); //se face citirea de pe senzor

  //se seteaza parametrii a si b ai ecuatiei ppm = a * (RS/R0)^b
  //RS = val. rezistentei in prezenta unui gaz (val. calculata in functie de val. citita de senzor, Voltage_Resolution, si ADC_Bit_Resolution)
  //R0 = val. rezistentei in aerul curat

  MQ9.setA(1000.5); MQ9.setB(-2.186); //parametrii ecuatiei pentru calculul LPG
  MQ9_LPG = MQ9.readSensor(); //se interpreteaza valoarea citita de pe senzor cu parametrii setati pentru calculul ppm de LPG

  MQ9.setA(4269.6); MQ9.setB(-2.648); //parametrii ecuatiei pentru calculul CH4
  MQ9_CH4 = MQ9.readSensor(); //se interpreteaza valoarea citita de pe senzor cu parametrii setati pentru calculul ppm de CH4

  MQ9.setA(599.65); MQ9.setB(-2.244); //parametrii ecuatiei pentru calculul CO
  MQ9_CO = MQ9.readSensor(); //se interpreteaza valoarea citita de pe senzor cu parametrii setati pentru calculul ppm de CO

  //ppm de CO2 avand in vedere temperatura si umiditatea
  MQ135_CO2 = mq135.getCorrectedPPM(t, h);
  delay(100);

}

//daca e urgenta, verifica sa fi trecut 300 de secunde de la ultimul SMS de urgenta
void sendSMS(String message, int emergencyLevel) {
  if (emergencyLevel != NO_EMERGENCY && nrSecunde[emergencyLevel] > 300) {
    nrSecunde[emergencyLevel] = 0;
    mySerial.println("AT+CMGS=\"" + String(NUMAR_TELEFON) + "\""); //configurarea numarului de telefon
    delay(1000);
    mySerial.print(message); //scrie mesajul in SMS
    delay(100);
    mySerial.write(26);
    Serial.println("\nEMERGENCY MESSAGE SENT!");
    delay(1000);
  }
  if (emergencyLevel == NO_EMERGENCY) {
    mySerial.println("AT+CMGS=\"" + String(NUMAR_TELEFON) + "\""); //configurarea numarului de telefon
    delay(1000);
    mySerial.print(message); //scrie mesajul in SMS
    delay(100);
    mySerial.write(26);
    Serial.println("\nSTATUS MESSAGE SENT!");
  }
}

void function() {
  nrSecunde[EMERGENCY_CH4]++; //creste nr de secunde care au trecut de la ultimul SMS trimis pentru nivelul de urgenta EMERGENCY_CH4
  nrSecunde[EMERGENCY_CO]++; //creste nr de secunde care au trecut de la ultimul SMS trimis pentru nivelul de urgenta EMERGENCY_CO
  nrSecunde[EMERGENCY_CO2]++; //creste nr de secunde care au trecut de la ultimul SMS trimis pentru nivelul de urgenta EMERGENCY_CO2

  //printeaza in Serial Monitor
  Serial.println("\nTemperature / Humidity: " + String(t) + "/" + String(h) + "%");
  Serial.println("MQ135 CO2: " + String(MQ135_CO2) + " ppm");
  Serial.println("MQ9 LPG: " + String(MQ9_LPG) + "    CH4: " + String(MQ9_CH4) + "    CO:" + String(MQ9_CO));

  //trimite SMS in caz de depasire nivel CH4
  if (MQ9_CH4 > 50)
    sendSMS("Possible gas leak! - CH4 PPM: " + String(MQ9_CH4), EMERGENCY_CH4);

  //trimite SMS in caz de depasire nivel CO
  if (MQ9_CO > 150)
    sendSMS("Possible fire! - CO PPM: " + String(MQ9_CO), EMERGENCY_CO);

  //trimite SMS in caz de depasire nivel CO2
  if (MQ135_CO2 > 500)
    sendSMS("You should open a window. - CO2 PPM: " + String(MQ135_CO2), EMERGENCY_CO2);

}

void checkForSMS()
{
  //verifica daca exista informatie primita ca SMS
  if (mySerial.available())
  {
    String sms = mySerial.readString(); //citeste SMS-ul primit
    Serial.println(sms);
    if (sms.indexOf("status") > 0) //verifica daca in SMS exista "status"
    {
      message = "Temperature / Humidity: " + String(t) + "/" + String(h) + "%\n"
                + "MQ135 CO2: " + String(MQ135_CO2) + " ppm\n"
                + "MQ9 LPG: " + String(MQ9_LPG) + "    CH4: " + String(MQ9_CH4) + "    CO: " + String(MQ9_CO);
      sendSMS(message, NO_EMERGENCY); //trimite SMS cu statusul, fara nivel de urgenta
    }
  }
}
