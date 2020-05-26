/*
    This sketch establishes a TCP connection to a "quote of the day" service.
    It sends a "hello" message, and then prints received data.
*/
#include <Average.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WiFiClientSecureBearSSL.h>

// defines pins numbers - distance sensor
const int trigPin = 5;  //D4
const int echoPin = 4;  //D3
#define USONIC_DIV 58.0


ESP8266WiFiMulti WiFiMulti;

String CONFIGURL = "https://myurl-oiltank.json";
String unitID = "w23k8y4-oiltank";
int RESPSIZE = 700;
String  NOTIFY_ALERT_URL = "";
int MEASURE_SAMPLE_DELAY = 2000;
int MEASURE_SAMPLE_COUNT = 6;
String  SENSOR_DATA_POST_URL = "";
int TANK_ALERT_VALUE = 1700;
int TANK_EMPTY_VALUE = 1800;
int TANK_FULL_VALUE = 1200;
String  TIME_API_URL = ""; 
String CURR_TIME = "";
int WEEK_NUMBER = 0;
long UNIXTIME = 0l;
long DEEP_SLEEP_INTERVAL = 3600e6;//1 hour
JsonObject configData;

Average<long> ave(MEASURE_SAMPLE_COUNT);

// defines variables
long duration;
long distance;

// def end  - distance sensor


#ifndef STASSID
#define STASSID "XX"
#define STAPSK  "XX"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;
int LED = 2;

void setup() {
 
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode (LED, OUTPUT);
  
  Serial.begin(115200);
  digitalWrite(LED, HIGH); // Turn the LED off by making the voltage HIGH
   delay(1000); // Wait for a second
     digitalWrite(LED, LOW);
  delay(1000); 
     
    Serial.print("start ...... ");
  fluidlogic();
  
}

  void fluidlogic(){
    digitalWrite(LED, HIGH);
    
    // We start by connecting to a WiFi network
    connectToWifi();
    getConfigData();
    getCurrentTime();
    //collect sensor data and send to Firebase 
    sendSensorSats();
    Serial.print("switch off led ...... ");
    digitalWrite(LED, LOW);
    gotoSleep();
    
    }

    void gotoSleep(){
      Serial.println("Going into  sleep for 20 seconds");
      delay(2000);
      Serial.println("Going into deep sleep for 20 seconds");
      ESP.deepSleep(DEEP_SLEEP_INTERVAL); // 20e6 is 20 microseconds
      }

    void connectToWifi() {
        WiFi.mode(WIFI_STA);
        WiFiMulti.addAP(ssid, password);

       if  ((WiFiMulti.run() != WL_CONNECTED))  {
             Serial.print("Not Connected  ");
             delay(1000);
       }
      // We start by connecting to a WiFi network
      if  ((WiFiMulti.run() == WL_CONNECTED))  {
        Serial.println();
        Serial.print("V2 Connected to ");
        Serial.println(WiFi.SSID());
  
        Serial.println("WiFi connect status:");
         Serial.println(WL_CONNECTED);
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.print("WIFI Connected... ");
      }

    }

  void  sendSensorSats(){
    const size_t CAPACITY = JSON_OBJECT_SIZE(5);
      connectToWifi();
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
      // client->setFingerprint(fingerprint);
      client->setInsecure();
      HTTPClient https;    //Declare object of class HTTPClient

      long distInMM = measureFluid();
        Serial.println("");
        Serial.print("Distance: ");
        Serial.println(distInMM);

      //StaticJsonDocument<CAPACITY> doc; //https://arduinojson.org/v6/how-to/use-arduinojson-with-esp8266httpclient/
      DynamicJsonDocument doc(2048);
       doc["level"] = distInMM;
       doc["time"] = CURR_TIME;
       doc["weeknumber"]  =  WEEK_NUMBER;
       doc["unixtime"] = UNIXTIME;
       doc["ip"] = WiFi.localIP().toString();

      String data;
      serializeJson(doc, data);
      Serial.print("Sending : ");
      Serial.println(data);
      //   "https://fluidlevel-910e0.firebaseio.com/w23k8y4-oiltank.json"
      if ( https.begin(*client,SENSOR_DATA_POST_URL)){      //Specify request destination
       https.addHeader("Content-Type", "application/json");  //Specify content-type header

      int httpCode = https.POST(data);   //Send the request
      String payload = https.getString();                  //Get the response payload
    
      Serial.println(httpCode);   //Print HTTP return code
      Serial.println(payload);    //Print request response payload
       Serial.println("**************************************** DONE***************"); 
    
      https.end();  //Close connection
      }
    }


    
