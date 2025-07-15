//---------------------------------- LIBRERIAS -------------------------------------
#include "GPRS_WiFi.h"
#include <PubSubClient.h> /* https://pubsubclient.knolleary.net/ */
#include "microSD.h"

//--------------------------------- MQTT ---------------------------------------------
const int MQTT_PORT = 1883; // Puerto MQTT
char MQTT_CLIENT[17];
//PRODUCTIVO MAS TRANQUILO
const char *MQTT_USER = "mas_tranquilo:mas_tranquilo";          //WEST//vujvftwg:vujvftwg
const char *MQTT_PASS = "a0w7c1Q4BeppRB";   //WEST////FHMFZx8mv_db_wj9_YCY0rIkxe_YS4c_
const char *MQTT_SERVER = "artistic-whale.rmq.cloudamqp.com"; //WEST //artistic-whale.rmq.cloudamqp.com

//PRODUCTIVO
//const char *MQTT_USER = "vujvftwg:vujvftwg";          //WEST
//const char *MQTT_PASS = "FHMFZx8mv_db_wj9_YCY0rIkxe_YS4c_";   //WEST
//const char *MQTT_SERVER = "artistic-whale.rmq.cloudamqp.com"; //WEST

//DESARROLLO

//const char *MQTT_USER = "juhcioka:juhcioka";          //WEST
//const char *MQTT_PASS = "7Z6JiCWwWfuSPW_Ak1ED_zbiWxuBUSxl";   //WEST
//const char *MQTT_SERVER = "bossy-raccoon.rmq.cloudamqp.com"; //WEST

const char* MQTT_ROUTING_KEY = "appbox.dispositivo.seguridad";
const char* MQTT_ROUTING_KEY2 = "appbox.dispositivo.heartbeat";
const char* MQTT_ROUTING_KEY3 = "appbox.dispositivo.reiniciado";
const char* MQTT_ROUTING_KEY4 = "appbox.dispositivo.actualizado";
const char* MQTT_ROUTING_KEY5 = "appbox.baliza.encendida";
const char* MQTT_ROUTING_KEY6 = "appbox.baliza.apagada";
const char* MQTT_ROUTING_KEY7 = "appbox.sirena.encendida";
const char* MQTT_ROUTING_KEY8 = "appbox.sirena.apagada";
const char* MQTT_ROUTING_KEY9 = "appbox.alarma.encendida";
const char* MQTT_ROUTING_KEY10 = "appbox.alarma.apagada";

String MQTT_TOPIC_SUBSCRIBE1 = "";

PubSubClient mqtt(MQTT_SERVER, MQTT_PORT, gprsClient); //cliente gprs

void setup_mqtt() { 
  if (modem.isGprsConnected() != 1) {
    ConexionAPN();
  }
  else if (tiempo - periodo2 >= 1000) {
    if (!mqtt.connected()) {
      Serial.print("MAC: ");
      Serial.println(MQTT_CLIENT); 
      delay(100);
      if (mqtt.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS)) {
        mqtt.subscribe(MQTT_TOPIC_SUBSCRIBE1.c_str());
        Serial.println("MQTT Conectado");
      }
      else {
        periodo2 = tiempo;
      }
    }
  }
}

//--------------------------------- LOOP MQTT -------------------------------------
void Rabbitloop() {
  if (modem.isGprsConnected() == 1) {
    mqtt.loop();
  }
}
