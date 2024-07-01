#include <Arduino.h>
#include <ArduinoJSON.h>

String jsonBuilder(String device_id){
  JsonDocument doc, connected_device;
  String output;
  doc["name"] = "IOT " + device_id;
  connected_device["value"] = false;
  connected_device["type"] = "relay";
  doc["connected_device"] = connected_device;
  serializeJson(doc, output); 
  return output; // return string
}