void loop() {
  
}

  // measure fluid number of times 
  long measureFluid() {
    //myStats.
    for (int i = 0 ; i < MEASURE_SAMPLE_COUNT ; i++) {
    
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
  
      // Sets the trigPin on HIGH state for 10 micro seconds
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
  
      // Reads the echoPin, returns the sound wave travel time in microseconds
      duration = pulseIn(echoPin, HIGH);
  
  
      // Calculating the distance
      //distance= duration*0.034/2;
      // distance = (duration / 2) / 29.1;
      distance = (long) (((float) duration / USONIC_DIV) * 10.0);
      //Serial.print("Run.>>"); Serial.print(i);Serial.print(" = "); Serial.print(distance);
      ave.push(distance);
   
      delay(200);
    }
    long ans = ave.mean();
    //Serial.println(ans);
    return ans;
  }

  
  void getCurrentTime(){
    connectToWifi();
     WiFiClient client;

    HTTPClient timehttps;
   
    if (  timehttps.begin(client,"http://worldtimeapi.org/api/timezone/Europe/dublin.json")){
    int httpCode = timehttps.GET();
      Serial.print("http TIME response code :");
      Serial.println(httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
           DynamicJsonDocument doc(RESPSIZE);
            String httpresp = timehttps.getString();
            deserializeJson(doc, httpresp);
  
            CURR_TIME =  doc["datetime"].as<String>(); //String(ptrDateTime);
            WEEK_NUMBER = doc["week_number"].as<int>();
            UNIXTIME = doc["unixtime"].as<long>();

            Serial.print("http TIME response  :");
        Serial.println("| ");        Serial.print(CURR_TIME);
        Serial.println("| ");        Serial.print(WEEK_NUMBER);
        Serial.println("| ");        Serial.print(UNIXTIME);
        Serial.println("| ");
        
      }
      timehttps.end();
    }
    }

  void getConfigData() {
     connectToWifi();
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
      // client->setFingerprint(fingerprint);
      client->setInsecure();
       HTTPClient https;

       Serial.print("[HTTPS] begin...\n");
     if ( https.begin(*client, CONFIGURL)){
      int httpCode = https.GET();
      Serial.print("http response code :");
      Serial.println(httpCode);
    
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        DynamicJsonDocument doc(RESPSIZE);
        String httpresp = https.getString();
        deserializeJson(doc, httpresp);
        // Get the first element of the array
        JsonObject configRoot = doc.as<JsonObject>();
        
        for (JsonPair kv : configRoot) {
          Serial.println(kv.key().c_str());
          configData = kv.value().as<JsonObject>();
          break;
        }

        NOTIFY_ALERT_URL = configData["notify_alert_url"].as<String>(); //String(ptrNotifyAlerturl);
        MEASURE_SAMPLE_DELAY = configData["measure_sample_delay"].as<int>();
        MEASURE_SAMPLE_COUNT = configData["measure_samples_count"].as<int>();

        SENSOR_DATA_POST_URL = configData["sensor_datapost_url"].as<String>(); //String(ptrsensorUrl);
       
        TANK_ALERT_VALUE = configData["tank_alert_value"].as<int>();
        TANK_EMPTY_VALUE = configData["tank_empty_value"].as<int>();
        TANK_FULL_VALUE = configData["tank_full_value"].as<int>();
        
        TIME_API_URL = configData["time_api_url"].as<String>(); //String(ptrTimeApiUrl);
      
    
        Serial.print("http response  :");
        Serial.print("| ");       Serial.println(NOTIFY_ALERT_URL);
        Serial.print("| ");        Serial.println(MEASURE_SAMPLE_DELAY);
        Serial.print("| ");        Serial.println(MEASURE_SAMPLE_COUNT);
        Serial.print("| ");        Serial.println(SENSOR_DATA_POST_URL);
        Serial.print("| ");        Serial.println(TANK_ALERT_VALUE);
        Serial.print("| ");        Serial.println(TANK_EMPTY_VALUE);
        Serial.print("| ");        Serial.println(TANK_FULL_VALUE);
        Serial.print("| ");        Serial.println(TIME_API_URL);
        //https.end();
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      Serial.println("End ...getConfigData");
     https.end();
      Serial.println("End ...Https");
     }//https begin
     
      delay(10000);
       
    }
