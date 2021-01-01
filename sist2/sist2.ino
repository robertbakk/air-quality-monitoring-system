#include <TimerOne.h>
#include <MQUnifiedsensor.h>
#include "MQ135.h"
#include "DHT.h"
#define DHTPIN A3 //pinul la care este conectat senzorul DHT11
#define DHTTYPE DHT11
#define MQ135PIN A0 //pinul la care este conectat senzorul MQ135
#define MQ9PIN A1 //pinul la care este conectat senzorul MQ9
#define MQ2PIN A2 //pinul la care este conectat senzorul MQ2

#define         Board                   ("Arduino UNO")
#define         MQ2Type                    ("MQ-2")
#define         MQ9Type                    ("MQ-9")
#define         Voltage_Resolution      (5)
#define         ADC_Bit_Resolution      (10)
#define         RatioMQ9CleanAir        (9.6)
#define         RatioMQ2CleanAir        (9.83)

DHT dht(DHTPIN, DHTTYPE);
MQ135 mq135 = MQ135(MQ135PIN);
float h,t;
float MQ9_LPG,MQ9_CH4,MQ9_CO,MQ2_LPG,MQ2_CO;
float rzero,correctedRZero,resistance,ppm,correctedPPM;
MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ2PIN, MQ2Type);
MQUnifiedsensor MQ9(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ9PIN, MQ9Type);

void setup(){
  Timer1.initialize(1000000); 
  Timer1.attachInterrupt(function);
  dht.begin();
  Serial.begin(9600);
  MQ2.setRegressionMethod(1);
  MQ9.setRegressionMethod(1);
  MQ2.init();
  MQ9.init();

  float calcR0MQ2 = 0;
  float calcR0MQ9 = 0;
  float calcR0MQ135 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ2.update();
    calcR0MQ2 += MQ2.calibrate(RatioMQ2CleanAir);
    MQ9.update();
    calcR0MQ9 += MQ9.calibrate(RatioMQ9CleanAir);
  }
  MQ2.setR0(calcR0MQ2/10);
  MQ9.setR0(calcR0MQ9/10);
}
void loop(){
  h = dht.readHumidity();
  t = dht.readTemperature();
  MQ2.update();
  MQ9.update();

  MQ9.setA(1000.5); MQ9.setB(-2.186);
  MQ9_LPG = MQ9.readSensor();

  MQ9.setA(4269.6); MQ9.setB(-2.648);
  MQ9_CH4 = MQ9.readSensor();

  MQ9.setA(599.65); MQ9.setB(-2.244);
  MQ9_CO = MQ9.readSensor(); 

  MQ2.setA(574.25); MQ2.setB(-2.222);
  MQ2_LPG = MQ2.readSensor();

  MQ2.setA(36974); MQ2.setB(-3.109);
  MQ2_CO = MQ2.readSensor(); 

  rzero = mq135.getRZero();
  correctedRZero = mq135.getCorrectedRZero(t, h);
  resistance = mq135.getResistance();
  ppm = mq135.getPPM();
  correctedPPM = mq135.getCorrectedPPM(t, h);
  
}

void function() {
  Serial.print("MQ135 RZero: ");
  Serial.print(rzero);
  Serial.print("\t Corrected RZero: ");
  Serial.print(correctedRZero);
  Serial.print("\t Resistance: ");
  Serial.print(resistance);
  Serial.print("\t PPM: ");
  Serial.print(ppm);
  Serial.print("\t Corrected PPM: ");
  Serial.print(correctedPPM);
  Serial.print("ppm @ temp/hum: ");
  Serial.print(t);
  Serial.print("/");
  Serial.print(h);
  Serial.println("%");
  
  Serial.print("MQ9 LPG: ");
  Serial.print(MQ9_LPG);
  Serial.print("\t CH4: ");
  Serial.print(MQ9_CH4);
  Serial.print("\t CO: ");
  Serial.print(MQ9_CO);
  Serial.print("\t temp/hum: ");
  Serial.print(t);
  Serial.print("/");
  Serial.print(h);
  Serial.println("%");
  
  Serial.print("MQ2 LPG: ");
  Serial.print(MQ2_LPG);
  Serial.print("\t CO: ");
  Serial.print(MQ2_CO);
  Serial.print("\t temp/hum: ");
  Serial.print(t);
  Serial.print("/");
  Serial.print(h);
  Serial.println("%");

}
