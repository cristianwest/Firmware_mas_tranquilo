//---------------------------------- LIBRERIAS -------------------------------------
#include <uuid.h>         //Generacion de uuid transaccion
#include <HTTPUpdate.h>   //Actualizacion remota codigo
#include <ArduinoJson.h>  //Utilización Json en arduino
#include "Segurito.h"     //Carga las funciones de seguridad necesarias para el Callback
#include "GprsHttpUpdate.h"

String RazonReinicio = "";
byte EstWingbox = 0;
int hReport = 13; //hora de reporte en formato 24h
byte HB = 0; //Variable para enviar estado en caso de solicitar reporte de sensores
byte HBS = 0; //Variable para enviar estado de alarma
byte HBB = 0;
byte HBA = 0;
boolean rstSend = false; //Variable para marcar el correcto envio del reset

//---------------- FORMACIÓN JSON PARA HEARTBEAT, RESET y ACTUALIZACION --------------
void EnviarEstados(String codigo, const char* RouKey, String MAC, int Fecha, int zonahoraria, String Status, float batery) {
  DynamicJsonBuffer jsonBuffer (JSON_ARRAY_SIZE(1000));
  JsonObject& equipo   = jsonBuffer.createObject();
  JsonObject& emisor   = jsonBuffer.createObject();
  JsonObject& cabecera = jsonBuffer.createObject();
  JsonObject& cuerpo = jsonBuffer.createObject();
  JsonObject& attr = jsonBuffer.createObject();
  JsonObject& estwingbox = jsonBuffer.createObject();
  JsonObject& tx       = jsonBuffer.createObject();
  emisor["equipo"]    =  equipo;
  equipo["id"] = MAC;
  cabecera["codigo"] = codigo;
  cabecera["version"]  = "1.0.0";
  cabecera["uuid"] = StringUUIDGen();
  cuerpo["zh"] = zonahoraria * 60;
  cuerpo["f"] = Fecha;
  cuerpo["uuid"] = MAC;
  cuerpo["a"] = attr;
  attr["conexion"] = "gprs";
  attr["fw"] = "0.0.3";
  attr["status"] = Status;
  attr["Bateria"] = batery;
  tx["emisor"] = emisor; //Tx junta toda la info y es el dato que se envía a RabbitMQ
  tx["cabecera"] = cabecera;
  tx["cuerpo"] = cuerpo;
  char jsonChar[1000];
  tx.printTo(jsonChar, tx.measureLength() + 1);
  Serial.println(jsonChar);
  sdLog(jsonChar);

  if (mqtt.publish(RouKey, jsonChar)) {
    
    if (codigo == "appbox.dispositivo.heartbeat") {
      EstWingbox = 0;
    }
    else if (codigo == "appbox.dispositivo.reiniciado") {
      rstSend = true;
    }
  } else {
    
  }
}

