//---------------------------------- LIBRERIAS -------------------------------------
#include <HardwareSerial.h> //Utilización de otros pines para comunicación serial
#include "Rabbit.h"         //Incluye funciones y variables de Rabbit.h
#include "Json.h"           //Incluye funciones y variables de Json.h

int flagLog = 0;
//---------------------- HEARTBEAT - Envío del estado del sistema --------------------------
void EstadoWingBox() {
  if (Hour==15 && Minutes==2 && Seconds == 0){//Reinicio 15:02:00
     ESP.restart();
  }
  if ((Minutes%10 ==0 )) {
    if (EstWingbox == 1) {
      if(HB==1){
      Serial.println("HeartBeat enviado");
      sdLog("HeartBeat enviado");
      EnviarEstados("appbox.dispositivo.heartbeat", MQTT_ROUTING_KEY2, WiFi.macAddress(), FechaEpoch, (int)timezone, estado, bateria);
      HB=0;
      }
    }
  } 
  else {
    HB=1;
    EstWingbox = 1;
  }

  if ((sirenaState == true) && (HBS == 0) && (alarmaState != true)) {
    EnviarRabbit("appbox.sirena.encendida", WiFi.macAddress(), idUser, FechaEpoch, (int)timezone);
    HBS = 1;
  }
  else if ((sirenaState == false) && (HBS == 1)) {
    EnviarRabbit("appbox.sirena.apagada", WiFi.macAddress(), idUser, FechaEpoch, (int)timezone);
    HBS = 0;
  }
  else if ((balizaState == true) && (HBB == 0) && (alarmaState != true)) {
    EnviarRabbit("appbox.baliza.encendida", WiFi.macAddress(), idUser, FechaEpoch, (int)timezone);
    HBB = 1;
  }
  else if ((balizaState == false) && (HBB == 1)) {
    EnviarRabbit("appbox.baliza.apagada", WiFi.macAddress(), idUser, FechaEpoch, (int)timezone);
    HBB = 0;
  }
  else if ((alarmaState == true) && (HBA == 0)) {
    EnviarRabbit("appbox.alarma.encendida", WiFi.macAddress(), idUser, FechaEpoch, (int)timezone);
    HBA = 1;
  }
  else if ((alarmaState == false) && (HBA == 1)) {
    EnviarRabbit("appbox.alarma.apagada", WiFi.macAddress(),  idUser, FechaEpoch, (int)timezone);
    HBA = 0;
  }
 
}

//---------------------------------- SETUP ----------------------------------------
void setup() {

  Serial.begin(115200);
  Serial.println("Inicio WingBox");
  sdLog("Inicio WingBox");
  WiFi.macAddress().toCharArray(MQTT_CLIENT, WiFi.macAddress().length() + 1);// se utiliza para la cola y para el nombre del bluetooth
  sd_card();
  WiFi.mode(WIFI_OFF);
  btStop();
  //----------------RELAYS-------------------------------------
  pinMode(pinSirena1, OUTPUT); digitalWrite(pinSirena1, HIGH);
  pinMode(pinSirena2, OUTPUT); digitalWrite(pinSirena2, HIGH);
  pinMode(pinBaliza, OUTPUT); digitalWrite(pinBaliza, HIGH);
  //--------------BATERIA TTGO-------------------------------------
  //I2CPower.begin(I2C_SDA, I2C_SCL, 400000);                       //Estable la dirección para utilizar baterias
  //bool isOk = setPowerBoostKeepOn(1);                             // Mantiene la energía desde la batería
  //---------------SIM800L-----------------------------------------
  //pinMode(MODEM_PWKEY, OUTPUT); digitalWrite(MODEM_PWKEY, LOW);   //Pin de configuración alimentación
  //pinMode(MODEM_RST, OUTPUT); digitalWrite(MODEM_RST, HIGH);     //Pin de configuración alimentación
  //pinMode(MODEM_POWER_ON, OUTPUT); digitalWrite(MODEM_POWER_ON, HIGH); //Pin de configuración alimentación
  /*pinMode(MODEM_RESET, OUTPUT); digitalWrite(MODEM_RESET, HIGH);
  digitalWrite(MODEM_RESET, LOW);
  delay(2000);
  digitalWrite(MODEM_RESET, HIGH);
  Serial.print("Espera mientras se reinicia el modulo...");
  delay(20000);*/

  
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);           //Serial para comunicación con simcard
  Serial.print("Iniciando modulo");
  sdLog("Iniciando modulo");

    // Envío de comando AT para reiniciar el módulo
  SerialAT.println("AT+CRESET");
  delay(5000); // Esperar tiempo suficiente para que el módulo reinicie
  
  // Verificación de inicio del módem
  while (!modem.init()) {                                         //Inicializa modem. Sin simcard no inicializara nunca
    Serial.println(".");
  }
  delay(2000);
  //----------------- CONFIGURACION & CONEXION ------------------------------
  MQTT_TOPIC_SUBSCRIBE1 = String(MQTT_CLIENT);                    //Topico equivalente a la mac del dispositivo para enviar comandos
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);                         //Configuracion clientes rabbit
  mqtt.setCallback(callback_mqtt);                                //Configuracion clientes rabbit
  ConexionAPN();
  delay(1000);
  setup_mqtt();
  sdLog("prueba logs");                                           //Intenta la conexion con rabbit gprs

  //-------------REINICIO---------------------------------------------------
  RazonReinicio = String(esp_reset_reason());
}
void logsLoop(){
  if ((Hour==10 && Minutes==51 )) {
    if(flagLog==1){
      listado_de_logs();
      flagLog=0;
    }
  } 
  else {
    flagLog=1;
  }
}

//------------------------------------ LOOP -----------------------------------------
void loop() {
  tiempo = millis();
  ConexionAPN();
  setup_mqtt();
  LocalTime();
  //MEDICION BATERIA
  if (alarmState == "") { //Siempre que no se ejecute otra tarea
    EstadoComponentes(2); 
  }                    //Obtiene la fecha actual
  EnvioReset(FechaEpoch, Year);   //Envia el evento para verificar que se realizo el reset
  EstadoWingBox();                //heartbeat
  AlarmaLoop();
  Rabbitloop();
  logsLoop();
}
