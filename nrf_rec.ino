#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal_I2C.h>
#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#define FIREBASE_HOST "crippledcare-10730-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "ZiArP6GKf0kZDo0oO7OBPhI7hwlpyDRwQfwWYuWc"

#define WIFI_SSID "PAPPURAJ-Linux"
#define WIFI_PASSWORD "11111111"

const char* ssid = "IoT";
const char* password = "12345678";

FirebaseData firebaseData;
LiquidCrystal_I2C lcd(0x27, 16, 2);
RF24 radio(D8, D0); // CE, CSN
const byte address[6] = "00001";


void dis(String a, String b, String c, String d) {
    a.concat(String("                    ").substring(0, 20 - a.length()));
    b.concat(String("                    ").substring(0, 20 - b.length()));
    c.concat(String("                    ").substring(0, 20 - c.length()));
    d.concat(String("                    ").substring(0, 20 - d.length()));
    lcd.print(a);
    lcd.print(c);
    lcd.print(b);
    lcd.print(d);
}


void writeDB(String field,String value){Firebase.setString(firebaseData, "/" +field, value );}





// HTML page served to the client
const char* html = R"(
<!DOCTYPE html>
<html>
<head>
<title>NodeMCU Server</title>
</head>
<body>
<h1>Enter a String</h1>
<form method="get">
<input type="text" name="inputString" placeholder="Enter a string">
<input type="submit" value="Submit">
</form>
</body>
</html>
)";

// EEPROM configuration
const int eepromSize = 4096;
const int stringAddress = 0;

WiFiServer server(80);







String urlDecode(String input) {
  String decoded = "";
  char c;
  int len = input.length();
  for (int i = 0; i < len; i++) {
    c = input.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%') {
      char hex[3];
      hex[0] = input.charAt(++i);
      hex[1] = input.charAt(++i);
      hex[2] = '\0';
      decoded += strtol(hex, nullptr, 16);
    } else {
      decoded += c;
    }
  }
  return decoded;
}









void writeStringToEEPROM(int addrOffset, String strToWrite)
{
  int len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  
 EEPROM.commit();
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(i+addrOffset+1, strToWrite[i]);
     EEPROM.commit();
  }
}

String readStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];

  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
   // Serial.println(data[i]);
  }
  data[newStrLen] = '\0';

  return String(data);
}






void startServer(){
  
  WiFiClient client = server.available();

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String request = client.readStringUntil('\r');
      int start = request.indexOf("inputString=") + 12;
      int end = request.indexOf("HTTP/1.1");
      String out=urlDecode(request.substring(start,end));
      if(!(out=="HTTP/" || out=="")){
        Serial.println(out);
        client.print("String saved to EEPROM.");
         writeStringToEEPROM(0,out);
        Serial.print("From EEPROM: ");
        Serial.println(readStringFromEEPROM(0));
      }
        client.flush();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println();
        client.println(html);
        break;
      }
    }

    delay(10);
    client.stop();
  }
}








void setup() {
  Serial.begin(9600);
  EEPROM.begin(500);
  lcd.begin();
  lcd.clear();
  lcd.backlight();
  pinMode(0,INPUT_PULLUP);


  if(!digitalRead(0)){
    dis("Connect to the WiFi","and set patient ID", "from any browser","IP: 192.168.4.1");
    WiFi.softAP(ssid, password);
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("Server IP: ");
    Serial.println(apIP);
    server.begin();
    while(1)startServer();
  }
  
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  dis("Welcome to", "Our project","","Connecting to WiFi!");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
        digitalWrite(D4, 0);
        delay(150);
        Serial.print(".");
        digitalWrite(D4, 1);
        delay(100);
  }
  
  delay(500);
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  writeDB("Hello","World");
  
}

int moisture = 0, ges = 0, oxygen = 0, pulse = 0;
float Temp = 0, rTemp,bTemp;
String ecg="";

void loop() {
  if (radio.available()) {
    char text[64] = "";
    radio.read(&text, sizeof(text));
    Serial.println(String(text));
    // dis(text,"");

    String inputString = String(text);
    String prefix = inputString.substring(0, 2);
    String val = inputString.substring(3);
    String temp;
    if (prefix == "Ms") {
      moisture = val.toInt();
    } else if (prefix == "Gs") {
      ges = val.toInt();
    } else if (prefix == "Rt") {
      rTemp = val.toFloat();
    } else if (prefix == "Ox") {
      oxygen = val.toInt();
    } else if (prefix == "Hr") {
      pulse = val.toInt();
    } else if (prefix == "EC") {
      ecg = val;
    } else if (prefix == "TB") {
      bTemp = val.toFloat();
    } 
    else {
      Serial.println("Invalid command");
    }

    String dis1 = "Need: ";
    switch (ges) {
        case 1:
          dis1+="food!";
          break;

        case 2:
          dis1+="water!";
          break;
          
        case 3:
        dis1+="medicine!";
          break;

        case 4:
        dis1+="emergency!";
          break;
        
        default:
          dis1+="nothing!";
          break;
    }

  String dis2=" Bed: ";
  if(moisture) dis2+="Fresh!";
  else dis2+="Wet!"  ;
  
  dis(dis1,dis2,"Temp: "+String(bTemp)+"  HR:"+String(pulse)," Oxy: "+String(oxygen)+"  BP: 120/80");



  }
}