//--------------------------- ACTUALIZACION FIRMWARE ------------------------------------
void actualizar_firmware(JsonObject & tx_cuerpo) {
  String host    = tx_cuerpo["host"];
  String url     = tx_cuerpo["url"];
  if(modem.isGprsConnected()){
    GprsHttpUpdate(host,url);
  }
}
//------------------------------- ENVIO EVENTO RESET -------------------------------------
void EnvioReset(int FechaSeg, int year_reset) {
  if (year_reset >= 2021) {
    if (!rstSend ) {
      setup_mqtt();
      EnviarEstados("appbox.dispositivo.reiniciado", MQTT_ROUTING_KEY3, WiFi.macAddress(), FechaSeg, (int)timezone, "", bateria);
    }
  }
}
//------------------------- Formacion JSON y Envio Rabbit ---------------------------------
void EnviarRabbit(String codigo, String MAC, String ID, int Fecha, int zonahoraria) {
  DynamicJsonBuffer jsonBuffer (JSON_ARRAY_SIZE(1000));
  JsonObject& equipo   = jsonBuffer.createObject();
  JsonObject& emisor   = jsonBuffer.createObject();
  JsonObject& cabecera = jsonBuffer.createObject();
  JsonObject& cuerpo = jsonBuffer.createObject();
  JsonObject& attr = jsonBuffer.createObject();
  JsonObject& tx       = jsonBuffer.createObject();
  emisor["equipo"]    =  equipo;
  equipo["id"] = MAC;
  cabecera["codigo"] = codigo;
  cabecera["version"]  = "1.0.0";
  cabecera["uuid"] = StringUUIDGen();
  cuerpo["f"] = Fecha;
  cuerpo["uuid"] = MAC;
  cuerpo["zh"] = zonahoraria * 60;
  cuerpo["a"] = attr;
  attr["puerto"] = "1";
  attr["id_user"] = ID;
  attr["conexion"] = "gprs";
  tx["emisor"] = emisor; //Tx junta toda la info y es el dato que se envía a RabbitMQ
  tx["cabecera"] = cabecera;
  tx["cuerpo"] = cuerpo;
  tx.printTo(jsonChar, tx.measureLength() + 1);
  Serial.println(jsonChar);

  if (codigo=="appbox.baliza.encendida"){
      if (mqtt.publish(MQTT_ROUTING_KEY5, jsonChar)) {
      } 
  }
  if (codigo=="appbox.baliza.apagada"){
     if (mqtt.publish(MQTT_ROUTING_KEY6, jsonChar)) {
      } 

  }
  if (codigo=="appbox.sirena.encendida"){
     if (mqtt.publish(MQTT_ROUTING_KEY7, jsonChar)) {
      } 

  }
  if (codigo=="appbox.sirena.apagada"){
     if (mqtt.publish(MQTT_ROUTING_KEY8, jsonChar)) {
      } 

  }
  if (codigo=="appbox.alarma.encendida"){
     if (mqtt.publish(MQTT_ROUTING_KEY9, jsonChar)) {
      } 

  }
  if (codigo=="appbox.alarma.apagada"){
     if (mqtt.publish(MQTT_ROUTING_KEY10, jsonChar)) {
      } 

  }
}

//-------------------------------- CALLBACK RABBIT -------------------------------------------
void callback_mqtt(char* topic, byte * payload, unsigned int length) {
  //Transformar en variables un archivo JSON: https://arduinojson.org/v5/assistant/
  String cadena_in = "";
  for (int i = 0; i < length; i++) {
    cadena_in = cadena_in + (String((char)payload[i]));
  }
  cadena_in.trim();
  Serial.println("Mensaje Recibido:");
  Serial.println(cadena_in);
  //Desmenuza el json que llega a la placa
  DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(1000));
  JsonObject& json_in = jsonBuffer.parseObject(cadena_in);
  JsonObject& cabecera = json_in["cabecera"];
  JsonObject& cuerpo   = json_in["cuerpo"];
  JsonObject& emisor   = json_in["emisor"];
  String tx_routing_key = cabecera["routing_key"];
  String tx_codigo      = cabecera["codigo"];
  String tx_version     = cabecera["version"];
  String codigo =tx_codigo.substring(tx_codigo.indexOf('.')+1);
  String actuador = codigo.substring(0, codigo.indexOf('.'));
  String accion =codigo.substring(codigo.indexOf('.')+1);
  if (actuador == "sirena") {
    sirena(accion);
  }
  if (actuador == "baliza") {
    baliza(accion);
  }
  if (actuador == "alarma") {
    if(accion =="encender"){
      alarmaState=true;
    }
    else if(accion =="apagar"){
      alarmaState=false;
    }
    sirena(accion);
    baliza(accion);
  }
  else if (tx_codigo == "appbox.dispositivo.test") {
    String code = cuerpo["test"];
    if (code == "sensores") {
      alarmState = "ESTADO";
    }
  }
  else if (tx_codigo == "appbox.dispositivo.actualizar") {
    actualizar_firmware(cuerpo);
  }
  else if (tx_codigo == "appbox.dispositivo.reiniciar") {
    ESP.restart();
  }
}
