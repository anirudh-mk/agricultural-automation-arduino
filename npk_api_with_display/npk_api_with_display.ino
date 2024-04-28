#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <SoftwareSerial.h>
#include <Wire.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define RE D4
#define DE D5

const char* ssid = "MK";
const char* password = "123456789";

String serverUUID = "e1d3282a-2743-447a-859e-a2b583b8ac34";
String serverName = "http://anirudhmk123.pythonanywhere.com/api/v1/farm/send/" + serverUUID + "/";

String serverPostName = "http://anirudhmk123.pythonanywhere.com/api/v1/farm/post/" + serverUUID + "/";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

int menu_navigation_pin = A0;

int screen = 0;



//npk

const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte values[11];

SoftwareSerial mod(D3, D6);


void setup() {
  //npk
  Serial.begin(9600);
  mod.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);

  pinMode(menu_navigation_pin, INPUT);

  lcd.init();
  lcd.clear();
  lcd.backlight();
  
  //wifi
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;

      String serverPath = serverName + "?temperature=24.37";
      http.begin(client, serverPath.c_str());

      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);

        // Parse JSON response
        StaticJsonDocument<256> doc; // Adjust the size as needed
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          JsonObject message = doc["message"]["general"][0][0]; // Access the first object in the "message" array
          String nitrogen_str = message["n"];
          String phosphorous_str = message["p"];
          String potasium_str = message["k"];
          String time_required_str = message["time_required"];

          // Convert strings to integers
          int required_nitrogen = nitrogen_str.toInt();
          int required_phosphorous = phosphorous_str.toInt();
          int required_potasium = potasium_str.toInt();
          int time_required = time_required_str.toInt();

          Serial.println("n: " + String(required_nitrogen));
          Serial.println("p: " + String(required_phosphorous));
          Serial.println("k: " + String(required_potasium));
          Serial.println("time_required: " + String(time_required));
          
          //npk

          float measured_nitrogen, measured_phosphorous, measured_potassium;
          measured_nitrogen = measure("Nitrogen", nitro);
          delay(250);
          measured_phosphorous = measure("Phosphorous", phos);
          delay(250);
          measured_potassium = measure("Potassium", pota);
          delay(250);


          if(required_nitrogen>measured_nitrogen){
            digitalWrite(D0,HIGH);
            Serial.println("turn on nitrogen motor");  
          }
          else{
            Serial.println("turn off nitrogen motor");
            digitalWrite(D0,LOW);
          }
            if(required_phosphorous>measured_phosphorous){
            Serial.println("turn on phosphorous motor");  
            digitalWrite(D7,LOW);
          }
          else{
            Serial.println("turn off phosphorus motor");
            digitalWrite(D7,HIGH);
          }
            if(required_potasium>measured_potassium){
            Serial.println("turn on potassium motor");
            digitalWrite(D8,LOW);  
          }
          else{
            Serial.println("turn off potassium motor");
            digitalWrite(D8,HIGH);
          }
        
          int menu_navigation_value = map(analogRead(menu_navigation_pin), 0, 1023, 0, 2);
          Serial.println(menu_navigation_value);

          Serial.println(analogRead(menu_navigation_pin));
        
          
          if (menu_navigation_value > 1)
          {
            screen++;
            if (screen > 2)
            {
              screen = 0;
            }
          }
          if (menu_navigation_value < 1)
          {
            screen--;
            if (screen < 0)
            {
              screen = 2;
            }
          }
        
        
          switch (screen)
          {
            case 0:
              delay(500);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Nitrogen : " + String(measured_nitrogen));
              lcd.setCursor(0, 1);
              lcd.print("Required : " + String(required_nitrogen));
              break;
            case 1:
              delay(500);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phosporous : " + String(measured_phosphorous));
              lcd.setCursor(0, 1);
              lcd.print("Required : " + String(required_phosphorous));
              break;
            case 2:
              delay(500);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Potassium : " + String(measured_potassium));
              lcd.setCursor(0, 1);
              lcd.print("Required : "+ String(required_potasium));
              break;
          }
                  
          
          

          
          String url = String(serverPostName) + "?n=" + String(measured_nitrogen) + "&p=" + String(measured_phosphorous) + "&k=" + String(measured_potassium);

          
          http.begin(client, url);
  
          http.addHeader("Content-Type", "application/x-www-form-urlencoded");
          int httpResponseCode = http.POST("");
     
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
            
          http.end();

          delay(1000);
          
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
        }
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

float measure(const char* name, const byte* command) {
  float value = 0.0;
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  if (mod.write(command, 8) == 8) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
    value = values[4];
    Serial.print(name);
    Serial.print(": ");
    Serial.print(value);
    Serial.println("mg/kg");
  }
  return value;
}
