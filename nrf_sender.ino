#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_MPU6050.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS     1000

OneWire oneWire(A2);
DallasTemperature sensors(&oneWire);

PulseOximeter pox;
uint32_t tsLastReport = 0;

RF24 radio(7, 8); // CE, CSN
Adafruit_MPU6050 mpu;
const byte address[6] = "00001";


double xLimit=5.0,yLimit=5.0, tLimit=50.0;
float x,y,envTemp,bodyTemp;


String ecgData = "";  
int readingCount = 0; 



void onBeatDetected() {
    Serial.println("♥ Beat!");
}

void setup() {                                                                                                             

    Serial.begin(9600);
    sensors.begin();
    pinMode(A3,INPUT);
    pinMode(A0,INPUT); //ECG Input
    pinMode(2,INPUT);
    pinMode(3,INPUT);
    Serial.print("Initializing pulse oximeter..");

    if (!pox.begin()) { 
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }


  while (!Serial)
    delay(10); 

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");


  delay(100);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  //pox.setIRLedCurrent(MAX30100_LED_CURR_24MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
  
}


float BPM, SpO2;

void loop() {


  sensors.requestTemperatures(); 
  bodyTemp=sensors.getTempCByIndex(0);
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  x=a.acceleration.y;
  y=a.acceleration.z;
  envTemp=temp.temperature;

  pox.update();


  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        Serial.print("Heart rate:");
        Serial.print(pox.getHeartRate());
        Serial.print("bpm / SpO2:");
        Serial.print(pox.getSpO2());
        Serial.println("%  |  "); 
        BPM = pox.getHeartRate();
        SpO2 = pox.getSpO2();
        tsLastReport = millis();
    }

  int thr=5;
  int gest;
  if(x>thr) gest=1;
  else if(x<-thr) gest=2;
  else if(y>thr) gest=3;
  else if(y<-thr) gest=4;
  else gest=0;



  if(!((digitalRead(2) == 1)||(digitalRead(3) == 1))){
   if (ecgData.length() > 0) {
      ecgData += "^";
    }
    ecgData += String(analogRead(A0));
    readingCount++;  

    if (readingCount >= 20) {
      //Serial.println(ecgData);
      ecgData = "";  
      readingCount = 0;  
    }
  }

    
//   String out="G:"+String(gest)+",TE:"+String(envTemp)+",Hr:"+String(pox.getHeartRate())+",OX:"+String(pox.getSpO2())+",EC:"+ecgData+",TB:"+String(bodyTemp);
//    Serial.println(out);
    
    nrfSender("TB",String(bodyTemp));
    delay(20);
    nrfSender("EC",ecgData);
    delay(20);
    nrfSender("Gs",String(gest));
    delay(20);
    nrfSender("Rt",String(envTemp));
    delay(200);
    nrfSender("Ox",String(pox.getSpO2()));
    delay(20);
    nrfSender("Ms",String(digitalRead(A3)));
    delay(90);
  
}


void nrfSender(String prefix, String nrfdata){
  String fullSt = prefix+":"+nrfdata;
  char nrfArray[fullSt.length() + 1];
  fullSt.toCharArray(nrfArray, sizeof(nrfArray)); 
  radio.write(&nrfArray, sizeof(nrfArray));
  Serial.println("Func-"+String(nrfArray));
}
