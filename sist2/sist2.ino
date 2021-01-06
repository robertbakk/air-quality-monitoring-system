#include <SoftwareSerial.h>
#include <TimerOne.h>
#include <MQUnifiedsensor.h>
#include "MQ135.h"
#include "DHT.h"

#define DHTPIN A3 //pinul la care este conectat senzorul DHT11
#define DHTTYPE DHT11
#define MQ135PIN A0 //pinul la care este conectat senzorul MQ135
#define MQ9PIN A1 //pinul la care este conectat senzorul MQ9
#define MQ2PIN A2 //pinul la care este conectat senzorul MQ2

#define Board ("Arduino UNO")
#define MQ2Type ("MQ-2")
#define MQ9Type ("MQ-9")
#define Voltage_Resolution (5)
#define ADC_Bit_Resolution (10)
#define RatioMQ9CleanAir (9.6)
#define RatioMQ2CleanAir (9.83)

SoftwareSerial mySerial(3, 2);
DHT dht(DHTPIN, DHTTYPE);
MQ135 mq135 = MQ135(MQ135PIN);
float h, t;
float MQ9_LPG, MQ9_CH4, MQ9_CO, MQ135_CO2;
int nrSecunde = 300;
bool messageSent = false;
MQUnifiedsensor MQ9(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ9PIN, MQ9Type);

void setup() {
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(function);
  dht.begin();
  Serial.begin(9600);
  mySerial.begin(9600);

  MQ9.setRegressionMethod(1);
  MQ9.init();

  float calcR0MQ9 = 0;
  for (int i = 0; i < 10; i++)
  {
    MQ9.update();
    calcR0MQ9 += MQ9.calibrate(RatioMQ9CleanAir);
  }

  MQ9.setR0(calcR0MQ9 / 10);

  h = dht.readHumidity();
  t = dht.readTemperature();

  mq135.setR0(mq135.getCorrectedRZero(t, h));

  mySerial.println("AT");
  delay(200);
  mySerial.println("AT+CMGF=1");
  delay(200);
  mySerial.println("AT+CNMI=1,2,0,0,0");
  delay(200);
}
void loop() {
  updateSerial();
  h = dht.readHumidity();
  t = dht.readTemperature();

  MQ9.update();

  MQ9.setA(1000.5); MQ9.setB(-2.186);
  MQ9_LPG = MQ9.readSensor();

  MQ9.setA(4269.6); MQ9.setB(-2.648);
  MQ9_CH4 = MQ9.readSensor();

  MQ9.setA(599.65); MQ9.setB(-2.244);
  MQ9_CO = MQ9.readSensor();

  MQ135_CO2 = mq135.getCorrectedPPM(t, h);

}

void sendSMS(String message) {
  if (nrSecunde > 300) {
      mySerial.println("AT+CMGS=\"+40752173878\"");
      delay(100);

      mySerial.print(message);
      delay(100);
      mySerial.write(26);
      Serial.println("MESSAGE SENT!");
      nrSecunde = 0;
  }
}

void function() {
  nrSecunde++;

  Serial.print("Temperature / Humidity: ");
  Serial.print(t);
  Serial.print("/");
  Serial.print(h);
  Serial.println("%");

  Serial.print("MQ135 CO2: ");
  Serial.print(MQ135_CO2);
  Serial.println(" ppm");

  Serial.print("MQ9 LPG: ");
  Serial.print(MQ9_LPG);
  Serial.print("    CH4: ");
  Serial.print(MQ9_CH4);
  Serial.print("    CO: ");
  Serial.println(MQ9_CO);

  if (MQ9_CH4 > 50)
    sendSMS("Possible gas leak! - CH4 PPM: " + String(MQ9_CH4));

  if (MQ9_CO > 150)
    sendSMS("Possible fire! - CO PPM: " + String(MQ9_CO));
  else {
    if (MQ135_CO2 > 500)
      sendSMS("You should open a window. - CO2 PPM: " + String(MQ135_CO2));
  }

}

void updateSerial()
{
  if (mySerial.available())
  {
    String sms = mySerial.readString();
    Serial.println(sms);
    if (sms.indexOf("status") > 0)
    {
      mySerial.println("AT+CMGS=\"+40752173878\"");
      delay(100);

      mySerial.print("Temperature / Humidity: ");
      mySerial.print(t);
      mySerial.print("/");
      mySerial.print(h);
      mySerial.println("%");

      mySerial.print("MQ135 CO2: ");
      mySerial.print(MQ135_CO2);
      mySerial.println(" ppm");

      mySerial.print("MQ9 LPG: ");
      mySerial.print(MQ9_LPG);
      mySerial.print("    CH4: ");
      mySerial.print(MQ9_CH4);
      mySerial.print("    CO: ");
      mySerial.println(MQ9_CO);
      delay(100);
      mySerial.write(26);
      Serial.println("MESSAGE SENT!");

    }
  }
}
