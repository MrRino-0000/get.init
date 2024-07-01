#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <ArduinoJSON.h>
#include <secrets.h>
#include <startup.h>
#include <writer.h>


// wifi config
WiFiClientSecure ssl_client1, ssl_client2;
DefaultNetwork network;
// end wifi config

// firebase config
// Authentication
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD, 3000 /* expire period in seconds (<3600) */);
FirebaseApp app;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client1, getNetwork(network)), aClient2(ssl_client2, getNetwork(network));
bool taskComplete = false;
void asyncCB(AsyncResult &aResult);
void printResult(AsyncResult &aResult);
void databaseFetcher();
void firebaseTask();
void triggerRelay();

// Database
RealtimeDatabase Database;
// end firebase config

// variable
unsigned long ms = 0;
const char* device_name = "device_name";
String  path = "/devices/";
String device_id = "";
bool status = false;
const int ledPin = 2;
#define EXTRA_PIN D2
#define RED_LED D0
#define GREEN_LED D3
bool doneFetching = false;
void checkStatus();

void firebaseSetup(){
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  ssl_client1.setInsecure();
  ssl_client1.setBufferSizes(4096, 1024);
  ssl_client2.setInsecure();
  ssl_client2.setBufferSizes(4096, 1024);
  
  initializeApp(aClient2, app, getAuth(user_auth), asyncCB, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  Database.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
  Database.get(aClient, path, asyncCB, true, "streamTask");
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(EXTRA_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  digitalWrite(EXTRA_PIN, LOW);
  digitalWrite(RED_LED, HIGH);
  digitalWrite(ledPin, LOW);
  digitalWrite(GREEN_LED, LOW);
  device_id = DEVICE_ID;
  path = path + device_id;
  wifiSetup();
  firebaseSetup();
}

void loop() {
  checkStatus();
  firebaseTask();
  triggerRelay();
}

void checkStatus(){
  if (taskComplete){
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
  }else{
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }
}

void firebaseTask(){
  app.loop();
  // To get the authentication time to live in seconds before expired.
  Database.loop();
  if (app.ready() && !taskComplete){
    taskComplete = true;
    databaseFetcher();
    // Print authentication info
    Serial.println("Authentication Information");
    Firebase.printf("User UID: %s\n", app.getUid().c_str());
    Firebase.printf("Auth Token: %s\n", app.getToken().c_str());
    Firebase.printf("Refresh Token: %s\n", app.getRefreshToken().c_str());
  }
}

void databaseFetcher(){
  if (millis() - ms > 5000){
    ms = millis();
    String x = jsonBuilder(device_id);
    Database.set<object_t>(aClient2, path, x, asyncCB, "setTask");
  }
}

void triggerRelay(){
  if (status == 0){
    digitalWrite(ledPin, LOW);
    digitalWrite(EXTRA_PIN, HIGH);
  }else{
    digitalWrite(EXTRA_PIN, LOW);
    digitalWrite(ledPin, HIGH);
  }
}

void asyncCB(AsyncResult &aResult){
  // WARNING!
  // Do not put your codes inside the callback and printResult.
  printResult(aResult);
}

void printResult(AsyncResult &aResult){
  if (aResult.isEvent()){
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
  }

  if (aResult.isDebug()){
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()){
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  if (aResult.available()){
    RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
    if (RTDB.isStream()){
      Serial.println("----------------------------");
      Firebase.printf("task: %s\n", aResult.uid().c_str());
      Firebase.printf("event: %s\n", RTDB.event().c_str());
      Firebase.printf("path: %s\n", RTDB.dataPath().c_str());
      Firebase.printf("data: %s\n", RTDB.to<const char *>());
      Firebase.printf("type: %d\n", RTDB.type());
      if (RTDB.type() != 0){
        status = RTDB.to<bool>();
        String v5 = RTDB.to<String>();
      }
    }else{
      Serial.println("----------------------------");
      Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }

    Firebase.printf("Free Heap: %d\n", ESP.getFreeHeap());
  }
}
